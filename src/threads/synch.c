/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"


static void donate_priority (struct lock* lock, int priority);

/* Compares priorities of threads in semaphore_elems with singleton waiters */
static bool sema_elem_priority_less_func (
  const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

/* intr_level when lock_release disables interrupts (See lock_release) */
static enum intr_level lock_release_intr_level;
/* Flag showing that lock_release disabled interrupts (See lock_release)*/
static bool lock_release_intr_flag;


/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value)
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();

  while (sema->value == 0)
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0)
    {
      sema->value--;
      success = true;
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  //Corrects for earlier disabling of interrupts in lock_release

  if (lock_release_intr_flag)
  {
    old_level = lock_release_intr_level;
    lock_release_intr_flag = false;
  } else
    old_level = intr_disable ();

  if (!list_empty (&sema->waiters))
  {
    struct list_elem *max =
      list_max (&sema->waiters, &thread_priority_less_func, NULL);
    list_remove (max);
    thread_unblock (list_entry (max, struct thread, elem));
  }
  sema->value++;
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void)
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  lock->priority = PRI_MIN;
  sema_init (&lock->semaphore, 1);
  sema_init (&lock->access_sema, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  struct thread* cur = thread_current ();
  struct semaphore* lock_access = &lock->access_sema;

  donate_priority (lock, cur->priority);

  cur->waiting_on = lock;

  sema_down (&lock->semaphore);

  sema_down (lock_access);

  cur->waiting_on = NULL;
  list_push_back (&cur->acquired_locks, &lock->elem);

  if (list_empty (&lock->semaphore.waiters))
    lock->priority = PRI_MIN;
  else
  {
    lock->priority = list_entry (
      list_max (&lock->semaphore.waiters, &thread_priority_less_func, NULL),
      struct thread,
      elem)->priority;
  }

  lock->holder = thread_current ();
  sema_up (lock_access);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
  {
    lock->holder = thread_current ();
    list_push_back (&thread_current ()->acquired_locks, &lock->elem);
  }
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  lock->holder = NULL;

  struct thread* cur = thread_current ();
  struct list* lock_list = &cur->acquired_locks;
  int temp_priority = PRI_MIN;

  struct list_elem* current_elem;

  for (current_elem = list_begin (lock_list);
       current_elem != list_end (lock_list);
       current_elem = list_next (current_elem))
    {
      struct lock* acquired_lock = list_entry (current_elem, struct lock, elem);
      if (acquired_lock == lock)
        list_remove (current_elem);
      else if (temp_priority < acquired_lock->priority)
          temp_priority = acquired_lock->priority;

    }

  /* If interrupts were enabled when the priority is lowered it is possible that
     the timer could interrupt the current thread after the priority is
     decreased but before the semaphore is upped. This would cause the current
     thread to yield and only be rescheduled to up the semaphore when its new,
     lower, priority is highest in ready_threads. This is the same problem
     priority donation attempts to avoid, so we must disable interrupts. */

  lock_release_intr_level = intr_disable ();
  lock_release_intr_flag = true;
  if (temp_priority > cur->base_priority)
    cur->priority = temp_priority;
  else
    cur->priority = cur->base_priority;

  sema_up (&lock->semaphore);

}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock)
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}


/* Implements nested priority donation.
   Compares LOCK's priority to PRIORITY if PRIORITY is greater sets LOCK's
   priority to PRIORITY. Then if LOCK's holders priority is also lower sets that
   to priority. Finally if if the holder is waiting on another lock calls itself
   recursively on that lock.  */
void
donate_priority (struct lock* lock, int pri)
{

  sema_down (&lock->access_sema);

  if (lock->priority < pri)
  {

    lock->priority = pri;
    struct thread *lock_holder = lock->holder;

    /* There is a possible race condition here if the current thread is
       interrupted and the lock holder is scheduled between checking
       lock_holder->priority < pri and setting lock_holder->priority = pri;
       the lock holder could call thread_set_priority and increase its priority
       above pri. We cannot use a semaphore to prevent this as we make the
       assumption that thread_set_priority can be called in an interrupt
       context so we must disable interrupts. */

    enum intr_level old_level = intr_disable ();
    if (lock_holder != NULL && lock_holder->priority < pri)
    {
      lock_holder->priority = pri;
      intr_set_level (old_level);

      struct lock *nested_lock = lock_holder->waiting_on;
      sema_up (&lock->access_sema);

      if (nested_lock != NULL)
        donate_priority (nested_lock, pri);
    }
    else
      intr_set_level (old_level);
      sema_up (&lock->access_sema);
  }
  else
    sema_up (&lock->access_sema);
}


/* One semaphore in a list. */
struct semaphore_elem
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters))
  {
    struct list_elem *max =
      list_max (&cond->waiters, &sema_elem_priority_less_func, NULL);
      list_remove (max);


    sema_up (&list_entry (max, struct semaphore_elem, elem)->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

/* Compares priorities of threads in semaphore_elems with singleton waiters */
bool sema_elem_priority_less_func (
  const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{

  struct semaphore sema_a
      = list_entry (a, struct semaphore_elem, elem)->semaphore;
  struct semaphore sema_b
      = list_entry (b, struct semaphore_elem, elem)->semaphore;

  ASSERT (list_begin (&sema_a.waiters) == list_rbegin (&sema_a.waiters));
  ASSERT (list_begin (&sema_b.waiters) == list_rbegin (&sema_b.waiters));


  return thread_priority_less_func (list_begin (&sema_a.waiters),
                                    list_begin (&sema_b.waiters), NULL);
}
