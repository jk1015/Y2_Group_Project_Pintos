#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "devices/shutdown.h"


#define invalid_memory_error_message "Program attempted to access invalid memory, and has been terminated.\n"

#define throw_invalid_memory_access() (printf(invalid_memory_error_message));(thread_exit ())

static void syscall_handler (struct intr_frame *);
static void check_user_pointer (const void *uaddr);
static void*
deref_user_pointer (const void *uaddr, uint32_t offset, uint32_t size);

static int32_t sys_halt (const void* stack);
static int32_t sys_exit (const void* stack);
static int32_t sys_exec (const void* stack);
static int32_t sys_wait (const void* stack);
static int32_t sys_write (const void* stack);

static int SYSCALL_AMOUNT;


/* A syscall. */
struct syscall
{
  int number;                       /* System call number. */
  int32_t (*function) (const void *argv);    /* Function to execute action. */
};

/* Table of supported actions. */
static const struct syscall syscalls[] =
{
  {SYS_HALT, sys_halt},
  {SYS_EXIT, sys_exit},
  {SYS_EXEC, sys_exec},
  {SYS_WAIT, sys_wait},
  {SYS_CREATE, NULL},
  {SYS_REMOVE, NULL},
  {SYS_OPEN, NULL},
  {SYS_FILESIZE, NULL},
  {SYS_READ, NULL},
  {SYS_WRITE, sys_write},
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
  int syscall_nr = *((int *) deref_user_pointer(f->esp, 0, 0));
  for(int i = 0; i < SYSCALL_AMOUNT; i++)
  {
    if(syscalls[i].number == syscall_nr)
    {
      f->eax = syscalls[i].function(f->esp);
      return;
    }
    ASSERT(i != SYSCALL_AMOUNT - 1);
  }
}

static void
check_user_pointer (const void *uaddr)
{
  if (!is_user_vaddr(uaddr) || !pagedir_get_page(thread_current()->pagedir, uaddr)) {
    throw_invalid_memory_access ();
  }
}

static void *
deref_user_pointer (const void *uaddr, uint32_t offset, uint32_t size)
{

  uaddr = (void *) ((uint32_t *) uaddr + offset);
  // TODO: Check malloc, lock etc. are freed
  if (is_user_vaddr (uaddr))
  {
    if (size <= 1 || is_user_vaddr((void *) ((char *) uaddr + size - 1)))
    {
      void* ptr = pagedir_get_page (thread_current()->pagedir, uaddr);
      if (ptr != NULL)
        return ptr;
    }
  }
  //throw_invalid_memory_access ();
  NOT_REACHED ();
}

static int32_t
sys_halt (const void* stack UNUSED)
{
  shutdown_power_off ();
  NOT_REACHED ();
}

static int32_t
sys_exit (const void* stack)
{
  int32_t exit_code = *((int32_t *) deref_user_pointer(stack, 1, 1));
  thread_current ()->exit_info->exit_code = exit_code;

  thread_exit ();
  NOT_REACHED ();
}


static int32_t
sys_exec (const void* stack)
{
  /*
  int arg_arr[1];
  retrieve_args (stack, arg_arr, 1);
  arg_arr[0] = retrieve_ptr ((void*)arg_arr[0]);  // translating virtual to kernel address
  const char* cmd_line = (const char*)arg_arr[0];
  tid_t tid = process_execute (cmd_line);
  //TODO: get child thread and wait for it to load then return (tid or -1) depending on if it loaded or not
  */
  return (-1);
}

static int32_t
sys_wait (const void* stack)
{
  /*
  int arg_arr[1];
  retrieve_args (stack, arg_arr, 1);
  return process_wait (arg_arr[0]);
  */
  return(-1);
}

static int32_t
sys_write (const void* stack)
{
  int32_t fd    = *((uint32_t *) deref_user_pointer(stack, 1, 1));

  uint32_t size = *((uint32_t *) deref_user_pointer(stack, 3, 1));

  void *buffer = *((void **) deref_user_pointer(stack, 2, size));

  uint32_t ret_size;

  if (fd == 1)
  {
    ret_size = size;
    char *char_buf = (char *) buffer;
    while (size > 0)
    {
      if (size > 200)
      {
        putbuf(char_buf, 200);
      }
      else
      {
        putbuf(char_buf, size);
        break;
      }
      size -= 200;
      char_buf += 200;
    }
    return (int32_t) ret_size;
  }
  return 0;
}
