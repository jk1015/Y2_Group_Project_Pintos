#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"



#define invalid_memory_error_message "Program attempted to access invalid memory, and has been terminated.\n"

#define throw_invalid_memory_access() (printf(invalid_memory_error_message));(thread_exit ())

static void syscall_handler (struct intr_frame *);
static void check_user_pointer
(const void *uaddr, uint32_t offset, uint32_t size);
static void*
convert_user_pointer (const void *uaddr, uint32_t offset, uint32_t size);

static int32_t sys_halt (const void* stack UNUSED);
static int32_t sys_exit (const void* stack);
static int32_t sys_exec (const void* stack);
static int32_t sys_wait (const void* stack);
static int32_t sys_write (const void* stack);
static int32_t sys_create (const void* stack);
static int32_t sys_remove (const void* stack);

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
  {SYS_CREATE, sys_create},
  {SYS_REMOVE, sys_remove},
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
  int syscall_nr = *((int *) convert_user_pointer(f->esp, 0, 0));
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
check_user_pointer (const void *uaddr, uint32_t offset, uint32_t size)
{
  uaddr = (uint32_t *) uaddr + offset;
  void* end_of_buffer = (char *) uaddr + size - 1;

  if (!is_user_vaddr(uaddr) ||
      !pagedir_get_page(thread_current()->pagedir, uaddr) ||
      !is_user_vaddr(end_of_buffer) ||
      !pagedir_get_page(thread_current()->pagedir, end_of_buffer))
  {
    thread_exit ();
    NOT_REACHED ();
  }
}

/* Converts a user pointer to its location in kernel memory after checking if
   the memory access is valid. Checks a buffer by setting the size field > 0  */

static void *
convert_user_pointer (const void *uaddr, uint32_t offset, uint32_t size)
{

  uaddr = (uint32_t *) uaddr + offset;
  if (is_user_vaddr (uaddr))
  {
    if (size <= 0 || is_user_vaddr((char *) uaddr + size - 1))
    {
      void* ptr = pagedir_get_page (thread_current()->pagedir, uaddr);
      if (ptr != NULL)
        return ptr;
    }
  }
  //throw_invalid_memory_access ();
  thread_exit ();
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
  int32_t exit_code = *((int32_t *) convert_user_pointer(stack, 1, 0));

  thread_current ()->exit_info->exit_code = exit_code;
  thread_exit ();
  NOT_REACHED ();
}


static int32_t
sys_exec (const void* stack)
{
  char* cmd_line = *((char **) convert_user_pointer(stack, 1, 0));
  check_user_pointer(stack, 1, strlen(cmd_line));

  tid_t tid = process_execute (cmd_line);
  if (tid != TID_ERROR)
    return tid;

  return -1;
}

static int32_t
sys_wait (const void* stack)
{
  int pid = *((int *) convert_user_pointer(stack, 1, 0));

  return process_wait (pid);
}

static int32_t
sys_write (const void* stack)
{
  int32_t fd    = *((uint32_t *) convert_user_pointer(stack, 1, 0));
  uint32_t size = *((uint32_t *) convert_user_pointer(stack, 3, 0));
  void *buffer = *((void **) convert_user_pointer(stack, 2, size));

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

//bool create (const char * file , unsigned initial_size )
static int32_t
sys_create (const void* stack)
{
  const char *file_name = *((const char **) convert_user_pointer(stack, 1, 0));
  int size = *((int *) convert_user_pointer(stack, 2, 0));
  if (size < 0) // size must be unsigned
    return false;
  // lock
  bool answer = filesys_create(file_name, size);
  // unlock
  return answer;
}

//bool remove (const char * file )
static int32_t
sys_remove (const void* stack)
{
  const char *file_name = *((const char **) convert_user_pointer(stack, 1, 0));
  // lock
  bool answer = filesys_remove(file_name);
  // unlock
  return answer;
}
