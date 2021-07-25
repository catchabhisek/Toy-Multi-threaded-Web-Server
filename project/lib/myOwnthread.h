#ifndef _myOwnthread_
#define _myOwnthread_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <setjmp.h>

enum state
{
    STATE_RUNNING = 1,
    STATE_CANCEL,
    STATE_DONE,
    STATE_WAIT
};

typedef struct mythread_t {
    long int id;
} mythread_t;

typedef struct mythread_attr
{
    size_t guardsize;
    size_t stacksize;
} mythread_attr;

typedef struct thread_queue_node {

    int state;
    int id;
    void* (*routine)(void*);
    void* arg;
    void *retval;
    sigjmp_buf context;
    struct thread_queue_node* joinWait;
    struct thread_queue_node* next;
} thread_queue_node;

typedef struct thread_queue {
    thread_queue_node *front; 
    thread_queue_node *rear; 
} thread_queue;

typedef struct mythread_mutex{
    long lock;
    long owner;
} mythread_mutex_t;

typedef struct mythread_condition{
    thread_queue *wait_queue;
} mythread_cond_t;


/* Thread Library Methods*/

/*Create a new thread with the appropriate TCB and push it in the scheduler queue.*/
extern int mythread_create(mythread_t* tid, mythread_attr* attr, void *(*start)(void *), void *args);

/*Exit the thread from the scheduler queue.*/
extern int mythread_exit(void* status);

/*Set the thread status to STATE_CANCEL*/
extern int mythread_cancel (mythread_t* thread);

/*Set the guardsize and stacksize of the thread*/
extern void mythread_attr_init (mythread_attr* attr);

/*Free the allocated space for the attribute*/
extern void mythread_attr_destroy (mythread_attr* attr);


/*Give up the resources and run the preceding thread in scheduler queue.*/
extern void mythread_yield(void);

/*Join the thread with the current running thread*/
extern int mythread_join(long int thread);

/*Lock Variables for the Thread Library*/

/*Initialize the mutex structure*/
extern int mythread_mutex_init(mythread_mutex_t *mutex);

/*Use atomic operation to set the lock */
extern int mythread_mutex_lock(mythread_mutex_t *mutex);

/*Use atomic operation to unset the lock */
extern int mythread_mutex_unlock(mythread_mutex_t *mutex);

/*Free the mutex structure*/
extern int mythread_mutex_destroy(mythread_mutex_t *mutex);


/*Condition Variables for the Thread Library*/

/*Initialize the condition variables*/
extern int mythread_cond_init(mythread_cond_t *cond);

/*Use atomic operation to set the lock */
extern int mythread_cond_signal(mythread_cond_t *cond);

/*Use atomic operation to unset the lock */
extern int mythread_cond_wait(mythread_cond_t *cond, mythread_mutex_t *mutex);

/*Free the mutex structure*/
extern int mythread_cond_destroy(mythread_cond_t *cond);


/*Create itimerval structure for generating a signal per 50ms*/

extern struct itimerval timer;

/*Create sigaction structure for handling a signal*/
extern struct sigaction preemption_signal;

/*Block the signals*/
extern void block_signals(int sig);

/*Unblock the signals*/
extern void unblock_signals(int sig);

/*Initialize the timer and context switch handler*/
extern void init_signal();


#endif