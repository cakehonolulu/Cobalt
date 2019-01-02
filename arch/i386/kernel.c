#include <i386/isr.h>
#include <i386/shell.h>
#include <i386/a20.h>
#include <i386/8042.h>
#include <i386/8253.h>
#include <i386/8259.h>
#include <i386/kheap.h>
#include <i386/gdt.h>
#include <i386/paging.h>
#include <i386/idt.h>
#include <i386/irq.h>
#include <i386/vga.h>
#include <i386/panic.h>
#include <i386/tss.h>
#include <i386/gfx.h>
#include <i386/fs.h>
#include <i386/initrd.h>
#include <i386/elf.h>
#include <multiboot.h>
#include <stdbool.h>

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

extern uint32_t x86_kernel_start, x86_kernel_end, x86_placement_address, x86_kernel_code;
uint32_t x86_ramsize, x86_initial_esp, x86_initrd_size, x86_initrd_start, x86_initrd_end;
uint32_t x86_memory_location, x86_ramstart, x86_memory_end_location, x86_total_frames;
bool modules_exist = false;
int x86_memory_amount, x86_usable_mem;

#define ROUNDUP(x, y) (x % y ? x + (y - (x % y)) : x)

multiboot_info_t *mb;
elf_t kernel_elf;

void init(unsigned long magic, multiboot_info_t *mbi, unsigned int initial_boot_stack)
{
	//x86_initial_esp = initial_boot_stack;
	//*mb = *mbi;
	/*if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		__asm__ __volatile__ ("cli");
		__asm__ __volatile__ ("hlt");
		return;
	}*/

	//x86_initrd_start = *(uint32_t*)(mbi->mods_addr);
	//x86_initrd_end = *(uint32_t*)(mbi->mods_addr+4);
	//x86_initrd_size = x86_initrd_end - x86_initrd_start;

    //multiboot_memory_map_t* mmap = mbi->mmap_addr;
    //x86_ramsize = mbi->mem_upper / 1024 + 2;

    init_text_mode();

    /*while(mmap < mbi->mmap_addr + mbi->mmap_length)
    {
		if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->len > 0x100000)
		{
			x86_memory_location = mmap->addr;
			x86_memory_amount = mmap->len;
		}

        mmap = (multiboot_memory_map_t*) ( (unsigned int)mmap + mmap->size + sizeof(mmap->size) );
    }

    x86_placement_address = x86_initrd_end;

    x86_ramstart = x86_memory_location;
    x86_memory_end_location = x86_memory_location + x86_memory_amount;
    x86_usable_mem = x86_memory_end_location - x86_memory_location;*/
    
    // Second method for getting total usable memory
    //			  Bytes             KiB    MiB
    // We add up 2 MiB to round up the memory calculation
    /*x86_ramsize = (x86_usable_mem / 1024 / 1024) + 2;
    
    x86_total_frames = x86_usable_mem / 0x1000;

    kernel_elf = elf_from_multiboot(mbi);*/

    // GRUB enables A20 for us
	init_a20();	
	init_gdt();
	init_idt();
	setup_sse();
	pit_init();
	//initialize_paging(x86_total_frames, 0, 0);
	//malloc_stats();
	init_8042_keyboard();

 	/*if (mbi->mods_count > 0)
	{
		modules_exist = true;
		uint32_t x86_initrd_location = x86_initrd_start;
		fs_root = initialise_initrd(x86_initrd_location);
	}
	else
	{
		printk("No Modules Found!\n");
	}*/

	shell();

	/* XXX: Kernel can't reach this zone,
	if it does, throw a Kernel Panic Error */
	PANIC("Reached Kernel End!");
	while(1);
}