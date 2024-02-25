/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;

	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);
	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);

	lock->lock_wchan = wchan_create(lock->lk_name);
	if (lock->lock_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

	spinlock_init(&lock->lock_spinlock);
	lock->is_locked = false;
	lock->lock_holder = NULL; // TODO: feels weird

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);
	if (lock->lock_holder != NULL) {
		panic("Nothing should be holding lock in `lock_destroy`");
	}

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&lock->lock_spinlock);
	wchan_destroy(lock->lock_wchan);
	// TODO: feels weird, plus shouldn't it already be NULL?
	lock->lock_holder = NULL; 

	kfree(lock->lk_name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	/* Call this (atomically) before waiting for a lock */
	//HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

	KASSERT(lock != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the lock's spinlock to protect the wchan as well. */
	spinlock_acquire(&lock->lock_spinlock);
	HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);
	while (lock->is_locked) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the lock; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting.
		 */
		wchan_sleep(lock->lock_wchan, &lock->lock_spinlock);
	}
	KASSERT(!(lock->is_locked));
	lock->is_locked = true;
	lock->lock_holder = curthread;
	HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
	spinlock_release(&lock->lock_spinlock);

	/* Call this (atomically) once the lock is acquired */
	//HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
}

void
lock_release(struct lock *lock)
{
	/* Call this (atomically) when the lock is released */
	//HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);

	KASSERT(lock != NULL);

	if (!(lock->is_locked && lock->lock_holder == curthread)) {
		panic("Only thread holding lock can invoke `lock_release`");
	}

	spinlock_acquire(&lock->lock_spinlock);

	lock->is_locked = false;
	lock->lock_holder = NULL;
	HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);
	wchan_wakeone(lock->lock_wchan, &lock->lock_spinlock);

	spinlock_release(&lock->lock_spinlock);
}

// TODO: originally did not acquire spinlock to look at internal
// state, not sure if it's necessary
bool
lock_do_i_hold(struct lock *lock)
{
	bool result;

	spinlock_acquire(&lock->lock_spinlock);
	result = lock->is_locked && lock->lock_holder == curthread;
	spinlock_release(&lock->lock_spinlock);
	return result;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}

	cv->cv_wchan = wchan_create(cv->cv_name);
	if (cv->cv_wchan == NULL) {
		kfree(cv->cv_name);
		kfree(cv);
		return NULL;
	}

	spinlock_init(&cv->cv_spinlock);

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&cv->cv_spinlock);
	wchan_destroy(cv->cv_wchan);

	kfree(cv->cv_name);
	kfree(cv);
}

/*
 * Used to have `lock_release` before `spinlock_acquire` but that meant
 * that in cvt2, the wakethread could get in `cv_signal`, acquire the spinlock,
 * and then `wchan_wakeone` which will be a no-op because sleepthread would not
 * have gone to sleep on the wait channel yet. This order makes cvt2 pass, but
 * I'm not sure if it's quite right, and also, the fact that I don't symmetrically
 * treat the spinlock and lock on the way out of the function feels off, but all
 * the tests pass.
 * Note that if I do make it symmetric i.e. first `lock_acquire` then `spinlock_release`
 * at the end of the function, cvt2 has the following output almost immediately:
 * panic: Assertion failed: curcpu->c_spinlocks == 1, at ../../thread/thread.c:1042 (wchan_sleep)
*/
void
cv_wait(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));

	/*
	 * May not block in an interrupt handler.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&cv->cv_spinlock);
	lock_release(lock);
	wchan_sleep(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);
	lock_acquire(lock);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_spinlock);
	wchan_wakeone(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_spinlock);
	wchan_wakeall(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);
}


/*
 * Reader writer lock implementation
 */
struct rwlock *
rwlock_create(const char *name)
{
	struct rwlock *rwlock;

	rwlock = kmalloc(sizeof(*rwlock));
	if (rwlock == NULL) {
		return NULL;
	}

	rwlock->rwlock_name = kstrdup(name);
	if (rwlock->rwlock_name == NULL) {
		kfree(rwlock);
		return NULL;
	}

	// TODO: do we need the below, that exists in `lock_create`?
	// HANGMAN_LOCKABLEINIT(&rwlock->lk_hangman, rwlock->lk_name);

	rwlock->rwlock_wchan_reader = wchan_create(rwlock->rwlock_name);
	if (rwlock->rwlock_wchan_reader == NULL) {
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}
	rwlock->rwlock_wchan_writer = wchan_create(rwlock->rwlock_name);
	if (rwlock->rwlock_wchan_writer == NULL) {
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}

	spinlock_init(&rwlock->rwlock_spinlock);
	rwlock->holder_count_reader = 0; // TODO: necessary?
	rwlock->is_locked_writer = false;
	rwlock->rwlock_holder_writer = NULL; // TODO: feels weird

	return rwlock;
}
void
rwlock_destroy(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	if ((rwlock->holder_count_reader != 0) ||
		(rwlock->rwlock_holder_writer != NULL)) {
		panic("Nothing should be holding rwlock in `rwlock_destroy`");
	}

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&rwlock->rwlock_spinlock);
	wchan_destroy(rwlock->rwlock_wchan_reader);
	wchan_destroy(rwlock->rwlock_wchan_writer);
	// TODO: feels weird, plus shouldn't it already be NULL?
	// Just copied this from `lock_destroy`
	rwlock->rwlock_holder_writer = NULL; 

	kfree(rwlock->rwlock_name);
	kfree(rwlock);
}

/*
 * Operations:
 *    rwlock_acquire_read  - Get the lock for reading. Multiple threads can
 *                          hold the lock for reading at the same time.
 *    rwlock_release_read  - Free the lock.
 *    rwlock_acquire_write - Get the lock for writing. Only one thread can
 *                           hold the write lock at one time.
 *    rwlock_release_write - Free the write lock.
 *
 * These operations must be atomic. You get to write them.
 */

void
rwlock_acquire_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the rwlock's spinlock to protect the wchan as well. */
	spinlock_acquire(&rwlock->rwlock_spinlock);
	// TODO: do something with deadlock detector? Below from `lock_acquire`
	// HANGMAN_WAIT(&curthread->t_hangman, &rwlock->lk_hangman);
	while (rwlock->is_locked_writer) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the lock; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting.
		 */
		wchan_sleep(rwlock->rwlock_wchan_reader, &rwlock->rwlock_spinlock);
	}
	KASSERT(!(rwlock->is_locked_writer));
	rwlock->holder_count_reader += 1;
	// TODO: do something with deadlock detector? Below from `lock_acquire`
	// HANGMAN_ACQUIRE(&curthread->t_hangman, &rwlock->lk_hangman);
	spinlock_release(&rwlock->rwlock_spinlock);
}

void
rwlock_release_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	// TODO: do we need to verify that it's currently read-locked and that
	// the curthread holds the lock? E.g. in `lock_acquire`

	spinlock_acquire(&rwlock->rwlock_spinlock);

	rwlock->holder_count_reader -= 1;
	KASSERT(rwlock->holder_count_reader >= 0);  // TODO: necessary?
	// TODO: do anything with deadlock detector like in `lock_release`?
	wchan_wakeone(rwlock->rwlock_wchan_writer, &rwlock->rwlock_spinlock);

	spinlock_release(&rwlock->rwlock_spinlock);
}

void
rwlock_acquire_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	if (rwlock->rwlock_holder_writer == curthread) {
		panic("Current thread already holds writer lock in `rwlock_acquire_write`");
	}

	/* Use the rwlock's spinlock to protect the wchan as well. */
	spinlock_acquire(&rwlock->rwlock_spinlock);
	// TODO: do something with deadlock detector? Below from `lock_acquire`
	// HANGMAN_WAIT(&curthread->t_hangman, &rwlock->lk_hangman);
	while (rwlock->holder_count_reader > 0 || rwlock->is_locked_writer) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the lock; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting.
		 */
		wchan_sleep(rwlock->rwlock_wchan_writer, &rwlock->rwlock_spinlock);
	}
	KASSERT(rwlock->holder_count_reader == 0 && !(rwlock->is_locked_writer));
	rwlock->is_locked_writer = true;
	rwlock->rwlock_holder_writer = curthread;
	// TODO: do something with deadlock detector? Below from `lock_acquire`
	// HANGMAN_ACQUIRE(&curthread->t_hangman, &rwlock->lk_hangman);
	spinlock_release(&rwlock->rwlock_spinlock);
}

void
rwlock_release_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	// TODO: doing the below, but it's weird because I'm doing no such thing
	// in `rwlock_release_read`. Check `lock_acquire` and `lock_release` for
	// where this is all coming from
	if (!(rwlock->is_locked_writer && rwlock->rwlock_holder_writer == curthread)) {
		panic("Only writer thread holding rwlock can invoke `rwlock_release_writer`");
	}

	spinlock_acquire(&rwlock->rwlock_spinlock);

	rwlock->is_locked_writer = false;
	rwlock->rwlock_holder_writer = NULL;
	// TODO: do we need to do anything with deadlock detector? Check `lock_release`
	// TODO: is waking both wait channels the right thing?
	wchan_wakeall(rwlock->rwlock_wchan_reader, &rwlock->rwlock_spinlock);
	wchan_wakeone(rwlock->rwlock_wchan_writer, &rwlock->rwlock_spinlock);
	
	spinlock_release(&rwlock->rwlock_spinlock);
}
