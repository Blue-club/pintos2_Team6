/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

/* Project 3. */
#include <string.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
/* Project 3. */

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

/* Project 3. */
/* Do the mmap in Lazy loading. */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	if ((uint64_t)addr % PGSIZE) {
		return NULL;
	}

	struct supplemental_page_table *spt = &thread_current ()->spt;
	int pg_cnt = 0;
	off_t read_bytes = 0;
	void *upage = NULL;
	struct page *page;
	
	file_seek (file, offset);
	while (length > pg_cnt * PGSIZE) {
		upage = (uint64_t)addr + pg_cnt * PGSIZE;

		if (spt_find_page (spt, upage) != NULL) {
			return NULL;
		}
		if (vm_alloc_page (VM_FILE, upage, writable) == NULL) {
			return NULL;
		}

		page = spt_find_page (spt, upage);
		if (page == NULL || !vm_claim_page (page->va)) {
			return NULL;
		}
		pg_cnt++;

		read_bytes = file_read (file, page->frame->kva, PGSIZE);

		if (read_bytes < PGSIZE) {
			break;
		}
	}
	memset ((uint64_t)page->frame->kva + read_bytes, 0, PGSIZE - read_bytes);
	
	file_seek (file, offset);
	return upage;
}

/* Do the munmap */
void
do_munmap (void *addr) {
}
