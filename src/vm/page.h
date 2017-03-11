#ifndef PAGE_H
#define PAGE_H

#include <inttypes.h>
#include <hash.h>

struct page_table_elem
{
  uint32_t page_no; // Page number
  struct hash_elem elem; // Hash elem
};

#endif
