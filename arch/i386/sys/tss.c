#include <i386/tss.h>
#include <i386/gdt.h>
#include <string.h>

struct tss_entry_struct
{
   uint32_t prev_tss;
   uint32_t esp0;
   uint32_t ss0;
   uint32_t esp1;
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;
   uint32_t cs;
   uint32_t ss;
   uint32_t ds;
   uint32_t fs;
   uint32_t gs;
   uint32_t ldt;
   uint8_t trap;
   uint8_t iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

tss_entry_t tss_entry;

void write_tss(int32_t num, uint16_t ss0, uint32_t esp0)
{
   uint32_t base = (uint32_t) &tss_entry;
   uint32_t limit = base + sizeof(tss_entry);
   gdt_set_gate(num, base, limit, 0xE9, 0x00);
   memset(&tss_entry, 0, sizeof(tss_entry));
   tss_entry.ss0  = ss0;
   tss_entry.esp0 = esp0;
   tss_entry.cs   = 0x0b;
   tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}

void set_kernel_stack(uint32_t stack)
{
    tss_entry.esp0 = stack;
}