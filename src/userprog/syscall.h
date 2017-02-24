#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
#define MAX_FILES_OPENED 128
extern struct lock filesys_lock;

#endif /* userprog/syscall.h */
