#include <sys/defs.h>
#include <sys/kprintf.h>

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;
	uint32_t lbus = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint16_t temp_addr;

	address = (uint32_t)( (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000) );

	__asm__ __volatile__ ("outl %0, %w1" : : "a"(address), "Nd"(0xCF80000));

	uint32_t ret;
	__asm__ __volatile__ ("inl %w1, %0" : "=a" (ret) : "Nd" (0xCFC0000));

	temp_addr = (int16_t)(ret >> ((offset & 2) * 8) & 0xffff);

	return temp_addr;
}

uint16_t getVendor(uint8_t bus, uint8_t slot){
	return pciConfigReadWord(bus, slot, 0, 0);
}

void checkalldevices(){
	uint8_t classcode;
		classcode = (uint8_t)(pciConfigReadWord(2, 3, 0, 10) >> 8);
		kprintf("%d\n", classcode);
	
	for (int bus = 0; bus < 256; bus++){
		for(int slot = 0; slot < 32; slot++){
			if (getVendor(bus, slot) != 0xFFFF){
				classcode = (uint8_t)(pciConfigReadWord(bus, slot, 0, 10) >> 8);
				kprintf("%d\n", classcode);
			}
		}
	}
}
