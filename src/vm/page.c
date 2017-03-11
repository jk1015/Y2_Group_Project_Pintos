#include "vm/page.h"
#include "threads/thread.h"

struct hash
page_create_sup_table (void);

static bool
page_less_func (
  const struct hash_elem *a, const struct hash_elem *b, void *aux);

static unsigned
page_hash_func (const struct hash_elem *e, void *aux);

struct hash
page_create_sup_table ()
{
  struct hash sup_table;
  bool success = hash_init(&sup_table, &page_hash_func, &page_less_func, NULL);
  if (!success)
    thread_exit();
  return sup_table;
}

static bool
page_less_func (
  const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  const struct page_table_elem *page_a =
    hash_entry (a, struct page_table_elem, hash_elem);
  const struct page_table_elem *page_b =
    hash_entry (b, struct page_table_elem, hash_elem);
  return (unsigned) page_a->page_no < (unsigned) page_b->page_no;
}

static unsigned
page_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
  return (unsigned) hash_entry (e, struct page_table_elem, hash_elem)->page_no;
}
