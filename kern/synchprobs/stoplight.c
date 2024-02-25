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
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

int gostraight_quadrant(uint32_t direction, uint32_t turn_number);
int turnleft_quadrant(uint32_t direction, uint32_t turn_number);

static struct rwlock *intersection_lock;
static struct lock *rightlocks[4];

/*
 * Called by the driver during initialization.
 */

void
stoplight_init() {
	int i;

	intersection_lock = rwlock_create("intersection");

	for (i = 0; i < 4; i ++) {
		rightlocks[i] = lock_create("rightlock");
	}
	return;
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup() {
	int i;

	rwlock_destroy(intersection_lock);

	for (i = 0; i < 4; i++) {
		lock_destroy(rightlocks[i]);
	}
	return;
}


void
turnright(uint32_t direction, uint32_t index)
{
	// kprintf_n("Right-turning car %lu coming from direction %lu to acquire read lock\n",
	// 	(unsigned long)index, (unsigned long)direction);
	rwlock_acquire_read(intersection_lock);
	lock_acquire(rightlocks[direction]);
	kprintf_n("Right-turning car %lu coming from direction %lu has a read lock\n",
		(unsigned long)index, (unsigned long)direction);
	inQuadrant(direction, index);
	leaveIntersection(index);
	// kprintf_n("Right-turning car %lu coming from direction %lu to release read lock",
	// 	(unsigned long)index, (unsigned long)direction);
	lock_release(rightlocks[direction]);
	rwlock_release_read(intersection_lock);
	kprintf_n("Right-turning car %lu coming from direction %lu released read lock\n",
		(unsigned long)index, (unsigned long)direction);
	return;
}

int
gostraight_quadrant(uint32_t direction, uint32_t turn_number)
{
	if (turn_number == 1) {
		return direction;
	}
	else if (turn_number == 2) {
		return (direction + 3) % 4;
	}
	else {
		return -1;
	}
}
void
gostraight(uint32_t direction, uint32_t index)
{
	int quadrant1, quadrant2;

	quadrant1 = gostraight_quadrant(direction, 1);
	quadrant2 = gostraight_quadrant(direction, 2);

	// kprintf_n("Straight-going car %lu coming from direction %lu to acquire write lock\n",
	// 	(unsigned long)index, (unsigned long)direction);
	rwlock_acquire_write(intersection_lock);
	kprintf_n("Straight-going car %lu coming from direction %lu has write lock\n",
		(unsigned long)index, (unsigned long)direction);
	inQuadrant(quadrant1, index);
	inQuadrant(quadrant2, index);
	leaveIntersection(index);
	rwlock_release_write(intersection_lock);
	kprintf_n("Straight-going car %lu coming from direction %lu released write lock\n",
		(unsigned long)index, (unsigned long)direction);

	return;
}
// 1: 1, 0, 3
// 2: 2, 1, 0
// 3: 3, 2, 1
// 0: 0, 3, 2

int
turnleft_quadrant(uint32_t direction, uint32_t turn_number)
{
	if (turn_number == 1) {
		return direction;
	}
	else if (turn_number == 2) {
		return (direction + 3) % 4;
	}
	else if (turn_number == 3) {
		return (direction + 2) % 4;
	}
	else {
		return -1;
	}
}
void
turnleft(uint32_t direction, uint32_t index)
{
	int quadrant1, quadrant2, quadrant3;

	quadrant1 = turnleft_quadrant(direction, 1);
	quadrant2 = turnleft_quadrant(direction, 2);
	quadrant3 = turnleft_quadrant(direction, 3);

	// kprintf_n("Left-turning car %lu coming from direction %lu to acquire write lock\n",
	// 	(unsigned long)index, (unsigned long)direction);
	rwlock_acquire_write(intersection_lock);
	kprintf_n("Left-turning car %lu coming from direction %lu has write lock\n",
		(unsigned long)index, (unsigned long)direction);
	inQuadrant(quadrant1, index);
	inQuadrant(quadrant2, index);
	inQuadrant(quadrant3, index);
	leaveIntersection(index);
	rwlock_release_write(intersection_lock);
	kprintf_n("Left-turning car %lu coming from direction %lu released write lock\n",
		(unsigned long)index, (unsigned long)direction);

	return;
}
