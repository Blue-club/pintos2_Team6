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

	file_page->myfile = file_segment->file;
	file_page->ofs = file_segment->ofs;
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
	// palloc_free_page(page->frame->kva); // 이거 넣으면 터짐
	// free(page->uninit.aux);
	pml4_clear_page(&thread_current()->pml4, page->va);
	free(page);
}

/* Do the mmap */
void *do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
	uint64_t va = addr;
	file = file_reopen(file);
	int pg_cnt = DIV_ROUND_UP(length, PGSIZE);

	while (length > 0) {
		
		pg_cnt--;

		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;

        struct file_segment *file_segment = malloc(sizeof(struct file_segment));
		file_segment->file = malloc(sizeof(struct file));
		memcpy(file_segment->file, file, sizeof(struct file));
        file_segment->page_read_bytes = page_read_bytes;
		file_segment->ofs = offset;

		// printf("file: %p\n", file_segment->file);//삭제
		// printf("offset: %d\n", offset);//삭제

		if (pg_cnt == 0) {
			if(!vm_alloc_page_with_initializer(VM_FILE | VM_MARKER_1, va, writable, lazy_load_segment, file_segment)) {
				return NULL;
			}
		}
        else {
			if (!vm_alloc_page_with_initializer(VM_FILE, va, writable, lazy_load_segment, file_segment))
				return NULL;
		}
        length -= page_read_bytes;
        offset += page_read_bytes;

        va += PGSIZE;
	}

    return addr;
}

/* Do the munmap */
void do_munmap (void *addr) {
	int pg_cnt = 0;

	while (true) {
		struct page *page = spt_find_page(&thread_current()->spt, (uint64_t)addr + pg_cnt * PGSIZE);
		struct file_page *file_page = &page->file;

		if (page->uninit.type & VM_MARKER_1) {
			if (page->writable) {
				// printf("------------\n");//삭제
				// printf("file_page->myfile: %p\n", file_page->myfile);//삭제
				// printf("file_page->actual_read_bytes: %d\n", file_page->actual_read_bytes);//삭제
				// printf("file_page->ofs: %d\n", file_page->ofs);//삭제
				file_write_at(file_page->myfile, page->frame->kva, file_page->actual_read_bytes, file_page->ofs);
			}
			break;
		}
		else
			if (page->writable) {
				file_write_at(file_page->myfile, page->frame->kva, file_page->actual_read_bytes, file_page->ofs);
			}
		
		pg_cnt++;
	}
	// printf("@@@@@@@@@@@\n");//삭제
}