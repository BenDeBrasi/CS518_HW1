#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#include <ucontext.h>

static ucontext_t uctx_main;

typedef struct {

	ucontext_t ucp;
	pthread_attr_t attr;

} mypthread_t;


typedef struct {



} mypthread_attr_t;



int my_pthread_create(mypthread_t * thread, mypthread_attr_t * attr, void *(*function)(void *), void * arg);
void my_pthread_yield();
void my_pthread_exit(void * value_ptr);
int my_pthread_join(mypthread_t thread, void ** value_ptr);




#endif
