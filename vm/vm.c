/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* Project 3. */
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <string.h>

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

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable, vm_initializer *init, void *aux) {
	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Project 3 */
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = malloc(sizeof(struct page));//palloc_get_page(PAL_ASSERT);
		void *new_initializer;

		switch (VM_TYPE(type)) {
			case VM_ANON:
				new_initializer = anon_initializer;
				break;
			case VM_FILE:
				new_initializer = file_backed_initializer;
				break;
		}

		uninit_new(page, upage, init, type, aux, new_initializer);
		page->writable = writable;

		return spt_insert_page(spt, page);
	}
err:
	return false;
}

#define list_elem_to_hash_elem(LIST_ELEM)                       \
	list_entry(LIST_ELEM, struct hash_elem, list_elem)

/* Find VA from spt and return page. On error, return NULL. */
/* return upage. */
struct page *spt_find_page(struct supplemental_page_table *spt, void *va) {
	/* Project 3 */
	struct hash* h = &spt->spt_hash;

	// va를 가진 진짜 page를 찾기위해 임시 page를 만든다.
	struct page *tmp_page = malloc(sizeof(struct page));
	// 4KB로 끊기는 va가 안들어올 때도 존재함 -> 확인했음.
	// 임시 page의 va를 pg_round_down (va)로 초기화.
	tmp_page->va = pg_round_down(va);
	// 임시 page의 h_elem으로 진짜 page의 h_elem을 찾는다.
	struct hash_elem *elem = hash_find(h, &tmp_page->h_elem);
	free(tmp_page);

	// 현재 스레드의 spt에서 찾지 못했다면 NULL 반환.
	if (!elem)
		return NULL;

	struct page *page = hash_entry(elem, struct page, h_elem);

	return page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page) {
	int succ = false;

	/* Project 3 */
	// 전달받은 page의 h_elem을 spt에 집어넣는다.
	// hash_insert에서 이미 같은 va를 가진 page에 대한 존재 여부 검사도 이뤄짐.
	if (hash_insert(&spt->spt_hash, &page->h_elem) == NULL)
		succ = true;
	/* Project 3 */

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
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
static struct frame *vm_get_frame(void) {
	struct frame *frame = NULL;

	/* Project 3 */
	frame = malloc(sizeof(struct frame));		// kernel pool 할당
	void *kva = palloc_get_page(PAL_USER);		// user pool 할당

	if (!kva) {
		free(frame);
		PANIC("todo\n");
	}
	/* Project 3 */

	frame->kva = kva;
	frame->page = NULL;

	return frame;
}

/* Growing the stack. */
static void vm_stack_growth (void *addr) {
	void *stack_bottom = pg_round_down(addr);
	int pg_cnt = 0;

	while (true) {
		void *upage = (uint64_t)stack_bottom + PGSIZE * pg_cnt;

		if (spt_find_page(&thread_current()->spt, upage) != NULL)
			break;

		vm_alloc_page(VM_ANON | VM_MARKER_0, upage, true);
		vm_claim_page(upage);

		pg_cnt++;
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user, bool write, bool not_present) {
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = NULL;

	if (addr == NULL)
		return false;
	else if (is_kernel_vaddr(addr))
		return false;
	else if (USER_STACK >= addr && addr >= USER_STACK - (1 << 20)) {
		if (f->rsp - 8 != addr)
			return false;

		vm_stack_growth(addr);
		return true;
	}
	else if (not_present) {
		page = spt_find_page(&spt->spt_hash, addr);

		if (page == NULL)
			return false;

		return vm_do_claim_page(page);
	}

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
bool vm_claim_page(void *va) {
	struct page *page = NULL;

	/* Project 3. */
	struct supplemental_page_table *spt = &thread_current()->spt;
	page = spt_find_page(spt, va);

	if (!page)
		return false;

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool vm_do_claim_page(struct page *page) {
	struct frame *frame = vm_get_frame();
	
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* Project 3. */
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	uint64_t* pml4 = thread_current()->pml4;
	// 가상 주소와 물리 주소를 맵핑한 정보를 페이지 테이블에 추가한다.
	pml4_set_page(pml4, page->va, frame->kva, page->writable);

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt) {
	/* Project 3 */
	// 현재 스레드의 spt의 hash table을 초기화 해준다.
	// 이 때 hash_func와 less_func를 연결해준다.
	hash_init(&spt->spt_hash, hash_func, less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy (struct supplemental_page_table *dst, struct supplemental_page_table *src) {
	struct hash_iterator iter;
	hash_first(&iter, &src->spt_hash);
	while(hash_next(&iter)) {
		struct hash_elem *elem = hash_cur(&iter);
		struct page *src_page = hash_entry(elem, struct page, h_elem);

		void *upage = src_page->va;
		void *init = src_page->uninit.init;
		void *aux = src_page->uninit.aux;
		bool writable = src_page->writable;
		
		uint8_t type = VM_TYPE(src_page->operations->type);
		switch (type) {
			case VM_UNINIT:
				// NULL이면 종료!
				ASSERT(vm_alloc_page_with_initializer (VM_ANON, upage, writable, init, aux) != NULL);
				break;
			case VM_ANON:
			case VM_FILE:
				if (vm_alloc_page (type, upage, writable)) {
					struct page *dst_page = spt_find_page (dst, upage);

					if (vm_do_claim_page(dst_page)) {
						struct frame *dst_frame = dst_page->frame;
						memcpy(dst_frame->kva, src_page->frame->kva, PGSIZE);
					}
					else {
						return false;
					}
				}
				else {
					return false;
				}
				
				break;
		}
	}
	
	return true;
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill (struct supplemental_page_table *spt) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->spt_hash, action_func);
}

/* Project 3 */
uint64_t hash_func(const struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, h_elem);
	return hash_bytes(&page->va, 8);
}

bool less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	struct page *a_page = hash_entry(a, struct page, h_elem);
	struct page *b_page = hash_entry(b, struct page, h_elem);

	return a_page->va < b_page->va;
}

void action_func(struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, h_elem);

	if(page) {
		destroy(page);
	}
}
/* Project 3 */