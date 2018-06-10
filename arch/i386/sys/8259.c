#include <i386/8259.h>
#include <i386/vga.h>
#include <i386/system.h>
#include <stdint.h>

void pic_init(void)
{
#ifdef DBG_PIT
    printk("Starting PIT initialization...\n");
#endif
    outb(PIC_MASTER_CMD, 0x10 + 0x01);
    outb(PIC_SLAVE_CMD,  0x10 + 0x01);
    outb(PIC_MASTER_DATA, 0x20);
    outb(PIC_SLAVE_DATA, 0x28);
    outb(PIC_MASTER_DATA, 4);
    outb(PIC_SLAVE_DATA, 2);
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA, 0x01);
    outb(PIC_MASTER_DATA, 0);
    outb(PIC_SLAVE_DATA, 0);
#ifdef DBG_PIT
    printk("Started PIT successfully...\n");
#endif
    printkok("Initialized PIC (8259)");
}

void pic_send_eoi(uint8_t irq)
{
    if(irq >= 8)
        outb(PIC_SLAVE_CMD, PIC_CMD_EOI);
    outb(PIC_MASTER_CMD, PIC_CMD_EOI);
}