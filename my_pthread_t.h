//	AUTHOR: MING TAI HA


#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#include <ucontext.h>
#include <stdlib.h>

#define NUM_THREADS 10
#define STACK_SIZE 1024
#define NUM_LEVELS 5
#define NUM_LOCKS 1


// DEFINING THREAD STATES
/*
 	The following enum defines the possible states which a thread can be in
	READY: 		A thread has been created and is ready to be scheduled.
	RUNNING:	A thread is currently running
	WAITING:	A thread is currently waiting to be scheduled. This thread
					was originally running but has either yielded to another
					thread or is waiting for a joining thread to complete
					execution
	QUEUED:		A thread is currently enqueued for scheduled
	TERMINATED:	A thread has completed execution or has been killed
*/

typedef enum state {
	READY,
	RUNNING,
	SCHEDULED,
	WAITING,
	TERMINATED
} state;

//	Defining Structs
/*	
	The following struct provides the basic structure of a thread. The thread
		will contain a ucontext object, which holds a stack, pointer to a
		successor. It also contains a mypthread_t pointer as this struct will
		be inserted into the queue for scheduling. Additional information information
		include the Thread ID, Number of Runs, and Priority. There is also
		a struct which contains the thread attributes, as well as void pointer
		which contains the values returned from execution.
*/

typedef struct mypthread_t {

	ucontext_t ucp;
	struct mypthread_t * next_thr;
	state thr_state;
	long int thr_id;
	int num_runs;
	int priority;
	void * retval;

} mypthread_t;

/*	
	The following struct contains basic information about the associated thread.
		Information includes State, Thread ID, Num_Runs, Priority. The priority
		is defined to be HIGHEST at 1
*/
typedef struct {

} mypthread_attr_t;


//	Defining a Linked List Queue 
/*
	The queue struct will contain a head node and a size. The queue will be
	add newly created threads to the back of the queue and remove queued thread
	when the thread is scheduled to run
*/

// TODO:: Building a queue lock into the queue struct and adjust in the enqueue,
//				dequeue functions
typedef struct {

	mypthread_t * head;
	mypthread_t * tail;
	int size;

} queue;

void queue_init(queue * first);
void enqueue(queue * first, mypthread_t * thr_node);
mypthread_t * dequeue(queue * first);
mypthread_t * peek(queue * first);
char queue_isEmpty(queue * first);

//	Global Variables

//mypthread_t * thr_array;
//mypthread_attr_t * thr_attr_array;
//unsigned char is_first_thread = 0;
//const int size = ARRAY_SIZE;


//	Defining a basic Scheduler class
/*
	This class contains as a pointer to a multi-level priority scheduler, a pointer
		for multiple wait queues, and a pointer to a const list of run timings. The
		scheduler also has a pthread which contains the main context, as well as a
		counter for the number of threads assigned. The scheduler also keeps track
		of the current thread that is being run.

	MING:: The reason why the scheduler should contain the main context is so the
		scheduler can be instantiated first before creating the first mypthread_t.
		Making the first mypthread requires a uclink, which would be the main context.
		Moreover, the scheduler should exist such that any thread can be scheduled
		at anytime. If the scheduler has a pthread which contains the main context,
		the scheduling new threads can always access the main thread if needed.
*/

typedef struct {

	queue * mlpq;
	queue * wait;
	mypthread_t * thr_main;
	mypthread_t * thr_cur;
	int prior_list[NUM_LEVELS];
	long int num_sched;

} scheduler;

// Basic PTHREADS API

int my_pthread_create(mypthread_t * thread, mypthread_attr_t * attr, void *(*function)(void *), void * arg);
void my_pthread_yield();
void my_pthread_exit(void * value_ptr);
int my_pthread_join(mypthread_t * thread, void ** value_ptr);

// Auxiliary Functions

void run_thread(mypthread_t * thr_node, void *(*f)(void *), void * arg);
void aging_adjust();
void sched_init();
void sched_addThread(mypthread_t * thr_node, int priority);
mypthread_t * sched_pickThread();


#endif
