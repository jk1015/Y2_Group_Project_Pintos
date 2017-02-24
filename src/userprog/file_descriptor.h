#ifndef USERPROG_FILE_DESCRIPTOR_H
#define USERPROG_FILE_DESCRIPTOR_H

#include "filesys/filesys.h"
#include "filesys/file.h"

struct fd_reference
{
  struct file* reference;
  tid_t pid;
};

#endif /* userprog/file_descriptor.h */
