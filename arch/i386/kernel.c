#include <i386/isr.h>
#include <i386/a20.h>
#include <i386/8042.h>
#include <i386/8253.h>
#include <i386/8259.h>
#include <i386/kheap.h>
#include <i386/paging.h>
#include <i386/vga.h>
#include <i386/panic.h>
#include <i386/gfx.h>
#include <i386/fs.h>
#include <i386/initrd.h>
#include <i386/elf.h>
#include <multiboot2.h>
#include <stdbool.h>

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

extern uint32_t x86_kernel_start, x86_kernel_end, x86_placement_address, x86_kernel_code;
uint32_t x86_ramsize, x86_initial_esp, x86_initrd_size, x86_initrd_start, x86_initrd_end;
uint32_t x86_memory_location, x86_ramstart, x86_memory_end_location, x86_total_frames;
bool modules_exist = false;
int x86_memory_amount, x86_usable_mem;

#define ROUNDUP(x, y) (x % y ? x + (y - (x % y)) : x)

struct multiboot_tag_mmap *tag_mmap;
elf_t kernel_elf;

void init(unsigned long magic, uint32_t addr, unsigned int initial_boot_stack)
{
	int is_initrd_loaded = 0;
	x86_initial_esp = initial_boot_stack;
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
	{
		__asm__ __volatile__("cli");
		__asm__ __volatile__("hlt");
		return;
	}
	init_text_mode();

	struct multiboot_tag *tag;
	unsigned size;

	size = *(unsigned *)addr;
	// printk("Announced mbi size 0x%x\n", size);
	for (tag = (struct multiboot_tag *)(addr + 8);
		 tag->type != MULTIBOOT_TAG_TYPE_END;
		 tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
	{
		// printk("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
		switch (tag->type)
		{
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			// printk("Command line = %s\n",
			// 	   ((struct multiboot_tag_string *)tag)->string);
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			// printk("Boot loader name = %s\n",
			// 	   ((struct multiboot_tag_string *)tag)->string);
			break;
		case MULTIBOOT_TAG_TYPE_MODULE:
			if (!strcmp(((struct multiboot_tag_module *)tag)->cmdline, "initrd"))
			{
				is_initrd_loaded = 1;
				x86_initrd_start = ((struct multiboot_tag_module *)tag)->mod_start;
				x86_initrd_end = ((struct multiboot_tag_module *)tag)->mod_end;
				x86_initrd_size = x86_initrd_end - x86_initrd_start;
			}
			break;
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
			x86_ramsize = (((struct multiboot_tag_basic_meminfo *)tag)->mem_upper) / 1024 + 2;
			break;
		case MULTIBOOT_TAG_TYPE_MMAP:
		{
			tag_mmap = (struct multiboot_tag_mmap *)tag;
			multiboot_memory_map_t *mmap;
			for (mmap = ((struct multiboot_tag_mmap *)tag)->entries;
				 (multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size;
				 mmap = (multiboot_memory_map_t *)((unsigned long)mmap + ((struct multiboot_tag_mmap *)tag)->entry_size))
    {
		if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->len > 0x100000)
		{
			x86_memory_location = mmap->addr;
			x86_memory_amount = mmap->len;
		}
			}
		}
		break;
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
			kernel_elf = elf_from_multiboot((struct multiboot_tag_elf_sections *)tag);
			break;
		}
    }

    x86_placement_address = x86_initrd_end;

    x86_ramstart = x86_memory_location;
    x86_memory_end_location = x86_memory_location + x86_memory_amount;
	x86_usable_mem = x86_memory_end_location - x86_memory_location;
    
    // Second method for getting total usable memory
    //			  Bytes             KiB    MiB
    // We add up 2 MiB to round up the memory calculation
	x86_ramsize = (x86_usable_mem / 1024 / 1024) + 2;
    
    x86_total_frames = x86_usable_mem / 0x1000;

    // GRUB enables A20 for us
	init_a20();	
	init_gdt();
	init_idt();
	// setup_sse();
	pit_init();
	initialize_paging(x86_total_frames, 0, 0);
	malloc_stats();
	init_8042_keyboard();

	if (is_initrd_loaded)
	{
		modules_exist = true;
		uint32_t x86_initrd_location = x86_initrd_start;
		fs_root = initialise_initrd(x86_initrd_location);
	}
	else
	{
		printk("No Modules Found!\n");
	}

	struct multiboot_tag_mmap *tmp = kmalloc(tag_mmap->size);
	memcpy(tmp, tag_mmap, tag_mmap->size);
	tag_mmap = tmp;

	shell();

	/* XXX: Kernel can't reach this zone,
	if it does, throw a Kernel Panic Error */
	PANIC("Reached Kernel End!");
	while(1);
}