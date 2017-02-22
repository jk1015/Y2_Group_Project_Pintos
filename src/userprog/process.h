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
    bool is_parent_dead;
    tid_t pid;
    struct list_elem elem;
    struct semaphore sema;
    int exit_code;
  };

#endif /* userprog/process.h */
