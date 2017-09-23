uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;
	uint32_t lbus = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)( (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000) );

	__asm__ __volatile__ ("outl %0, %1" : : "a"(address), "Nd"(0xCF8));

	uint32_t ret;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (ret) : "Nd" (0xCFC));

	ret = (ret >> ((offset & 2) * 8) & 0xffff);

	return ret;
}
