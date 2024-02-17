/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

#define CREATELOOPS		8
#define NLOCKLOOPS    120  // TODO: change back to 120
#define NTHREADS      32  // TODO: change back to 32

static volatile unsigned long testval1;

static struct rwlock *testlock = NULL;
static struct semaphore *donesem = NULL;

struct spinlock status_lock;
static bool test_status = TEST161_FAIL;

// Declaration to avoid below error
// test/rwtest.c:75:5: error: no previous prototype for 'rwtestthread_reader' [-Werror=missing-prototypes]
// int rwtestthread_reader(void *junk, unsigned long num)
// TODO: figure out whythis error comes up here but not for e.g. `locktestthread`?
static void rwtestthread_reader(void *junk, unsigned long num);
static void rwtestthread_writer(void *junk, unsigned long num);


static
void
rwtestthread_reader(void *junk, unsigned long num)
{
	(void)junk;

	int i;

	/*
	 * Ok for other readers to print but not writer
	 */

	for (i=0; i<NLOCKLOOPS; i++) {
		kprintf_n(".");

		rwlock_acquire_read(testlock);
		random_yielder(4);

		testval1 = num;

		if (testval1 == NTHREADS) {
			goto fail;
		}
		random_yielder(4);

		if (testval1 == NTHREADS) {
			goto fail;
		}
		random_yielder(4);

		if (testval1 == NTHREADS) {
			goto fail;
		}
		random_yielder(4);

		rwlock_release_read(testlock);
	}

	// TODO: do something akin to `locktestthread`'s test of tracking
	// ownership properly?

	V(donesem);
	return;

fail:
	kprintf_n("fail");
	rwlock_release_read(testlock);
}

static
void
rwtestthread_writer(void *junk, unsigned long num)
{
	(void)junk;

	int i;

	/*
	 * Readers cannot print
	 */

	for (i=0; i<NLOCKLOOPS; i++) {
		kprintf_t(".");

		rwlock_acquire_write(testlock);
		random_yielder(4);

		testval1 = num;

		if (testval1 < NTHREADS) {
			goto fail;
		}
		random_yielder(4);

		if (testval1 < NTHREADS) {
			goto fail;
		}
		random_yielder(4);

		if (testval1 < NTHREADS) {
			goto fail;
		}
		random_yielder(4);

		rwlock_release_write(testlock);
	}

	// TODO: do something akin to `locktestthread`'s test of tracking
	// ownership properly?

	V(donesem);
	return;

fail:
	kprintf_n("fail");
	rwlock_release_write(testlock);
}

int rwtest(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	int i, result;

	kprintf_n("Starting rwt1...\n");
	for (i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");
		testlock = rwlock_create("testlock");
		if (testlock == NULL) {
			panic("rwt1: rwlock_create failed\n");
		}
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwt1: sem_create failed\n");
		}
		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testlock);
			sem_destroy(donesem);
		}
	}
	spinlock_init(&status_lock);
	test_status = TEST161_SUCCESS;

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		result = thread_fork("rwtest", NULL, rwtestthread_reader, NULL, i);
		if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
	}
	result = thread_fork("rwtest", NULL, rwtestthread_writer, NULL, NTHREADS);
	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		P(donesem);
	}
	P(donesem);

	rwlock_destroy(testlock);
	sem_destroy(donesem);
	testlock = NULL;
	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwt1");

	return 0;
}

int rwtest2(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kprintf_n("rwt2 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kprintf_n("rwt3 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kprintf_n("rwt4 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kprintf_n("rwt5 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}
