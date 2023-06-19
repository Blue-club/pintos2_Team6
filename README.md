# 💡Pintos2 6팀 WIL - Project 3 - Part 1

## 프로그램 실행 로직

1. initd()
	1. supplemental_page_table_init()
	2. process_exec()
		1. load()
			1. load_segment(): 파일 바이트를 페이지 단위로 나누고, 페이지마다 파일의 어느 영역을 읽어야 할지 설정
				1. vm_alloc_page_with_inirializer(upage, lazy_load_segment): page 구조체 생성 및 할당(malloc), page 구조체와 유저 가상 주소(upage)를 매핑, page fault가 발생하면, lazy_load_segment()를 호출하도록 설정.
					1. uninit_new (new_page, upage, lazy_load_segment)
					2. spt_insert_page(): 보조 페이지 테이블 생성 및 page 데이터 입력
			2. setup_stack()
		2. do_iret(): 프로그램 실행


## Page Fault 로직

1. page_fault()
	1. vm_try_handle_fault()
		1. vm_do_claim_page(): frame 구조체 생성 및 할당(vm_get_frame())
			1. pml4_set_page(): page와 frame 매핑
			2. swap_in(): uninit_initialize 호출
			3. uninit_initialize()
				1. anon_initializer()
				2. lazy_load_segment(): 파일의 특정 페이지를 읽고 물리 메모리에 저장한다. false이면 page_fault로 돌아가서 exit(-1)
					1. file_read()


## 트러블 슈팅

### check_address(): GOAT 최광민
```c
// userprog/syscall.c in Project 2
void
check_address(void *addr) {
	if (!is_user_vaddr (addr) || addr == NULL || pml4_get_page (&t->pml4, addr) == NULL) {
		exit (-1);
	}
}
```

- Project 2에서는 Eager loading 방식을 사용한다.
- load할 때, 파일의 모든 페이지를 페이지 테이블(pml4)에 매핑한다.
- 따라서 check_address하는 시점에 참조하고자 하는 데이터(addr)는 페이지 테이블(pml4)에 반드시 존재해야 한다.


```c
// userprog/syscall.c in Project 3
void
check_address(void *addr) {
	if (!is_user_vaddr (addr) || addr == NULL) {
		exit (-1);
	}
}
```

- Project 3에서는 Lazy loading 방식을 사용한다.
- load할 때, 파일을 읽지 않고 보조 페이지 테이블(SPT)에만 매핑한다.
	- 페이지 테이블에는 매핑하지 않는다.
	- 페이지 테이블은 Page Fault 시에 frame 구조체를 생성한 후 매핑된다.
- 따라서 시스템 콜이 호출되고 check_address하는 시점에 참조하고자 하는 데이터(addr)이 페이지 테이블(pml4)에 존재하지 않을 수 있다!



## 회고

### 김민석

- 로직을 파악하는 것이 제일 어려웠다. Supplemental Page Table을 구현하기 위해 제공된 hash 라이브러리를 사용했는데, 해시 테이블에 요소를 추가하기 위해서는 hash_elem을 추가해야 하는데 해시 테이블의 구조를 이해하는데도 어려움이 있었다.
- printf 디버깅을 굉장히 많이 활용했다. 어느 시점에 어떻게 터졌는지 확인하기에는 printf만한 것이 없다. 
- 그럼에도 해결하기 힘든 에러를 중간에 여럿 만났지만 동료들의 도움으로 잘 해결할 수 있었다.
- GOAT 홍윤표, 박윤찬, 최광민
  

### 박유빈

  

### 이동근