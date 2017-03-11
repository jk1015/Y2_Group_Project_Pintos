#ifndef PAGE_H
#define PAGE_H

#include <inttypes.h>
#include <hash.h>

struct hash
page_create_sup_table (void);

struct page_table_elem*
page_create_sup_table_elem (void* page_location);

void
page_destroy_sup_table_elem (struct hash_elem *e, void *aux);

struct page_table_elem
{
  uint32_t page_no; // Page number
  struct hash_elem hash_elem; // Hash elem
  // TODO: Identify location of page in file system.
};

#endif
