#ifndef PAGE_H
#define PAGE_H

#include <inttypes.h>
#include <hash.h>

struct hash
page_create_sup_table (void);

struct page_table_elem
{
  uint32_t page_no; // Page number
  struct hash_elem hash_elem; // Hash elem
};

#endif
