#include <sys/defs.h>
#include <sys/ahci.h>
#include <sys/kprintf.h>

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier
#define AHCI_DEV_NULL	0xFFFFFFFF	// Null drive

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;
	uint32_t lbus = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint16_t temp_addr;

	address = (uint32_t)( (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000) );

	__asm__ __volatile__ ("outl %0, %1" : : "a"(address), "Nd"((uint16_t)0xCF8));

	uint32_t ret;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (ret) : "Nd" ((uint16_t)0x0CFC));

	temp_addr = (uint16_t)(ret >> ((offset & 2) * 8) & 0xffff);

	return temp_addr;
}

uint16_t getVendor(uint8_t bus, uint8_t slot){
	return pciConfigReadWord(bus, slot, 0, 0);
}

void bringDownMemory(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;
	uint32_t lbus = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	
	address = (uint32_t)( (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000) );

	__asm__ __volatile__ ("outl %0, %1" : : "a"(address), "Nd"((uint16_t)0xCF8));
	__asm__ __volatile__ ("outl %0, %1" : : "a"(0x30000000), "Nd"((uint16_t)0xCFC));
}

int check_type(hba_port_t *port)
{
	uint32_t ssts = port->ssts;
 
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != 3) return AHCI_DEV_NULL;
	if (ipm != 1) return AHCI_DEV_NULL;
 
	switch (port->sig)
	{
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}

void probePort(hba_mem_t *abar){
	uint32_t pi = abar->pi;

	for(int i=0; i<32; i++){
		if(pi & 1){
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA)
			{
				kprintf("SATA drive found\n");
				return;
			}
			else if (dt == AHCI_DEV_SATAPI)
			{
				kprintf("SATAPI drive found at port\n");
				return;
			}
			else if (dt == AHCI_DEV_SEMB)
			{
				kprintf("SEMB drive found at port\n");
				return;
			}
			else if (dt == AHCI_DEV_PM)
			{
				kprintf("PM drive found at port\n");
				return;
			}
			else
			{
				kprintf("No drive found at port\n");
			} 
		}
		pi >>= 1;
	}
}

void getFirstDevice(uint8_t bus, uint8_t slot){
	uint64_t hba_mem;
	hba_mem_t *hba_mem_struct;

	bringDownMemory(bus, slot, 0, 0x24);

	hba_mem = ((uint64_t)(((uint32_t)((pciConfigReadWord(bus, slot, 0, 0x26) & 0xFFFF) << 16)) | ((uint32_t)(pciConfigReadWord(bus, slot, 0, 0x24) & 0xFFFF)))) & 0xFFFFFFFF;
	hba_mem_struct = (hba_mem_t*) hba_mem;

	probePort(hba_mem_struct);

	kprintf("Control back\n", hba_mem_struct->pi);
}

void checkalldevices(){
	uint16_t classcode, subclass, progif;
	int bus, slot;	
	for (bus = 0; bus < 255; bus++){
		for(slot = 0; slot < 32; slot++){
			if (getVendor(bus, slot) != 0xFFFF){
				classcode = ((pciConfigReadWord(bus, slot, 0, 10)&0xFF00) >> 8);
				if(classcode == 0x01){
					subclass = (pciConfigReadWord(bus, slot, 0, 10)&0x00FF);
					if(subclass == 0x06){
						progif = ((pciConfigReadWord(bus, slot, 0, 8)&0xFF00)>>8);
						if(progif == 0x01){
							getFirstDevice(bus, slot);
						}
					}
				} 
			}
		}
	}
}
