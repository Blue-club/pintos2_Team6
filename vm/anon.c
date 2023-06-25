/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/malloc.h"
#include "threads/mmu.h"
#include <string.h>

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	swap_disk->swap_arr = malloc((int)(disk_size(swap_disk) / 8));
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;

}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;

	
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	int arr_len = disk_size(swap_disk) / 8;
	int start_sec_no = 0;
	int end_sec_no = 0;
	
	for(int i = 0; i < arr_len; i++) {
		if (swap_disk->swap_arr[i] == false) {
			swap_disk->swap_arr[i] = true;
			start_sec_no = i * 8;
			end_sec_no = start_sec_no + 8;
			break;
		}
	}

	for (int i = start_sec_no; i < end_sec_no; i++) {
		disk_write(swap_disk, i, page->frame->kva);
	}

	pml4_clear_page(thread_current()->pml4, page->va);
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}