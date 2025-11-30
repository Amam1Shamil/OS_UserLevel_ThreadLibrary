#include "my_pthread_t.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define STACK_SIZE 64 * 1024
#define TIME_QUANTUM_MS 50 // Switch every 50ms

// --- Globals ---
my_pthread_t* current_thread = NULL;
my_pthread_t* run_queue_head = NULL;
my_pthread_t* run_queue_tail = NULL;
my_pthread_t main_thread_wrapper; // Placeholder for the original main() context

int thread_id_counter = 0;
int library_initialized = 0;
long context_switch_counter = 0; // Statistic

// --- Helper: Block/Unblock Timer (Critical Sections) ---
// This prevents the timer from firing while we are modifying queues
void enter_critical_section() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void exit_critical_section() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

// --- Priority Queue Management ---
void enqueue(my_pthread_t* thread) {
    thread->next = NULL;

    // Case 1: List is empty
    if (run_queue_head == NULL) {
        run_queue_head = thread;
        run_queue_tail = thread;
        return;
    }

    // Case 2: New thread has higher priority than the Head
    // (Higher number = Higher priority, so it cuts in line)
    if (thread->priority > run_queue_head->priority) {
        thread->next = run_queue_head;
        run_queue_head = thread;
        return;
    }

    // Case 3: Find the right spot in the middle/end
    my_pthread_t* current = run_queue_head;
    
    // Traverse until we find a thread with lower priority, or hit the end
    while (current->next != NULL && current->next->priority >= thread->priority) {
        current = current->next;
    }

    // Insert 'thread' after 'current'
    thread->next = current->next;
    current->next = thread;

    // Update tail if we inserted at the very end
    if (thread->next == NULL) {
        run_queue_tail = thread;
    }
}

my_pthread_t* dequeue() {
    if (run_queue_head == NULL) return NULL;
    my_pthread_t* thread = run_queue_head;
    run_queue_head = run_queue_head->next;
    if (run_queue_head == NULL) run_queue_tail = NULL;
    return thread;
}

// --- Scheduler ---
void schedule() {
    // 1. Get next thread
    my_pthread_t* prev_thread = current_thread;
    my_pthread_t* next_thread = dequeue();

    if (next_thread == NULL) {
        // No threads left? Continue running current if valid, else exit/return
        if (current_thread->state == TERMINATED) {
             exit(0); // All work done
        }
        return;
    }

    // 2. Switch Context
    current_thread = next_thread;
    current_thread->state = RUNNING;
    context_switch_counter++;

    if (prev_thread != next_thread) {
        swapcontext(&prev_thread->context, &next_thread->context);
    }
}

// --- Timer Handler (Preemption) ---
void timer_handler(int signum) {
    // Only yield if we are actually running a thread
    if (current_thread != NULL && current_thread->state == RUNNING) {
        my_pthread_yield();
    }
}

// --- Library Init ---
void init_library() {
    if (library_initialized) return;

    // Capture main thread as Thread 0
    current_thread = &main_thread_wrapper;
    current_thread->id = 0;
    current_thread->state = RUNNING;
    current_thread->priority = 0; 

    // Setup Timer
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = TIME_QUANTUM_MS * 1000;
    timer.it_value = timer.it_interval;
    setitimer(ITIMER_REAL, &timer, NULL);

    library_initialized = 1;
}

// --- Thread Wrapper ---
void thread_start_wrapper(void *(*start_routine)(void *), void *arg) {
    exit_critical_section(); // Re-enable interrupts
    void* retval = start_routine(arg);
    my_pthread_exit(retval);
}

// --- Thread Management API ---

int my_pthread_create(my_pthread_t *thread, void *(*function)(void*), void *arg) {
    return my_pthread_create_priority(thread, function, arg, 0);
}

int my_pthread_create_priority(my_pthread_t *thread, void *(*function)(void*), void *arg, int priority) {
    if (!library_initialized) init_library();

    enter_critical_section();

    thread->id = ++thread_id_counter;
    thread->state = READY;
    thread->priority = priority;

    getcontext(&thread->context);
    thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_link = NULL;

    makecontext(&thread->context, (void (*)(void))thread_start_wrapper, 2, function, arg);

    enqueue(thread); // Enqueue handles priority sorting

    exit_critical_section();
    return thread->id;
}

void my_pthread_yield() {
    enter_critical_section();
    
    my_pthread_t* me = current_thread;
    if (me->state == RUNNING) {
        me->state = READY;
        enqueue(me);
    }
    
    schedule();
    exit_critical_section();
}

void my_pthread_exit(void *retval) {
    enter_critical_section();
    
    current_thread->state = TERMINATED;
    current_thread->return_value = retval;
    
    schedule();
    
    exit_critical_section(); 
}

int my_pthread_join(my_pthread_t *thread, void **retval) {
    while (thread->state != TERMINATED) {
        my_pthread_yield();
    }
    if (retval != NULL) {
        *retval = thread->return_value;
    }
    return 0;
}

// --- Smart Blocking Mutex API ---

int my_pthread_mutex_init(my_pthread_mutex_t *mutex) {
    mutex->is_locked = 0;
    mutex->owner = NULL;
    mutex->wait_queue_head = NULL;
    mutex->wait_queue_tail = NULL;
    return 0;
}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
    // Atomic Test-and-Set
    while (__sync_lock_test_and_set(&mutex->is_locked, 1)) {
        
        // Lock is busy! Block execution.
        enter_critical_section();

        // 1. Add to Mutex Wait Queue
        current_thread->next = NULL;
        if (mutex->wait_queue_head == NULL) {
            mutex->wait_queue_head = current_thread;
            mutex->wait_queue_tail = current_thread;
        } else {
            mutex->wait_queue_tail->next = current_thread;
            mutex->wait_queue_tail = current_thread;
        }

        // 2. Block and Switch
        current_thread->state = BLOCKED;
        schedule(); // Go to sleep

        exit_critical_section();
    }
    mutex->owner = current_thread;
    return 0;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
    if (mutex->owner == current_thread) {
        mutex->owner = NULL;
        __sync_lock_release(&mutex->is_locked);
        
        enter_critical_section();
        // Wake up the first waiter
        if (mutex->wait_queue_head != NULL) {
            my_pthread_t* waking = mutex->wait_queue_head;
            mutex->wait_queue_head = waking->next;
            if (mutex->wait_queue_head == NULL) mutex->wait_queue_tail = NULL;

            waking->state = READY;
            enqueue(waking); // Put back in CPU Run Queue
        }
        exit_critical_section();
        return 0;
    }
    return -1;
}

// --- Condition Variable API ---

int my_pthread_cond_init(my_pthread_cond_t* cond) {
    cond->wait_queue_head = NULL;
    cond->wait_queue_tail = NULL;
    return 0;
}

int my_pthread_cond_wait(my_pthread_cond_t* cond, my_pthread_mutex_t* mutex) {
    my_pthread_mutex_unlock(mutex); // Release Lock

    enter_critical_section();
    
    // Add to Cond Wait Queue
    current_thread->next = NULL;
    if (cond->wait_queue_head == NULL) {
        cond->wait_queue_head = current_thread;
        cond->wait_queue_tail = current_thread;
    } else {
        cond->wait_queue_tail->next = current_thread;
        cond->wait_queue_tail = current_thread;
    }

    current_thread->state = BLOCKED;
    schedule(); // Sleep
    
    exit_critical_section();

    my_pthread_mutex_lock(mutex); // Re-acquire Lock
    return 0;
}

int my_pthread_cond_signal(my_pthread_cond_t* cond) {
    enter_critical_section();
    if (cond->wait_queue_head != NULL) {
        my_pthread_t* t = cond->wait_queue_head;
        cond->wait_queue_head = cond->wait_queue_head->next;
        if (cond->wait_queue_head == NULL) cond->wait_queue_tail = NULL;

        t->state = READY;
        enqueue(t);
    }
    exit_critical_section();
    return 0;
}

// --- Semaphore API ---

int my_pthread_sem_init(my_pthread_sem_t *sem, int value) {
    sem->count = value;
    my_pthread_mutex_init(&sem->lock);
    my_pthread_cond_init(&sem->cond);
    return 0;
}

int my_pthread_sem_wait(my_pthread_sem_t *sem) {
    my_pthread_mutex_lock(&sem->lock);
    while (sem->count <= 0) {
        my_pthread_cond_wait(&sem->cond, &sem->lock);
    }
    sem->count--;
    my_pthread_mutex_unlock(&sem->lock);
    return 0;
}

int my_pthread_sem_post(my_pthread_sem_t *sem) {
    my_pthread_mutex_lock(&sem->lock);
    sem->count++;
    my_pthread_cond_signal(&sem->cond);
    my_pthread_mutex_unlock(&sem->lock);
    return 0;
}