#include "my_pthread_t.h"
#include <ucontext.h>
#include <stdio.h>

#define TESTING 1

int my_pthread_create(mypthread_t * thread, mypthread_attr_t * attr, void *(*function)(void *), void * arg) {

	if(getcontext(&(thread->ucp)) == -1) {
		printf("getcontext error\n");
	}

	int i;
	char func_stack[1024];
	thread->ucp.uc_stack.ss_sp = func_stack;
	thread->ucp.uc_stack.ss_size = sizeof(func_stack);
	printf("Allocating the stack\n");
	makecontext(&(thread->ucp), (void *)function, 0);
	swapcontext(&uctx_main, &(thread->ucp));
	printf("Made Context\n");

}






#if TESTING

void f1(void) {

	int j;

	for(j = 0; j < 10; j++) {
		printf("Number: %d\n", j);
	}
}




int main() {

	printf("Starting Testing\n");

	mypthread_t * test_thread;
	test_thread = malloc(sizeof(pthread_t));
	mypthread_attr_t * test_thread_attr;
	void * arguments = 0;

	printf("Created Thread. Executing my_pthread_create.\n");

	if (my_pthread_create(test_thread, test_thread_attr, (void *(*)(void *))f1, arguments) != 0) {
		printf("Error creating pthread\n");
	}

	printf("Finished Execution\n");

	free(test_thread);
	free(test_thread_attr);

	return 0;

}


#endif
