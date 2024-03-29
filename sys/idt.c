#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/idt.h>

static void init_idt();
static void idt_set_gate(uint8_t, uint64_t, uint16_t, uint8_t);

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

void isr0(){
    kprintf("Hi\n");
}

void isr32(){
    kprintf("-----one\n");
    // outb(0x20,0x20);
    // __asm__ __volatile__ ("outb %0, %1" : : "a"(0x20), "Nd"(0x20));
    __asm__("iretq");
}

void isr33(){
    kprintf("two\n");
}

void init_descriptor_tables()
{
    init_idt();
}

static void init_idt()
{
    idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
    idt_ptr.base  = (uint64_t)&idt_entries;

//    memset(&idt_entries, 0, sizeof(idt_entry_t)*256);
   for(int i=0;i<32;i++){
    idt_set_gate( i, (uint64_t)isr0 , 0x08, 0x8E);
    }
    idt_set_gate(32, (uint64_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)isr33, 0x08, 0x8E);
    for(int i = 34;i<256;i++){
   	idt_set_gate( i, (uint64_t)isr0 , 0x08, 0x8E);
	 }
/*    idt_set_gate( 1, (uint32_t)isr1 , 0x08, 0x8E);
    idt_set_gate( 2, (uint32_t)isr2 , 0x08, 0x8E);
    idt_set_gate( 3, (uint32_t)isr3 , 0x08, 0x8E);
    idt_set_gate( 4, (uint32_t)isr4 , 0x08, 0x8E);
    idt_set_gate( 5, (uint32_t)isr5 , 0x08, 0x8E);
    idt_set_gate( 6, (uint32_t)isr6 , 0x08, 0x8E);
    idt_set_gate( 7, (uint32_t)isr7 , 0x08, 0x8E);
    idt_set_gate( 8, (uint32_t)isr8 , 0x08, 0x8E);
    idt_set_gate( 9, (uint32_t)isr9 , 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
*/
    //idt_flush((uint32_t)&idt_ptr);
    __asm__ ("lidt %0"::"m"(idt_ptr));
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t typeAttr)
{
    idt_entries[num].offset_low = base & 0xFFFF;
    idt_entries[num].offset_mid = (base >> 16) & 0xFFFF;
    idt_entries[num].offset_high = (base >> 32) & 0xFFFFFFFF;

    idt_entries[num].sel = sel;
    idt_entries[num].zero0 = 0;
    idt_entries[num].zero1 = 0;
    idt_entries[num].typeAttr = typeAttr;
}

