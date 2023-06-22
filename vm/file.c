/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include <string.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/process.h"
#include "round.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
	// not lazy loading

	// if((uint64_t)addr % PGSIZE)
	// 	return NULL;
	
	// file_seek(file, offset);

	// int pg_cnt = 0;
	// off_t read_bytes = 0;
	// void *va = NULL;
	// struct page *page;

	// while(length > pg_cnt * PGSIZE){
	// 	va = (uint64_t)addr + pg_cnt * PGSIZE;

	// 	if(spt_find_page(&thread_current()->spt, va) == NULL){
	// 		if(vm_alloc_page(VM_FILE, va, writable) == NULL)
	// 			return NULL;
	// 	}
	// 	else
	// 		return NULL;

	// 	page = spt_find_page(&thread_current()->spt, va);
	// 	vm_claim_page(page->va);
	// 	pg_cnt++;

	// 	read_bytes = file_read(file, page->frame->kva, PGSIZE);

	// 	if(read_bytes < PGSIZE)
	// 		break;
	// }

	// memset((uint64_t)page->frame->kva + read_bytes, 0, PGSIZE - read_bytes);
	// file_seek(file, offset);
	// return page->frame->kva;

	// lazy loading
	uint64_t va = addr;
    file = file_reopen(file);

	while (length > 0) {
		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
        struct file_segment *file_segment = malloc(sizeof(struct file_segment));
        file_segment->file = malloc(sizeof(struct file));

		memcpy(file_segment->file, file, sizeof(struct file));

		file_segment->page_read_bytes = page_read_bytes;
		file_segment->ofs = offset;

        if(!vm_alloc_page_with_initializer(VM_FILE, va, writable, lazy_load_segment, file_segment)) {
			return NULL;
		}

        length -= page_read_bytes;
        offset += page_read_bytes;

        va += PGSIZE;
	}

    return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	
}
