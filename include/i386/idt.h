#include <stdint.h>

/* cake: Function declared on another file
   to load our IDT table */
extern void idt_flush();

void idt_set_gate(uint8_t num, size_t base, unsigned short sel, uint8_t flags);
void init_idt(void);