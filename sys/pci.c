#include <sys/defs.h>
#include <sys/kprintf.h>

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

void getFirstDevice(uint8_t bus, uint8_t slot){
	uint32_t hba_mem;

	hba_mem = ((uint32_t)((pciConfigReadWord(bus, slot, 0, 0x26) & 0xFFFF) << 16)) | ((uint32_t)(pciConfigReadWord(bus, slot, 0, 0x24) & 0xFFFF));

	kprintf("%d\n", *hba_mem);
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

