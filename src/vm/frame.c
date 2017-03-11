#include "vm/frame.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"

static bool frame_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
static unsigned frame_hash_func (const struct hash_elem *e, void *aux UNUSED);



static struct hash frame_table;

void
frame_table_init (void)
{
  bool success = hash_init (&frame_table, &frame_hash_func, &frame_less_func, NULL);
  if (!success)
    shutdown_power_off();           // failed to initialise frame table
}

static bool
frame_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  const struct frame *frame_a = hash_entry (a, struct frame, hash_elem);
  const struct frame *frame_b = hash_entry (b, struct frame, hash_elem);
  return (unsigned) frame_a->kaddr < (unsigned) frame_b->kaddr;
}

static unsigned
frame_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
  return (unsigned) hash_entry (e, struct frame, hash_elem)->kaddr;
}

void*
frame_allocate(bool fill)
{
  void* page;
  if (fill)
    page = palloc_get_page(PAL_USER | PAL_ZERO);
  else
    page = palloc_get_page(PAL_USER);

  if (page == NULL)
    PANIC("Out of Frames");

  struct frame* frame = malloc(sizeof(struct frame));
  if (frame == NULL)
    PANIC ("Couldn't malloc, out of memory");
  frame->kaddr = page;
  void* success = hash_insert(&frame_table, &frame->hash_elem);
  ASSERT (success == NULL);

  struct page_table_elem* sup_elem = page_create_sup_table_elem (page);
  success = hash_insert(&(thread_current ()->sup_table), &sup_elem->hash_elem);
  ASSERT (success == NULL);

  return page;
}
