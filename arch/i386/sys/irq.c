#include <i386/irq.h>
#include <i386/system.h>
#include <i386/regs.h>

void *irq_routines[16] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

void x86_register_interrupt_handler(unsigned char n, void (*handler))
{
	__asm__ __volatile__ ("cli");
    irq_routines[n] = handler;
	__asm__ __volatile__ ("sti");
}

void x86_unregister_interrupt_handler(unsigned char n)
{
	__asm__ __volatile__ ("cli");
    irq_routines[n] = 0;
    __asm__ __volatile__ ("sti");
}

void init_irq(void)
{
    idt_set_gate(32, (unsigned)irq0, 0x08, 0x8E);
    idt_set_gate(33, (unsigned)irq1, 0x08, 0x8E);
    idt_set_gate(34, (unsigned)irq2, 0x08, 0x8E);
    idt_set_gate(35, (unsigned)irq3, 0x08, 0x8E);
    idt_set_gate(36, (unsigned)irq4, 0x08, 0x8E);
    idt_set_gate(37, (unsigned)irq5, 0x08, 0x8E);
    idt_set_gate(38, (unsigned)irq6, 0x08, 0x8E);
    idt_set_gate(39, (unsigned)irq7, 0x08, 0x8E);
    idt_set_gate(40, (unsigned)irq8, 0x08, 0x8E);
    idt_set_gate(41, (unsigned)irq9, 0x08, 0x8E);
    idt_set_gate(42, (unsigned)irq10, 0x08, 0x8E);
    idt_set_gate(43, (unsigned)irq11, 0x08, 0x8E);
    idt_set_gate(44, (unsigned)irq12, 0x08, 0x8E);
    idt_set_gate(45, (unsigned)irq13, 0x08, 0x8E);
    idt_set_gate(46, (unsigned)irq14, 0x08, 0x8E);
    idt_set_gate(47, (unsigned)irq15, 0x08, 0x8E);
}

void x86_irq_handler(struct regs *r)
{
    void (*handler)(struct regs *r);

    handler = irq_routines[r->int_no - 32];

    if (handler)
    {
        handler(r);
    }

    if (r->int_no >= 32)
    {
        unsigned char irq = r->int_no - 32;
        pic_send_eoi(irq);
    }
}