#include "kheap.h"
#include "paging.h"
#include <panic.h>
#include <stdint.h>
#include <types.h>

#define INITIAL_HEAP_SIZE (0x1000 * INITIAL_HEAP_PAGE_COUNT)

struct header
{
	uint8_t free;
	uint32_t size;
};

struct free_header
{
	struct header h;
	struct free_header *next;
	struct free_header *prev;
};

struct footer
{
	uint32_t size;
};

char *memoryhead;
char *memoryend; // 'memoryend' must always be page aligned.

struct free_header *head;

uint32_t allocations = 0;
uint32_t heap_free = 0;

static void do_kfree(void *p);

void malloc_stats()
{

#ifdef DBG_HEAP
	printf("Heap starts @ 0x%x ends @ 0x%x and contains ", memoryhead, memoryend);
#endif

	uint32_t bytes = memoryend - memoryhead;

#ifdef DBG_HEAP
	if(bytes / 1024 / 1024 > 0)
	{
		printf("%d MiB.\nCurrent allocations: [%d]\n", bytes / 1024 / 1024, allocations);
	}
	else if(bytes / 1024 > 0)
	{
		printf("%d KiB.\nCurrent allocations: [%d]\n", bytes / 1024, allocations);
	}
	else
	{
		printf("%d B.\nCurrent allocations: [%d]\n", bytes, allocations);
	}
#endif

	uint32_t free = 0;
	struct free_header *current = head;
	while(current)
	{
		free += current->h.size;
		current = current->next;
	}

#ifdef DBG_HEAP
	if(free / 1024 / 1024 > 0)
	{
		printf("Free: [%d MiB]\n", free / 1024 / 1024);
	}
	else if(free / 1024 > 0)
	{
		printf("Free: [%d KiB]\n", free / 1024);
	
	else
	{
		printf("Free: [%d Bytes]\n", free);
	}
#endif
}

static struct header *set_header_footer(char *block, uint32_t block_size)
{
	struct header *header = (struct header *)block;
	header->free = 1;
	header->size = block_size;
	struct footer *foot = (struct footer *)(block + block_size - sizeof (struct footer));
	foot->size = block_size;

	/* TODO: Find a less nasty solution. */
	// If we map pages above the end of RAM, we panic about heap corruption.
	if(foot->size != block_size)
	{
		PANIC("Out of Memory: Tried to map pages above RAM");
	}

	return header;
}

static void expand(uint32_t at_least)
{
	uint32_t size = 0x2000; // Expand two pages as minimum.

	if(at_least > size)
	{
		// If at_least occupies more than two pages,
		// map enough frames to supply it.
		size = (at_least & 0xFFFFF000) + 0x1000;
	}

	// Number of frames we're going to map.
	uint32_t num_frames = size / 0x1000;
	char *block_header = memoryend;

	uint32_t i;
	// If we're mapping more than one frame, we may end up with fewer than we asked for.
	// We still want to insert them into the heap and try an allocation.
	uint32_t actually_allocated = 0;

	for(i = 0; i < num_frames; i++)
	{
		struct page *page = map_kernel_page((uint32_t)memoryend, 0);

		if(page == NULL)
		{
			// We failed to grab a page-table entry for the desired page.
			break;
		}
		
		memoryend += 0x1000;
		actually_allocated += 0x1000;
	}

	if(actually_allocated == 0)
	{
		// If we didn't manage to map anything, just return.
		return;
	}

	set_header_footer((char *)block_header, actually_allocated);
	do_kfree(block_header + sizeof (struct header));
}

void initialize_kheap(uint32_t start_addr)
{
	memoryhead = (char *)start_addr;
	memoryend = memoryhead + INITIAL_HEAP_SIZE;
	set_header_footer(memoryhead, INITIAL_HEAP_SIZE);
	head = (struct free_header *)memoryhead;
	head->next = NULL;
	head->prev = NULL;
	heap_free = head->h.size;
}

static struct free_header *find_block_with_page_aligned_addr(uint32_t needed_size)
{
	struct free_header *block = head;
	while(block != NULL)
	{
		char *start_addr = (char *)block;
		char *end_addr = (char *)(start_addr + block->h.size);
		uint32_t next_page = (((uint32_t)block) & 0xFFFFF000) + 0x1000;

		// Move the blockstart back by the size of the header. The
		// address we RETURN has to be page-aligned.
		char *blockstart = (char *)(next_page - sizeof (struct header));

		if(blockstart < (start_addr + block->h.size - sizeof (struct footer)))
		{
			// page-aligned address is within block bounds!
			// Check if there is enough space.
			uint32_t total_space = end_addr - blockstart;
			
			if(total_space >= needed_size)
			{
				// WE FOUND ONE!
				if(blockstart != start_addr)
				{
					// Now we need to break up the blocks.
					if((uint32_t)(blockstart - start_addr) < (uint32_t)(sizeof(struct header) + sizeof(struct footer)))
					{
						// There's not enough room before blockstart to split the blocks.
						// Try to find another.
						continue;
					}
					else
					{
						// There's enough room. Break up the block.
						uint32_t space_in_prev_block = blockstart - start_addr;
						set_header_footer((char *)block, space_in_prev_block);
						struct free_header *newblock = (struct free_header *)blockstart;
						set_header_footer((char *)newblock, total_space);
						newblock->next = block->next;
						
						if(newblock->next)
						{
							newblock->next->prev = newblock;
						}

						block->next = newblock;
						newblock->prev = block;
						// We've broken up the block. Return the new one!
						return newblock;
					}
				}
				else
				{
					// The block is already page-aligned. Just return it.
					return  block;
				}
			}
		}
	
		block = block->next;
	}
		return NULL;
}

// Find a free block of suitable size and alignment.
static struct free_header *find_block(uint32_t size, uint8_t align)
{
	struct free_header *block = head;
	while(block != NULL)
	{
		// If we're not looking for alignment, Just check size:
		if(!align)
		{
			if(block->h.size >= size)
			{
				break;
			}
		}
		else
		{
			// We need an aligned block.
			block = find_block_with_page_aligned_addr(size);
			break;
		}
		
		block = block->next;
	}
	
	return block;
}

void *kmalloc(uint32_t size)
{
	return kmalloc_ap(size, 0, NULL);
}

void *krealloc(void *p, uint32_t size)
{
	if(p == NULL)
	{
		return kmalloc(size);
	}
	
	struct header *header = (struct header *)((char *)p) - sizeof (struct header);
	void *newchunk = kmalloc(size);
	
	if(newchunk == NULL) return NULL; // Check if we actually malloc'ed newchunk.

	memcpy(newchunk, p, header->size);
	kfree(p);
	return newchunk;
}

void *kmalloc_ap(uint32_t size, uint8_t align, uint32_t *phys)
{
	if(size <= 0)
	{
		return NULL;
	}

	if(memoryhead == NULL)
	{
		PANIC("kheap: not initialized");
	}

	// The block must be large enough to fit our pruposes.
	uint32_t needed_size = size + sizeof (struct free_header) + sizeof (struct footer);

#ifdef DBG_HEAP
	printf("Allocating %d Byte(s)\n", needed_size);
#endif

	// Find a free block.
	struct free_header *block = find_block(needed_size, align);

	if(block == NULL)
	{
		// We couldn't find a suitable block.
		// Try to expand the heap
		expand(needed_size);

		// Try to find a block again, now that we've
		// hopefully expanded.
		block = find_block(needed_size, align);

		if(block == NULL)
		{
			// If we still can't find a suitable block, PANIC!
			PANIC("Failed to expand kheap.");
		}

	}
	
	// Now we have found the block we are going to use!
	block->h.free = 0;

	if(head == block)
	{
		head = block->next;
	}

	if(block->prev)
	{
		block->prev->next = block->next;
	}

	if(block->next)
	{
		block->next->prev = block->prev;
	}

	if(block->h.size - needed_size < sizeof (struct free_header) + sizeof (struct footer))
	{
		// Not enough room left in the remaining block to put a free entry. Just include the
		// rest of the space in the alloc'd block.
		needed_size = block->h.size;
		
	}
	else
	{
		// Chop up the block and insert the remainder into the free list.

		// Thse may get clobbered when writing the remainder header.
		// Save them now so we can use them to insert the remainder.
		void *blocknext = block->next;
		void *blockprev = block->prev;

		struct free_header *remainder_block = (struct free_header *)(((char *) block) + needed_size);
		set_header_footer((char *)remainder_block, block->h.size - needed_size);

		remainder_block->next = blocknext;
		
		if(remainder_block->next)
		{
			remainder_block->next->prev = remainder_block;
		}

		remainder_block->prev = blockprev;

		if(remainder_block->prev)
		{
			remainder_block->prev->next = remainder_block;
		}
		else
		{
			// Blockprev was head. Set head to remainder.
			head = remainder_block;
		}
	}

	block->h.size = needed_size;
	struct footer *block_footer = (struct footer *)((char *)block + needed_size - sizeof (struct footer));
	block_footer->size = needed_size;

	// We have oficcially made an allocation.
	allocations++;
	heap_free -= block->h.size;

	char *return_addr = ((char *)block) + sizeof (struct header);
	
	if(phys)
	{
		struct page *p = get_kernel_page((uint32_t)return_addr, 0);
		*phys = (p->frame << 12) | (((uint32_t)return_addr) & 0xFFF);
	}

	return return_addr;
}

static struct free_header *get_previous_block(struct header *block_head)
{
	char *block = (char *)block_head;

	if(block == memoryhead)
	{
		/* Heap location is at the first available location,
		there's no free block. */
		return NULL;
	}

	struct footer *prev_foot = (struct footer *)(block - sizeof (struct footer));
	struct header *prev_head = (struct header *)(block - prev_foot->size);
		
	if(prev_head == block_head)
	{
		// header/footer sizes can never be the zero.
		PANIC("kheap corruption detected");
	}
		
	return (struct free_header *)prev_head;
}

static struct free_header *get_next_block(struct header *block_head)
{
	char *block = (char *)block_head;
	void *nextblock = block + block_head->size;

	if(nextblock == memoryend)
	{
		return NULL;
	}
		
	return nextblock;
}

static void do_kfree(void *p)
{
	if(p == NULL)
	{
		return;
	}

	struct free_header *block_header = (struct free_header *)(((char *)p) - sizeof (struct header));
	block_header->h.free = 1;
	heap_free += block_header->h.size;

	struct free_header *prev_block = get_previous_block((struct header *)block_header);
	struct free_header *next_block = get_next_block((struct header *)block_header);

	// Try to join the prev block first.
	if(prev_block && prev_block->h.free)
	{
		// If the previous block is free, just add our size to it.
		set_header_footer((char *)prev_block, prev_block->h.size + block_header->h.size);

		// If the next block is also free, we can join it simply.
		if(next_block && next_block->h.free)
		{
			prev_block->next = next_block->next;

			if(prev_block->next)
			{
				prev_block->next->prev = prev_block;
			}
			
			set_header_footer((char *)prev_block, prev_block->h.size + next_block->h.size);
		}
	
		return;
	}

	// Join both blocks.
	if(next_block && next_block->h.free)
	{
		block_header->next = next_block->next;

		if(block_header->next)
		{
			block_header->next->prev = block_header;
		}

		block_header->prev = next_block->prev;

		if(block_header->prev)
		{
			block_header->prev->next = block_header;
		}

		set_header_footer((char *)block_header, block_header->h.size + next_block->h.size);
				
		if(head == next_block)
		{
			head = block_header;
			block_header->prev = NULL;
		}

		return;
	}

	/* Blocks on both sides are used. Look backwards for closest free block.
	   We couldn't get pointers from either the previous or the next block since neither were free.
	   We won't be merging with any other blocks. We just want to find the closest previous block
	   So we can insert the block into the free list.
	   First check if there even ARE any free blocks before this one.
	   This is an optimization to avoid scanning backwards through
	   The entire heap if we don't need to. */
	if(head == NULL || head > block_header)
	{
				// No blocks before prev_block.
				prev_block = NULL;
	}

	while(prev_block)
	{
		if(prev_block->h.free)
		{
			break;
		}

		prev_block = get_previous_block((struct header *)prev_block);
	}

	if(prev_block == NULL)
	{
		// Add the block to the top of the heap.
		block_header->next = head;
		head = block_header;

		if(block_header->next)
		{
			block_header->next->prev = block_header;
		}
				
				block_header->prev = NULL;
		}
		else
		{
			// Add the block after prev_block.
			block_header->next = prev_block->next;
				
			if(block_header->next)
			{
				block_header->next->prev = block_header;
			}
				
			block_header->prev = prev_block;
			prev_block->next = block_header;
		}

	set_header_footer((char *)block_header, block_header->h.size);
}

// Function to unmap the last block positioned on the heap.
static int unmap_blocks()
{
	// Check if last block is a page's size large.
	struct footer *f = (struct footer *)(memoryend - sizeof (struct footer));

	if(f->size < 0x1000)
	{
		// If block isn't a page's size large, don't unmap it.
		return 0;
	}

	struct header *h = (struct header *)(memoryend - f->size);
		
	if(!h->free)
	{
		// Unfreed block, can't be unmapped.
		return 0;
	}

	char *page_addr = memoryend - 0x1000;
		
	if(page_addr < (char *)h
		|| (uint32_t)(page_addr - (char *)h) < (sizeof (struct free_header) + sizeof (struct footer)))
	{
		// No space left if we free the page.
		return 0;
	}

	// A page can be freed.
	uint32_t newsize = h->size - 0x1000;
	memoryend -= 0x1000;
	heap_free -= 0x1000;
	set_header_footer((char *)h, newsize);
	unmap_kernel_page((uint32_t)memoryend);
	// Return 1 if succeeded.
	return 1;
}

void kfree(void *p)
{
	do_kfree(p);
	allocations--;
	while(unmap_blocks()){}
}