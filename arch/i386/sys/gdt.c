#include <i386/gdt.h>
#include <i386/vga.h>

/* GDT structure */
struct gdt_entry gdt[6];

/* GDT pointer */
struct gdt_ptr gp;

/* Function to craft a descriptor for the Global Descriptor Table */
void gdt_set_gate(int num, size_t base, size_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

/* Called on architecture init.
   Setups the first 3 GDT entries
   and switches the resulting crafted
   GDT to the processor and updates
   the segment registers */
void init_gdt(void)
{
	/* Setup GDT limit */
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;

    /* Setup GDT pointer */
    gp.base = (uint32_t) &gdt;

    /* GDT Null Descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* GDT Code Segment */
    // Segment Type: Code
    // Base adress: 0
    // High adress: 4 Gigabytes
    // Granularity: 4 Kilobyte
    // Opcode: 32-bit
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* GDT Data Segment */
    // Segment Type: Data
    // Rest is same as Code Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* GDT Code Segment */
    // Segment Type: Code
    gdt_set_gate(3, 0, 0xffffffff, 0xFA, 0xCF);

    /* GDT Code Segment */
    // Segment Type: Code
    gdt_set_gate(4, 0, 0xffffffff, 0xF2, 0xCF);

    /* Write TSS Info on the GDT Table */
    write_tss(5, 0x10, 0x0);

    /* Replace old GDT with the new one by 
       flushing all the changes */
    gdt_flush((uint32_t)&gp);
    printkok("Initialized GDT");

    /* Flush Task State Segment */
    tss_flush();
    printkok("Initialized TSS");
}