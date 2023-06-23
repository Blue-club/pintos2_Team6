#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <debug.h>
#include <stddef.h>

/* Project 2. */
#include <threads/thread.h>
#include <userprog/process.h>
/* Project 2. */

void syscall_init (void);

/* Project 2. */
void check_address (void *);
void halt (void);
void exit (int);
pid_t fork (char *, struct intr_frame *);
int exec (const char *);
int wait (pid_t);
bool create (const char *, unsigned);
bool remove (const char *);
int open (const char *);
int filesize (int);
int read (int, void *, unsigned);
int write (int, const void *, unsigned);
void seek (int, unsigned);
unsigned tell (int);
void close (int);

struct lock filesys_lock;
/* Project 2. */

/* Project 3 */
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
void munmap (void *addr);
/* Project 3 */

#endif /* userprog/syscall.h */
