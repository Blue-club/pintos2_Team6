/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

/* Project 3. */
#include <string.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/process.h"
/* Project 3. */

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
static bool lazy_load_file (struct page *page, void *aux);

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
	void *upage = addr;
	file = file_reopen (file);
	while (length > 0) {
		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		void *aux = NULL;
		struct file_segment *file_segment = malloc (sizeof (struct file_segment));
		file_segment->file = malloc (sizeof (struct file));
		file_segment->ofs = offset;
		file_segment->page_read_bytes = page_read_bytes;
		file_segment->page_zero_bytes = page_zero_bytes;
		memcpy (file_segment->file, file, sizeof (struct file));

		file_seek (file_segment->file, offset);
		aux = (void *)file_segment;
		if (!vm_alloc_page_with_initializer (VM_FILE | VM_MARKER_1, upage,
					writable, lazy_load_file, aux))
			return NULL;
			
		length -= page_read_bytes;
		offset += page_read_bytes;
		upage += PGSIZE;
	}
	return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
}

static bool
lazy_load_file (struct page *page, void *aux) {
	/* Project 3. */
	struct file_segment *file_segment = (struct file_segment *)aux;
	struct file *file = file_segment->file;
	off_t ofs = file_segment->ofs;
	size_t page_read_bytes = file_segment->page_read_bytes;

	if (pml4_get_page (thread_current ()->pml4, page->va) == NULL) {
		return false;
	}

	/* Get a page of memory. */
	void *kpage = page->frame->kva;
	if (kpage == NULL) {
		return false;
	}
	file_seek (file, ofs);
	size_t read_bytes = file_read (file, kpage, page_read_bytes);
	memset (kpage + read_bytes, 0, PGSIZE - read_bytes);

	//free (file);
	free (file_segment);
	return true;
}