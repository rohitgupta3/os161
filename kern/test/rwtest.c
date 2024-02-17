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

/*
 * Use these stubs to test your reader-writer locks.
 */

#define CREATELOOPS		8
#define NSEMLOOPS     63
#define NTHREADS      32

static volatile unsigned long testval1;

// static struct semaphore *testsem = NULL;
static struct rwlock *testlock = NULL;
static struct semaphore *donesem = NULL;

struct spinlock status_lock;
static bool test_status = TEST161_FAIL;

// static unsigned long semtest_current;

// Declaration to avoid below error
// test/rwtest.c:75:5: error: no previous prototype for 'rwtestthread_reader' [-Werror=missing-prototypes]
// int rwtestthread_reader(void *junk, unsigned long num)
// TODO: figure out whythis error comes up here but not for e.g. `locktestthread`?
static void rwtestthread_reader(void *junk, unsigned long num);


static
bool
failif(bool condition) {
	if (condition) {
		spinlock_acquire(&status_lock);
		test_status = TEST161_FAIL;
		spinlock_release(&status_lock);
	}
	return condition;
}


// static
// void
// // rwtestthread(void *junk, unsigned long num)
// semtestthread(void *junk, unsigned long num)
// {
// 	// copy of semtestthread
// 	(void)junk;
// 
// 	int i;
// 
// 	random_yielder(4);
// 
// 	/*
// 	 * Only one of these should print at a time.
// 	 */
// 	P(testsem);
// 	semtest_current = num;
// 
// 	kprintf_n("Thread %2lu: ", num);
// 	for (i=0; i<NSEMLOOPS; i++) {
// 		kprintf_t(".");
// 		kprintf_n("%2lu", num);
// 		random_yielder(4);
// 		failif((semtest_current != num));
// 	}
// 	kprintf_n("\n");
// 
// 	V(donesem);
// }

static
void
rwtestthread_reader(void *junk, unsigned long num)
{
	(void)junk;

	/*
	* Ok for other readers to print but not writer
	 */
	testval1 = num;

	return;

}

int rwtest(int nargs, char **args) {
	// copy of semtest
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
	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		P(donesem);
	}

	rwlock_destroy(testlock);
	sem_destroy(donesem);
	testlock = NULL;
	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwt1");

	return 0;

// 	// copy of locktest
// 	// int i, result;
// 
// 	// kprintf_n("Starting lt1...\n");
// 	// for (i=0; i<CREATELOOPS; i++) {
// 	// 	kprintf_t(".");
// 	// 	testlock = lock_create("testlock");
// 	// 	if (testlock == NULL) {
// 	// 		panic("lt1: lock_create failed\n");
// 	// 	}
// 	// 	donesem = sem_create("donesem", 0);
// 	// 	if (donesem == NULL) {
// 	// 		panic("lt1: sem_create failed\n");
// 	// 	}
// 	// 	if (i != CREATELOOPS - 1) {
// 	// 		lock_destroy(testlock);
// 	// 		sem_destroy(donesem);
// 	// 	}
// 	// }
// 	// spinlock_init(&status_lock);
// 	// test_status = TEST161_SUCCESS;
// 
// 	// for (i=0; i<NTHREADS; i++) {
// 	// 	kprintf_t(".");
// 	// 	result = thread_fork("synchtest", NULL, locktestthread, NULL, i);
// 	// 	if (result) {
// 	// 		panic("lt1: thread_fork failed: %s\n", strerror(result));
// 	// 	}
// 	// }
// 	// for (i=0; i<NTHREADS; i++) {
// 	// 	kprintf_t(".");
// 	// 	P(donesem);
// 	// }
// 
}

int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt2 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt3 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt4 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt5 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}
