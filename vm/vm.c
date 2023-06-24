/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* Project 3. */
#include "threads/vaddr.h"
#include "threads/mmu.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Project 3. */
static uint64_t hash_func (const struct hash_elem *e, void *aux);
static bool less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);
static void action_func (struct hash_elem *e, void *aux);
/* Project 3. */

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = malloc (sizeof (struct page));
		if (page == NULL)
			goto err;

		void *new_initializer = NULL;
		switch (VM_TYPE (type)) {
			case VM_ANON:
				new_initializer = anon_initializer;
				break;
			case VM_FILE:
				new_initializer = file_backed_initializer;
				break;
			default:
				return false;
		}
		uninit_new (page, upage, init, type, aux, new_initializer);
		page->writable = writable;

		/* TODO: Insert the page into the spt. */
		return spt_insert_page (spt, page);
	}
err:
	return false;
}

#define list_elem_to_hash_elem(LIST_ELEM)                       \
	list_entry(LIST_ELEM, struct hash_elem, list_elem)

/* Find VA from spt and return page. On error, return NULL. */
/* return upage. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;

	/* Project 3. */
	struct hash* h = &spt->spt_hash;
	// void *upage = pg_round_down (va);
	// size_t bucket_idx = hash_bytes (upage, 1 << 12) & (h->bucket_cnt - 1);
	// struct list *bucket = &h->buckets[bucket_idx];

	// struct list_elem *i;
	// for (i = list_begin (bucket); i != list_end (bucket); i = list_next (i)) {
	// 	struct hash_elem *hi = list_elem_to_hash_elem (i);
	// 	struct page *page = hash_entry (hi, struct page, h_elem);
	// 	if (upage == page->va) {
	// 		return page;
	// 	}
	// }

	page = malloc (sizeof (struct page));
	page->va = pg_round_down (va);
	struct hash_elem *elem = hash_find (h, &page->h_elem);
	free (page);

	if (elem == NULL) {
		return NULL;
	}

	struct page *upage = hash_entry (elem, struct page, h_elem);

	if (upage) {
		return upage;
	}
	/* Project 3. */

	return NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt,
		struct page *page) {
	int succ = false;

	/* Project 3. */
	/* TODO: check error. */
	if (hash_insert (&spt->spt_hash, &page->h_elem) == NULL) {
		succ = true;
	}
	/* Project 3. */

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete (&spt->spt_hash, &page->h_elem);
	vm_dealloc_page (page);
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	
	/* Project 3. */
	frame = malloc (sizeof (struct frame));
	ASSERT (frame != NULL);

	void *kva = palloc_get_page (PAL_USER);
	if (!kva) {
		free (frame);
		PANIC ("TODO");
	}
	frame->kva = kva;
	frame->page = NULL;
	/* Project 3. */

	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr) {
	void *stack_bottom = pg_round_down (addr);
	int pg_cnt = 0;
	while (true) {
		void *upage = (uint64_t)stack_bottom + pg_cnt * PGSIZE;
		if (spt_find_page (&thread_current ()->spt, upage) != NULL) {
			break;
		}
		if (!vm_alloc_page (VM_ANON | VM_MARKER_0, upage, true)) {
			break;
		}
		if (!vm_claim_page (upage)) {
			break;
		}
		pg_cnt++;
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr,
		bool user UNUSED, bool write, bool not_present) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;

	/* Project 3. */
	/* TODO: Validate the fault */
	if (addr == NULL) {
		return false;
	}
	else if (!is_user_vaddr (addr)) {
		return false;
	}
	else if (USER_STACK - (1 << 20) <= addr && addr <= USER_STACK) {
		if (f->rsp - 8 != addr) {
			return false;
		}

		vm_stack_growth (addr);
		if (pml4_get_page (thread_current ()->pml4, pg_round_down (addr)) == NULL) {
			return false;
		}
		return true;
	}
	else if (not_present) {
		page = spt_find_page (&spt->spt_hash, addr);
		if (page == NULL) {
			return false;
		}
		if (write && !page->writable) {
			return false;
		}
		return vm_do_claim_page (page);
	}
	/* Project 3. */

	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;

	/* Project 3. */
	page = spt_find_page (&thread_current ()->spt, va);
	if (page == NULL) {
		return false;
	}
	/* Project 3. */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* Project 3. */
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if (page == NULL) {
		free (frame);
		return false;
	}

	pml4_set_page (thread_current()->pml4, page->va, frame->kva, page->writable);
	/* Project 3. */

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	hash_init (&spt->spt_hash, hash_func, less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src) {
	struct hash_iterator i;
	hash_first (&i, &src->spt_hash);
	while (hash_next (&i)) {
		struct page *src_page = hash_entry (hash_cur (&i), struct page, h_elem);
		enum vm_type type = src_page->operations->type;
		void *upage = src_page->va;
		bool writable = src_page->writable;
		
		switch (VM_TYPE (type)) {
			case VM_UNINIT: {
				vm_initializer *init = src_page->uninit.init;
				void *aux = malloc (sizeof (struct file_segment));
				aux = src_page->uninit.aux;
				vm_alloc_page_with_initializer (VM_ANON, upage, writable, init, aux);
				break;
			}
			case VM_ANON:
			case VM_FILE: {
				if (!vm_alloc_page (type, upage, writable)) {
					return false;
				}
		
				if (!vm_claim_page (upage)) {
					return false;
				}

				struct page *dst_page = spt_find_page (dst, upage);
				memcpy (dst_page->frame->kva, src_page->frame->kva, PGSIZE);
				break;
			}
			default:
				return false;
		}
	}

	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// printf("destroy!!!!!\n");
	hash_clear (&spt->spt_hash, action_func);
}

static uint64_t
hash_func (const struct hash_elem *e, void *aux) {
	struct page* page = hash_entry (e, struct page, h_elem);
	return hash_bytes (&page->va, sizeof (void *));
}

static bool
less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	struct page *apage = hash_entry (a, struct page, h_elem);
	struct page *bpage = hash_entry (b, struct page, h_elem);
	return apage->va < bpage->va;
}

static void
action_func (struct hash_elem *e, void *aux) {
	struct page *page = hash_entry (e, struct page, h_elem);
	// printf("%d\n", page->operations->type);
	if (page != NULL) {
		vm_dealloc_page (page);
	}
}