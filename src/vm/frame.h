#ifndef FRAME_H
#define FRAME_H

#include <inttypes.h>
#include <hash.h>
#include "devices/shutdown.h"

void frame_table_init (void);
void* frame_allocate(bool fill);

struct frame
{
  //void *page;                   // Pointer to frame owner
  void *kaddr;                  // Kernel virtual address
  struct hash_elem hash_elem;   // Frame Hash Table
};

#endif
