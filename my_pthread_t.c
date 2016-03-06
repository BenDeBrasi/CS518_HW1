//	AUTHOR: MING TAI HA


#include "my_pthread_t.h"
#include <ucontext.h>
#include <stdio.h>
#include <string.h>

#define TESTING 1


static scheduler * sched;
static mypthread_t * thr_list;

void queue_init(queue * first) {
	first = malloc(sizeof(queue));
	first->head = NULL;
	first->tail = NULL;
	first->size = 0;
}

void enqueue(queue * first, mypthread_t * thr_node) {
	/*
		If there is nothing in the queue, the head and tail nodes will
			point to the same node after insertion.
		Else, insert the node at the tail of the queue
	*/
	if (first->size == 0) {
		first->head = thr_node;
		first->tail = thr_node;
		first->size++;
	} else {
		first->tail->next_thr = thr_node;
		first->tail = thr_node;
		first->size++;
	}
}

mypthread_t * dequeue(queue * first) {
	/*
		If there is nothing in the queue, return NULL
		Otherwise,
			If the queue has only one node, point a temp pointer
				to the head and set the head and tail to NULL for 
				bookkeeping.
			Otherwise, point a temp pointer and adjust only the head.
			Return the temp pointer.
	*/
	if (first->size == 0) {
		printf("Nothing to dequeue from an empty queue. Returning NULL\n");
		return NULL;
	}
	mypthread_t * tmp;
	if (first->size == 1) {
		tmp = first->head;
		first->head = NULL;
		first->tail = NULL;
	} else {
		tmp = first->head;
		first->head = first->head->next_thr;
	}
	tmp->next_thr = NULL;
	first->size--;
	return tmp;
}

mypthread_t * peek(queue * first) {
	return first->head;
}

char queue_isEmpty(queue * first) {
	return first->size == 0;
}



void sched_init() {
	/*
		Initializes a Scheduler object. The number of levels in the mutlilevel
			priority queue and the number of wait queues (and thus the number
			of locks) is predefined. The scheduler also comes with a list of
			timings which define the length of the runtime cycles, a cleanly
			allocated main thread, and a counter for the threads assigned.
	
		MING:: The reason why the scheduler should contain the main context is so the
			scheduler can be instantiated first before creating the first mypthread_t.
			Making the first mypthread requires a uclink, which would be the main context.
			Moreover, the scheduler should exist such that any thread can be scheduled
			at anytime. If the scheduler has a pthread which contains the main context,
			the scheduling new threads can always access the main thread if needed.
	*/
	int i, j, k;
	
	sched = malloc(sizeof(scheduler));
	sched->mlpq = malloc(NUM_LEVELS * sizeof(queue));
	sched->wait = malloc(NUM_LOCKS  * sizeof(queue));
	sched->thr_main = (mypthread_t *) calloc(1, sizeof(mypthread_t));

	for (i = 0; i < NUM_LEVELS; i++) {
		queue_init((sched->mlpq) + i);
	}
	for (j = 0; j < NUM_LOCKS; j++) {
		queue_init((sched->wait) + j);
	}
	for (k = 0; k < NUM_LEVELS; k++) {	// This is a temporary placeholder
		sched->prior_list[k] = k + 1;	// for storing scheduling times
	}
	
	sched->num_sched = 0;

	sched->thr_main->thr_id = 0;
	sched->thr_main->thr_state = NEW;
	sched->thr_main->next_thr = sched->thr_main;
	sched->thr_cur = NULL;
}

void sched_addThread(mypthread_t * thr_node, int priority) {
	/*
		This function adds a node to a particular queue. This function is
			used to make schedule insertion easy. The number of scheduled
			threads is increased by 1. Threads that are added to the
			scheduler have their states changed.
	*/
	if (priority < 0 || priority >= NUM_LEVELS) {
		printf("The priority is not within the Multi-Level Priority Queue.\n");
	} else {
		printf("Adding thread to level %d\n", priority);
		thr_node->thr_state = READY;
		enqueue(&(sched->mlpq[priority]), thr_node);
		sched->num_sched++;
	}
}

mypthread_t * sched_pickThread() {
	/*
		This function picks a thread to be scheduled from the scheduler,
			returning the one in the lowest index queue (which is the
			highest priority queue by convention).
	*/
	int i;
	for (i = 0; i < NUM_LEVELS; i++) {
		if (sched->mlpq[i].head != NULL) {
			mypthread_t * chosen = dequeue(&(sched->mlpq[i])); 
			printf("Found a thread to schedule in level %d\n", i);
			sched->num_sched--;
			return chosen;
		}
	}
	printf("Nothing to schedule. Exit Successfully\n");
	exit(EXIT_SUCCESS);
}

void run_thread(mypthread_t * thr_node, void *(*f)(void *), void * arg) {
	/*
		This function takes a thread and executes the function (with parameters arg).
			Any return value will be stored in retval. If a state is newly terminated,
			then it will not be scheduled any longer, and the number of scheduled
			threads is reduced by 1. The scheduler will now point to the currently
			running thread
	*/
	thr_node->thr_state = RUNNING;
	sched->thr_cur = thr_node;
	thr_node->retval = f(arg);
	if (thr_node->thr_state != TERMINATED) {
		thr_node->thr_state = TERMINATED;
//		sched->num_sched--;
	}

}

int my_pthread_create(mypthread_t * thread, mypthread_attr_t * attr, void *(*function)(void *), void * arg) {
	/*
		This function takes a thread that has already been malloc'd, gives the thread
			a stack, a successor, and creates a context that runs the function
			run_thread. run_thread is a function that handles the running of the
			function with the arg fed to make the context when scheduled to run
	*/
	if(getcontext(&(thread->ucp)) == -1) {
		printf("getcontext error\n");
		return -1;
	}
	
	thread->ucp.uc_stack.ss_sp = malloc(STACK_SIZE); //func_stack
	thread->ucp.uc_stack.ss_size = STACK_SIZE;
	thread->ucp.uc_link = &(sched->thr_main->ucp);
	printf("Allocating the stack\n");
	makecontext(&(thread->ucp), (void *)run_thread, 3, thread, function, arg);
	printf("Made Context\n");
	sched_addThread(thread, 0);
	printf("Added Thread to the Scheduler.\n");
	return 0;
}

void my_pthread_yield() {
	/*
		This function swaps the current thread and runs another thread from the scheduler.
			The current function waits. 
	*/
	mypthread_t * tmp;
	printf("Printing Scheduler Attributes\n");
	tmp = sched->thr_cur;
	sched->thr_cur = sched_pickThread();
	//Add tmp to wait queue
	tmp->thr_state = WAITING;
	//TODO: Implement Wait Queue
	sched->thr_cur->thr_state = RUNNING;
	swapcontext(&(sched->thr_cur->ucp), &(tmp->ucp));
}

void my_pthread_exit(void * value_ptr) {
	/*
		This function forcibly shuts down the current thread. It does so by setting
			the current state to TERMINATED. This function first checks if the
			thread is already dead. When the thread is terminated, the thread
			perpetually yields.
	*/
	if (sched->thr_cur->thr_state == TERMINATED) {
		printf("This thread has already exited.\n");
	}
	sched->thr_cur->thr_state = TERMINATED;
	sched->thr_cur->retval = value_ptr;
//	sched->num_sched--;
	my_pthread_yield();
}

int my_pthread_join(mypthread_t * thread, void ** value_ptr) {
	/*
		This function takes in the a thread pointer and has the current thread to the
			argument thread. Any return value
	*/
	while (thread->thr_state != TERMINATED) {
		my_pthread_yield();
	}
	thread->retval = value_ptr;
}

#if TESTING

void f1(void) {

	int j;

	for(j = 0; j < 10; j++) {
		printf("Number: %d\n", j);
	}
	printf("Function f1 Done\n");
}

void f2(void) {

	int j;

	for(j = 100; j < 125; j++) {
		printf("Number: %d\n", j);
		if (j == 115) {
			my_pthread_exit(NULL);
		}
	}
	printf("Function f2 done\n");
}

int main() {

	//	Code to test queue class

/*

	queue * t_queue;
	t_queue = malloc(sizeof(queue));
	queue_init(t_queue);

	mypthread_t * test1;
	mypthread_t * test2;
	mypthread_t * test3;
	mypthread_t * temp1;
	mypthread_t * temp2;
	mypthread_t * temp3;
	test1 = malloc(sizeof(mypthread_t));
	test2 = malloc(sizeof(mypthread_t));
	test3 = malloc(sizeof(mypthread_t));

	test1->thr_id = 1;
	test2->thr_id = 2;
	test3->thr_id = 3;

	printf("Adding Test1\n");
	enqueue(t_queue, test1);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, peek(t_queue)->thr_id);
	printf("Adding Test2\n");
	enqueue(t_queue, test2);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, peek(t_queue)->thr_id);
	printf("Adding Test3\n");
	enqueue(t_queue, test3);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, peek(t_queue)->thr_id);
	printf("Removing Test1\n");
	temp1 = dequeue(t_queue);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, temp1->thr_id);
	printf("Removing Test2\n");
	temp2 = dequeue(t_queue);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, temp2->thr_id);
	printf("Adding Test1\n");
	enqueue(t_queue, temp1);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, peek(t_queue)->thr_id);
	printf("Removing Test3\n");
	temp3 = dequeue(t_queue);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, temp3->thr_id);
	printf("Removing Test1\n");
	temp1 = dequeue(t_queue);
	printf("Size of Q: %d\t First Element: %li\n", t_queue->size, temp1->thr_id);
	printf("Removing from an empty queue\n");
	dequeue(t_queue);

	free(test1);
	free(test2);
	free(test3);
	free(t_queue);

*/

	//	Code to test Scheduler mechanisms

/*
	long int i;
	long int j;
	mypthread_t * test_thread;

	printf("Allocating space for the thread array\n");
	thr_list = malloc(NUM_THREADS * sizeof(mypthread_t));
	printf("Initializing the Scheduler\n");
	sched_init();
	printf("Printing Scheduler values\n");
	printf("The main thread's status %d\n", sched->thr_main->thr_state);

	printf("Giving IDs to the threads\n");
	for (i = 0; i < NUM_THREADS; i++) {
		thr_list[i].thr_id = i;
	}

	printf("Adding the threads to the scheduler\n");
	for (i = 0; i < NUM_THREADS; i++) {
		sched_addThread(&thr_list[i], i % NUM_LEVELS);
		printf("Number of items scheduled %li\n", sched->num_sched);
	}

	printf("Printing the Added Threads\n");
	for (i = 0; i < NUM_LEVELS; i++) {
		for (j = 0; j < 2; j++) {
			printf("Thread: %li, Size: %d\n", sched->mlpq[i].head->thr_id, sched->mlpq[i].size);
		}
	}

	printf("Removing Threads\n");
	for (i = 0; i < NUM_THREADS; i++) {
		test_thread = sched_pickThread();
		printf("Chosen Thread: %li\n", test_thread->thr_id);
		printf("Number of remaining threads %li\n", sched->num_sched);
	}
	test_thread = NULL;
	
	printf("Checking if removing from empty scheduler\n");
	test_thread = sched_pickThread();

	printf("Checking if the threads are still intact in thread list\n");
	for (i = 0; i < NUM_THREADS; i++) {
		printf("%li\t", thr_list[i].thr_id);
	}

	printf("Freeing Thread List\n");
	free(thr_list);
	printf("Freeing scheduler\n");	
	free(sched);
*/

	//	Code for testing pthreads


	printf("Starting Testing\n");


	printf("Allocating space for the thread array\n");
	thr_list = malloc(NUM_THREADS * sizeof(mypthread_t));
	printf("Initializing the Scheduler\n");
	sched_init();
	
	printf("Initializing thread\n");

	long int i;
	long int j;
	mypthread_t * test_thread1;
	mypthread_t * test_thread2;
	test_thread1 = malloc(sizeof(mypthread_t));
	test_thread2 = malloc(sizeof(mypthread_t));
	test_thread1->thr_id = 1;
	test_thread2->thr_id = 123;

	mypthread_t * sched_thread;

	mypthread_attr_t * test_thread_attr;
	void * arguments = NULL;

	printf("Creating Thread 1\n");

	if (my_pthread_create(test_thread1, test_thread_attr, (void *(*)(void *))f1, arguments) != 0) {
		printf("Error creating pthread 1\n");
	}

	printf("Creating Thread 2\n");

	if (my_pthread_create(test_thread2, test_thread_attr, (void *(*)(void *))f2, arguments) != 0) {
		printf("Error creating pthread 2\n");
	}

	printf("Stack Size: %li\n", sched->num_sched);
	sched_thread = sched_pickThread();
//	sched->thr_cur = sched_thread;
	printf("Just picked a thread. The ID is %li and the STATE is %d\n", sched_thread->thr_id, sched_thread->thr_state);

	if (swapcontext(&(sched->thr_main->ucp), &(sched_thread->ucp)) == -1) {
		printf("Error swapping threads\n");
	}

	sched_thread = sched_pickThread();
//	sched->thr_cur = sched_thread;
	printf("Just picked a thread. The ID is %li and the STATE is %d\n", sched_thread->thr_id, sched_thread->thr_state);

	printf("Fault before the swap\n");
	if (swapcontext(&(sched->thr_main->ucp), &(sched_thread->ucp)) == -1) {
		printf("Error swapping threads\n");
	}
	
	printf("Finished Execution\n");

	sched_thread = NULL;
	free(thr_list);
	free(test_thread1);
	free(test_thread2);

	//free(thr_array);
	//free(thr_attr_array);


	return 0;
}

#endif
