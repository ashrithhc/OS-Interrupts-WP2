#include <sys/defs.h>
#include <sys/ahci.h>
#include <sys/pci.h>
#include <sys/writeblock.h>
#include <sys/kprintf.h>
#define BOOL uint8_t
#define TRUE 1
#define FALSE 0
#define LOBYTE(w) ((uint8_t)(w))
#define HIBYTE(w) ((uint8_t)(((uint32_t)(w) >> 8) & 0xFF))

void *memset(void *s, int c, size_t n)
{
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

int find_cmdslot(hba_port_t *port)
{
	uint32_t slots = (port->sact | port->ci);
	for (int i=0; i<32; i++)
	{
		if ((slots&1) == 0) return i;
		slots >>= 1;
	}
	kprintf("Cannot find free command list entry\n");
	return -1;
}

BOOL read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf)
{
	port->is_rwc = (uint32_t)(-1);		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port);
	if (slot == -1) return FALSE;
 
	hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = (uint32_t)((count-1)>>4) + 1;	// PRDT entries count
 
	hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(hba_prdt_entry_t));
 
	int i;
	for (i=0; i<cmdheader->prdtl-1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint64_t)(buf);
		cmdtbl->prdt_entry[i].dbc = 8*1024;	// 8K bytes
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	cmdtbl->prdt_entry[i].dba = (uint64_t)(buf);
	cmdtbl->prdt_entry[i].dbc = count<<9;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;
 
	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);
 
	cmdfis->count = count;
 
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		kprintf("Port is hung\n");
		return FALSE;
	}
 
	port->ci = 1<<slot;	// Issue command
 
	while (1)
	{
		if ((port->ci & (1<<slot)) == 0) break;
		if (port->is_rwc & HBA_PxIS_TFES)	// Task file error
		{
			kprintf("Read disk error\n");
			return FALSE;
		}
	}
 
	if (port->is_rwc & HBA_PxIS_TFES)
	{
		kprintf("Read disk error\n");
		return FALSE;
	}
 
	return TRUE;
}
 
BOOL write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf)
{
	port->is_rwc = (uint32_t)(-1);		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port);
	if (slot == -1) return FALSE;
 
	hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 1;		// Read from device
	cmdheader->prdtl = (uint32_t)((count-1)>>4) + 1;	// PRDT entries count
 
	hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) + (cmdheader->prdtl-1)*sizeof(hba_prdt_entry_t));
 
	int i;
	for (i=0; i<cmdheader->prdtl-1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint64_t)(buf);
		cmdtbl->prdt_entry[i].dbc = 8*1024;	// 8K bytes
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	cmdtbl->prdt_entry[i].dba = (uint64_t)(buf);
	cmdtbl->prdt_entry[i].dbc = count<<9;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;
 
	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_WRITE_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);
 
	cmdfis->count = count;
 
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		kprintf("Port is hung\n");
		return FALSE;
	}
 
	port->ci = 1<<slot;	// Issue command
 
	while (1)
	{
		if ((port->ci & (1<<slot)) == 0) break;
		if (port->is_rwc & HBA_PxIS_TFES)	// Task file error
		{
			kprintf("Read disk error\n");
			return FALSE;
		}
	}
 
	if (port->is_rwc & HBA_PxIS_TFES)
	{
		kprintf("Read disk error\n");
		return FALSE;
	}
 
	return TRUE;
}
 
void writeToDisk(hba_port_t *port){
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
	
	char* writeStr = (char*) 0x900000;
	char *readStr = (char*) 0x700000;

	for(int i=0; i<100; i++){
		for(int blockOffset = 0; blockOffset < 8; blockOffset++){
			for(int c = 0; c<512; c++){
				*(writeStr+(512*blockOffset + c)) = (char)(i);
			}
		}
		write(port, i*8, 0, 8, (uint16_t *)writeStr);
		read(port, i*8, 0, 8, (uint16_t *)readStr);
	}
	kprintf("%s\n", readStr);
}
