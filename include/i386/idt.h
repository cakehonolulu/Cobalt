#include <stdint.h>

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

/* cake: Function declared on another file
   to load our IDT table */
extern void idt_flush();

void idt_set_gate(uint8_t num, size_t base, unsigned short sel, uint8_t flags);
void init_idt(void);