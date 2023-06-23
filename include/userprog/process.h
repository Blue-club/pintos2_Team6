#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

/* Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

/* Map region identifier. */
typedef int off_t;
#define MAP_FAILED ((void *) NULL)

/* Maximum characters in a filename written by readdir(). */
#define READDIR_MAX_LEN 14

/* Typical return values from main() and arguments to exit(). */
#define EXIT_SUCCESS 0          /* Successful execution. */
#define EXIT_FAILURE 1          /* Unsuccessful execution. */

struct file_segment {
	struct file *file;
	size_t page_read_bytes;
	size_t page_zero_bytes;
	off_t ofs;
};

tid_t process_create_initd (const char *);
tid_t process_fork (const char *, struct intr_frame *);
int process_exec (void *);
int process_wait (pid_t);
void process_exit (void);
void process_activate (struct thread *);

/* Project 2. */
void argument_stack (char **, int, struct intr_frame *);
int process_add_file (struct file *);
struct file *process_get_file (int);
void process_close_file (int);
struct thread *get_child_process (pid_t);
/* Project 2. */

/* Project 3 */
bool lazy_load_segment(struct page *page, void *aux);

#endif /* userprog/process.h */
