/*
 * Copyright (c) 2001, 2002, 2009
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
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>
// adding below to get access to `curthread`
#include <current.h>

/*
 * Called by the driver during initialization.
 */

static struct lock *testlock = NULL;
static volatile struct thread *male_thread = NULL;
static volatile struct thread *female_thread = NULL;
static volatile struct thread *matchmaker_thread = NULL;
static struct cv *testcv = NULL;

void whalemating_init() {
	testlock = lock_create("testlock");
	testcv = cv_create("testcv");
	return;
}

/*
 * Called by the driver during teardown.
 */

void
whalemating_cleanup() {
	return;
}

void
male(uint32_t index)
{
	(void)index;
	/*
	 * Implement this function by calling male_start and male_end when
	 * appropriate.
	 */
	male_start(index);
	while (1) {
		lock_acquire(testlock);
		// kprintf_n("male %d has the lock\n", index);
		if (male_thread == NULL) {
			male_thread = curthread;
			kprintf_n("male %d has the slot\n", index);
			cv_wait(testcv, testlock);
			male_end(index);
			male_thread = NULL;
			lock_release(testlock);
			break;
		}
		lock_release(testlock);
	}
	kprintf_n("male %d is returning\n", index);
	return;
}

void
female(uint32_t index)
{
	(void)index;
	/*
	 * Implement this function by calling female_start and female_end when
	 * appropriate.
	 */
	female_start(index);
	while (1) {
		lock_acquire(testlock);
		// kprintf_n("female %d has the lock\n", index);
		if (female_thread == NULL) {
			female_thread = curthread;
			kprintf_n("female %d has the slot\n", index);
			cv_wait(testcv, testlock);
			female_end(index);
			female_thread = NULL;
			lock_release(testlock);
			break;
		}
		lock_release(testlock);
	}
	kprintf_n("female %d is returning\n", index);
	return;
}

void
matchmaker(uint32_t index)
{
	(void)index;
	/*
	 * Implement this function by calling matchmaker_start and matchmaker_end
	 * when appropriate.
	 */
	matchmaker_start(index);
	while (1) {
		lock_acquire(testlock);
		// kprintf_n("matchmaker %d has the lock\n", index);
		if (matchmaker_thread == NULL && female_thread != NULL && male_thread != NULL) {
			matchmaker_thread = curthread;
			kprintf_n("matchmaker %d has the slot\n", index);
			cv_broadcast(testcv, testlock);
			matchmaker_end(index);
			matchmaker_thread = NULL;
			lock_release(testlock);
			break;
		}
		lock_release(testlock);
	}
	kprintf_n("matchmaker %d is returning\n", index);
	return;
}
