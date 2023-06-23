/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
/* Project 3 */
#include <string.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/process.h"
#include "lib/round.h"
#include "threads/malloc.h"

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
	struct file_segment *file_segment = (struct file_segment *)page->uninit.aux;
	
	file_page->file = file_segment->file;
	file_page->page_read_bytes = file_segment->page_read_bytes;
	file_page->page_zero_bytes = file_segment->page_zero_bytes;
	file_page->ofs = file_segment->ofs;

	return true;
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
	struct file_page *file_page = &page->file;
	
	if (pml4_is_dirty(thread_current()->pml4, page->va)) {
		file_write_at(file_page->file, page->frame->kva, file_page->page_read_bytes, file_page->ofs);
		pml4_set_dirty(thread_current()->pml4, page->va, 0);
	}

	hash_delete(&thread_current()->spt.spt_hash, &page->h_elem);
	pml4_clear_page(thread_current()->pml4, page->va);
}

/* Do the mmap */
void *do_mmap (void *addr, size_t length, int writable, struct file *f, off_t offset) {
	uint64_t va = addr;
	struct file *file = file_reopen(f);
	int pg_cnt = DIV_ROUND_UP(length, PGSIZE);
	size_t file_len = file_length(file);
	size_t read_bytes = file_len < length ? file_len : length;

	while (true) {
		pg_cnt--;

		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

        struct file_segment *file_segment = malloc(sizeof(struct file_segment));
		file_segment->file = malloc(sizeof(struct file));
		memcpy(file_segment->file, file, sizeof(struct file));
        file_segment->page_read_bytes = page_read_bytes;
		file_segment->page_zero_bytes = page_zero_bytes;
		file_segment->ofs = offset;

		if (!vm_alloc_page_with_initializer(VM_FILE, va, writable, lazy_load_segment, file_segment)) {
			return NULL;
		}

		struct page *page = spt_find_page(&thread_current()->spt, va);
		page->marker = VM_DUMMY;

		if(pg_cnt == 0) {
			page->marker = VM_FILE_END;
			break;
		}

        read_bytes -= page_read_bytes;
        offset += page_read_bytes;

        va += PGSIZE;
	}

    return addr;
}

/* Do the munmap */
void do_munmap (void *addr) {
	while (true) {
		struct page *page = spt_find_page(&thread_current()->spt, addr);
		// struct file_page *file_page = &page->file;

		if (page->marker & VM_FILE_END) {
			vm_dealloc_page(page);
			break;
		}
		else {
			vm_dealloc_page(page);
		}

		addr += PGSIZE;
	}
}