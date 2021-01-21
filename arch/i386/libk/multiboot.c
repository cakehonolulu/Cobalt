#include <multiboot2.h>
#include <stdint.h>

const char* mem_type_names[] = {"", "Available", "Reserved", "ACPI", "NVS", "Bad RAM"};

void get_memory_map(struct multiboot_tag_mmap *tag_mmap)
{
    multiboot_memory_map_t *mmap;
	for (mmap = tag_mmap->entries;
		(multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag_mmap + tag_mmap->size;
		mmap = (multiboot_memory_map_t *)((unsigned long)mmap + tag_mmap->entry_size))
	{
		uint64_t end = mmap->addr + mmap->len;
		if (end >> 32 == 0) {
			printk("0x%x - "
					"0x%x | %s\n",
					(unsigned)(mmap->addr & 0xffffffff),
					(unsigned)(end & 0xffffffff),
					mem_type_names[mmap->type]);
		} else {
			printk("0x%x_%x - "
					"0x%x_%x | %s\n",
					(unsigned)(mmap->addr >> 32),
					(unsigned)(mmap->addr & 0xffffffff),
					(unsigned)(end >> 32),
					(unsigned)(end & 0xffffffff),
					mem_type_names[mmap->type]);
		}
	}

}