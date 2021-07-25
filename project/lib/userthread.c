#include "myOwnthread.h"

//#include <sys/time.h>
//struct timeval tstart, tend;
//int count=0;
//double exectime=0;

/* Queue Operations for the thread library */
void enqueue_thread(thread_queue* scheduler_queue, thread_queue_node* thread)
{
        if (scheduler_queue->rear == NULL) {
            scheduler_queue->front = thread;
            scheduler_queue->rear = thread;
            return;
        }
        scheduler_queue->rear->next = thread;
        scheduler_queue->rear = thread;
}

thread_queue_node* dequeue_thread(thread_queue* scheduler_queue)
{
        if (scheduler_queue->front == NULL) 
            return NULL;
        thread_queue_node* node = scheduler_queue->front;
        scheduler_queue->front = scheduler_queue->front->next;
        if (scheduler_queue->front == NULL) {
            scheduler_queue->rear = NULL;
        }
        node->next=NULL;
        return node;
}

int queue_empty(thread_queue* scheduler_queue)
{
    if((scheduler_queue->rear == NULL) && (scheduler_queue->front == NULL))
        return 1;
    return 0;
}

void print_queue(thread_queue* scheduler_queue)
{
    thread_queue_node* temp = scheduler_queue->front;
    while(temp)
    {
        printf(" %d \n", temp->id);
        temp=temp->next;
    }
}

/*---------------------------------------------------------------------------------*/

/*The address translater*/
unsigned long translate_address(unsigned long addr)
{
    unsigned long ret;
    asm volatile("xor    %%fs:0x30,%0\n"
        "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

/*----------------------------------------------------------------------------------*/


/*The Signal Handler Code for Thread Library*/

struct itimerval timer;
struct sigaction preemption_signal;

void context_switch(int sig);

void block_signal(int sig)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, sig);
  sigprocmask(SIG_BLOCK, &mask, NULL);
}

void unblock_signal(int sig)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, sig);
  sigprocmask(SIG_UNBLOCK, &mask,NULL);
}

void init_signal()
{
    block_signal(SIGALRM);
    preemption_signal.sa_flags = SA_NODEFER | SA_RESTART;
    preemption_signal.sa_handler = &context_switch;
    sigemptyset(&preemption_signal.sa_mask);
    sigaction(SIGALRM, &preemption_signal, NULL);
    unblock_signal(SIGALRM);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 50000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 50000;
    setitimer(ITIMER_REAL, &timer, NULL);
}

/*--------------------------------------------------------------------------------------------*/

/* Set the attribute values of the thread */
void mythread_attr_init(mythread_attr* attr)
{
    /* Initialize the mythread_attr struct with \0 values*/
    attr = (mythread_attr *)memset (attr, '\0', sizeof(mythread_attr));

    /* Default guard size is set to 50  */
    attr->guardsize = 50;
    attr->stacksize = 8400;
}

void mythread_attr_destroy(mythread_attr* attr)
{
    free(attr);
}

/* Create the thread using pthread */

static int initialized = 0;
static thread_queue_node* current_thread;
static int thread_id = 0;
static thread_queue* scheduler_queue;

mythread_t* mythread_self()
{
    mythread_t* self = (mythread_t *)malloc(sizeof(mythread_t));
    self->id = current_thread->id;
    return self;
}

void mythread_switch()
{
    if (queue_empty(scheduler_queue))
        return;

    int ret_val = sigsetjmp(current_thread->context, 1);
    if (ret_val == 1)
        return;

    if ((current_thread->state != STATE_CANCEL) && (current_thread->state != STATE_WAIT))
        enqueue_thread(scheduler_queue, current_thread);

    current_thread = dequeue_thread(scheduler_queue);

    while (current_thread->state == STATE_CANCEL)
    {
        thread_queue_node* prev = current_thread;
        current_thread = dequeue_thread(scheduler_queue);
        free(prev);
    }
    unblock_signal(SIGALRM);
    siglongjmp(current_thread->context, 1);
}

void context_switch(int sig)
{
    block_signal(SIGALRM);
//    gettimeofday(&tstart, NULL);
    mythread_switch();
//    gettimeofday(&tend, NULL );
//    exectime += (tend.tv_usec - tstart.tv_usec);
//    count++;
//    printf("count %d \n", count);
//    printf("exectime %lf \n", exectime);
}

int mythread_exit(void* status)
{
    block_signal(SIGALRM);
    if (queue_empty(scheduler_queue))
    {
        unblock_signal(SIGALRM);
        exit(0);
    }
    /* if the main thread call gtthread_exit */
    if (current_thread->id == -1)
    {
        while (!queue_empty(scheduler_queue))
        {
            unblock_signal(SIGALRM);
            context_switch(SIGALRM);
            block_signal(SIGALRM);
        }
        unblock_signal(SIGALRM);
        exit(0);
    }

    thread_queue_node* prev = current_thread;
    current_thread = dequeue_thread(scheduler_queue);
    current_thread->state = STATE_RUNNING;
    free(prev);

    unblock_signal(SIGALRM);

    siglongjmp(current_thread->context, 1);

    return 0;
}

static int growsdown (void *fromaddr)
{
    int toaddr;
    return fromaddr > (void *) &toaddr;
}

static volatile void mythread_start(void)
{
    unblock_signal(SIGALRM);

    //current_thread->retval = (*current_thread->routine) (current_thread->arg);
    (*current_thread->routine)(current_thread->arg);

    /* when start_rountine returns, call gtthread_exit*/
    mythread_exit(&current_thread->state);
}

void init()
{
    scheduler_queue = (struct thread_queue *)malloc(sizeof(struct thread_queue));
    scheduler_queue->front = NULL;
    scheduler_queue->rear = NULL;

    thread_queue_node *main_thread = (thread_queue_node *) malloc(sizeof(thread_queue_node));
    if(main_thread == NULL)
    {
        printf("\nError Allocating Memory for TCB !!");
        return;
    }
    main_thread->state = STATE_RUNNING;
    main_thread->id = -1;
    current_thread = main_thread;

    init_signal();
}

int mythread_create(mythread_t* tid, mythread_attr* attr, void *(*start)(void *), void *args)
{
    block_signal(SIGALRM);
    if(initialized == 0)
    {
        init();
        initialized=1;
    }
    int *FromAddr;
    thread_queue_node *tcb = (thread_queue_node *) malloc(sizeof(thread_queue_node));
    if(tcb == NULL)
    {
        printf("\nError Allocating Memory for TCB !!");
        return -1;
    }
    tcb->state = STATE_RUNNING;
    tcb->id = thread_id++;
    tcb->routine = start;
    tcb->arg = args;
    tcb->joinWait = (thread_queue_node *) malloc(sizeof(thread_queue_node));

    unsigned long StackBottom = (unsigned long) ((char*)malloc((attr->guardsize + attr->stacksize) * sizeof(char)));;
    unsigned long sp = (growsdown (&FromAddr)? (attr->guardsize + attr->stacksize)+StackBottom : StackBottom);
    unsigned long pc = (unsigned long) mythread_start;

    if(sigsetjmp(tcb->context, 1) == 0)
    {
        (tcb->context)->__jmpbuf[6] = translate_address(sp);
        (tcb->context)->__jmpbuf[7] = translate_address(pc);
        sigemptyset(&(tcb->context)->__saved_mask);
    }

    tid->id = tcb->id;
    enqueue_thread(scheduler_queue, tcb);
    unblock_signal(SIGALRM);
    return 0;
}

thread_queue_node* thread_get(long int tid)
{
    thread_queue_node* current = scheduler_queue->front;
    while (current != NULL)
    {
        if (current->id == tid)
            return current;
        current = current->next;
    }
    return NULL;
}

int mythread_join(long thread_id)
{
    /* if a thread tries to join itself */
    if (thread_id == current_thread->id)
        return -1;


    thread_queue_node* t;
    /* if a thread is not created */
    if ((t = thread_get(thread_id)) == NULL)
        return -1;

    /* check if that thread is joining on me */
    if (t->joinWait->id == current_thread->id)
        return -1;

    current_thread->joinWait = t;
    while (t->state == STATE_RUNNING)
    {
        unblock_signal(SIGALRM);
        context_switch(SIGALRM);
        block_signal(SIGALRM);
    }
    return 0;
}

void mythread_yield(void)
{
    block_signal(SIGALRM);

    /* if no thread to yield, simply return */
    if (queue_empty(scheduler_queue))
        return;

    unblock_signal(SIGALRM);
    context_switch(SIGALRM);
    block_signal(SIGALRM);
}

int  mythread_equal(int t1, int t2)
{
    return t1 == t2;
}

int mythread_cancel(mythread_t* thread)
{
    /* if a thread cancel itself */
    if (mythread_equal(current_thread->id, thread->id))
        mythread_exit(0);

    block_signal(SIGALRM);
    thread_queue_node* t = thread_get(thread->id);
    if (t == NULL)
    {
        unblock_signal(SIGALRM);
        return -1;
    }
    if (t->state == STATE_CANCEL)
    {
        unblock_signal(SIGALRM);
        return -1;
    }
    else
        t->state = STATE_CANCEL;
    unblock_signal(SIGALRM);
    return 0;
}

int mythread_mutex_init(mythread_mutex_t *mutex){
    mutex->lock = 0;
    mutex->owner = -1;
    return 0;
}

int mythread_mutex_lock(mythread_mutex_t *mutex)
{
    if(mutex->owner == current_thread->id && mutex->lock==1)
        return 0;

    while(mutex->lock == 1 && mutex->owner != current_thread->id)
        mythread_yield();

    __sync_val_compare_and_swap(&(mutex->lock), 0, 1);
    mutex->owner = current_thread->id;
    return 0;
}

int mythread_mutex_unlock(mythread_mutex_t *mutex)
{
    if(mutex->owner != current_thread->id)
        return -1;
    __sync_val_compare_and_swap(&(mutex->lock), 1, 0);
    mutex->owner = -1;
    return 0;
}

int mythread_mutex_destroy(mythread_mutex_t *mutex){
    free(mutex);
    return 0;
}

int mythread_cond_init(mythread_cond_t *cond) {
    cond->wait_queue = (thread_queue *)malloc(sizeof(thread_queue));
    cond->wait_queue->front = NULL;
    cond->wait_queue->rear = NULL;
    return 0;
}

int mythread_cond_signal(mythread_cond_t *cond)
{
    block_signal(SIGALRM);
    if (queue_empty(cond->wait_queue))
    {
        unblock_signal(SIGALRM);
        return 0;
    }
    else 
    {
        thread_queue_node* waiting = dequeue_thread(cond->wait_queue);
        waiting->state = STATE_RUNNING;
        enqueue_thread(scheduler_queue, waiting);
        unblock_signal(SIGALRM);
        return 0;
    }
}

int mythread_cond_wait(mythread_cond_t *cond, mythread_mutex_t *mutex) 
{
    block_signal(SIGALRM);
    mythread_mutex_unlock(mutex);
    current_thread->state = STATE_WAIT;
    enqueue_thread(cond->wait_queue, current_thread);
    unblock_signal(SIGALRM);
    while(current_thread->state == STATE_WAIT)
    {
        mythread_yield();
    }
    mythread_mutex_lock(mutex);
    return 0;
}

int mythread_cond_destroy(mythread_cond_t *cond){
    free(cond->wait_queue);
    free(cond);
    return 0;
}