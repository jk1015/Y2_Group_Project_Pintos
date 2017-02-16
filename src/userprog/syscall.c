#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);
static void* deref_user_pointer (const void *uaddr);
static int SYSCALL_AMOUNT;


/* A syscall. */
struct syscall
{
  int number;                       /* System call number. */
  uint32_t (*function) (void *argv);    /* Function to execute action. */
};

/* Table of supported actions. */
static const struct syscall syscalls[] =
{
  {SYS_HALT, NULL},
  {SYS_EXIT, NULL},
  {SYS_EXEC, NULL},
  {SYS_WAIT, NULL},
  {SYS_CREATE, NULL},
  {SYS_REMOVE, NULL},
  {SYS_OPEN, NULL},
  {SYS_FILESIZE, NULL},
  {SYS_READ, NULL},
  {SYS_WRITE, NULL},
  {SYS_SEEK, NULL},
  {SYS_TELL, NULL},
  {SYS_CLOSE, NULL}
};


void
syscall_init (void)
{
  SYSCALL_AMOUNT = (uint32_t) sizeof(syscalls)/sizeof(syscalls[0]);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  void *stack_pointer = deref_user_pointer(f->esp);
  int syscall_nr = *((int*) stack_pointer);

  for(int i = 0; i < SYSCALL_AMOUNT; i++)
  {
    if(syscalls[i].number == syscall_nr)
    {
      void** stack = &stack_pointer;
      f->eax = syscalls[i].function(stack + 1);
    }
    ASSERT(i != SYSCALL_AMOUNT - 1);
  }
}

static void *
deref_user_pointer (const void *uaddr)
{
  // TODO: Check malloc, lock etc. are freed
  // TODO: Check special cases of strings, buffers, etc.
  if (is_user_vaddr (uaddr))
  {
    void* ptr = pagedir_get_page (thread_current()->pagedir, uaddr);
    if (ptr != NULL)
      return ptr;
  }
  process_exit();
  thread_exit();
}
