/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* Project 3: Swapping. */
static bool *swap_arr;

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
	swap_disk = disk_get (1, 1);

	int swap_arr_len = disk_size (swap_disk) / 8;
	swap_arr = malloc (swap_arr_len * sizeof (bool));

	for (int i = 0; i < swap_arr_len; i++) {
		swap_arr[i] = false;
	}
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->sec_no = -1;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	// printf("anonononnononnonon\n");
	int start_sec_no = anon_page->sec_no;
	anon_page->sec_no = -1;

	swap_arr[start_sec_no / 8] = false;

	for (int i = start_sec_no, j = 0; i < start_sec_no + 8; i++, j++) {
		disk_read(swap_disk, i, kva + j * DISK_SECTOR_SIZE);
	}

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	// printf("out!\n");
	int arr_len = disk_size (swap_disk) / 8;
	int start_sec_no = 0;

	for (int i = 0; i < arr_len; i++) {
		if (swap_arr[i] == false) {
			swap_arr[i] = true;
			start_sec_no = i * 8;
			break;
		}
	}

	page->anon.sec_no = start_sec_no;
	for (int i = start_sec_no, j = 0; i < start_sec_no + 8; i++, j++) {
		disk_write (swap_disk, i, page->frame->kva + j * DISK_SECTOR_SIZE);
	}

	pml4_clear_page (thread_current ()->pml4, page->va);
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	struct thread *t = thread_current ();

	pml4_clear_page (t->pml4, page->va);
}