#include <i386/idt.h>
#include <i386/vga.h>
#include <i386/irq.h>
#include <i386/isr.h>
#include <string.h>

/* cake: We use __attribute__((packed))
   in order to prevent the compiler
   from optimizing the code in any way */

/* IDT scheme */
struct idt_entry
{
    unsigned short base_lo;
    unsigned short sel;
    uint8_t always0;
    uint8_t flags;
    unsigned short base_hi;
} __attribute__((packed));

/* IDT's pointer scheme */
struct idt_ptr
{
    unsigned short limit;
    uint16_t base;
} __attribute__((packed));

/* IDT structure */
struct idt_entry idt[256];

/* IDT pointer */
struct idt_ptr idtp;

/* Function to craft a gate for the Interrupt Descriptor Table */
void idt_set_gate(uint8_t num, size_t base, unsigned short sel, uint8_t flags)
{
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    /* cakeh: When we finally implement Ring 3 jumping
       (Usermode), we need to add an 'OR' on the flag
       of every IDT Gate to ensure we don't get a
       General Protection Fault, Example Code:
       idt[num].flags = flags | 0x60; */
    idt[num].flags = flags;
}

/* Called on architecture init.
   Setups the IDT, initializes the ISR's
   and IRQ's within and finally enables
   interrupts */
void init_idt(void)
{
    /* Setup IDT limit */
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;

    /* Setup IDT pointer */
    idtp.base = (int) &idt;

    /* Clear the IDT by zeroing out it */
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    /* Init ISR's */
    init_isr();
    printkok("Initialized ISRs");

    /* Remap the PIC */
    pic_remap();
    printkok("Remapped the PIC");

    /* Init IRQ's */
    init_irq();
    printkok("Initialized IRQs");

    /* Replace old IDT with the new one by 
       flushing all the changes */
    idt_flush((uint32_t)&idtp);
    printkok("Initialized IDT");
    
    /* Enable Interrupts */
    __asm__ __volatile__ ("sti");
    printkok("Enabled Interrupts");
}