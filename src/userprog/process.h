#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* Represents a child of a thread. Contains exit code information if the child
   has exited, otherwise sema will be at zero. */
struct child_info
  {
    tid_t pid;
    struct list_elem elem;
    struct semaphore sema;
    bool is_parent_dead;
    struct semaphore load_sema;
    bool load_success;
    int exit_code;
  };

#endif /* userprog/process.h */
