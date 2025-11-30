#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

// Thread States
typedef enum { CREATED, RUNNING, READY, BLOCKED, TERMINATED } state_t;

// --- Thread Control Block (TCB) ---
typedef struct my_pthread_t {
    int id;                     // Unique Thread ID
    ucontext_t context;         // CPU Context (Registers, Stack, PC)
    state_t state;              // Current State
    void* return_value;         // To store exit value
    int priority;               // Priority Level (Higher # = Higher Priority)
    struct my_pthread_t* next;  // Pointer for Linked List Queue
} my_pthread_t;

// --- Smart Blocking Mutex Structure ---
typedef struct {
    int is_locked;                  // 0 = Unlocked, 1 = Locked
    my_pthread_t* owner;            // Thread holding the lock
    my_pthread_t* wait_queue_head;  // Head of threads waiting for this lock
    my_pthread_t* wait_queue_tail;  // Tail of threads waiting for this lock
} my_pthread_mutex_t;

// --- Condition Variable Structure ---
typedef struct {
    my_pthread_t* wait_queue_head; // Queue of threads waiting on this condition
    my_pthread_t* wait_queue_tail;
} my_pthread_cond_t;

// --- Semaphore Structure ---
typedef struct {
    int count;                  // Resource count
    my_pthread_mutex_t lock;    // Protection for the count
    my_pthread_cond_t cond;     // Wait queue for the semaphore
} my_pthread_sem_t;

// --- API Function Prototypes ---

// 1. Thread Management
int my_pthread_create(my_pthread_t *thread, void *(*function)(void*), void *arg);
int my_pthread_create_priority(my_pthread_t *thread, void *(*function)(void*), void *arg, int priority);
void my_pthread_exit(void *retval);
int my_pthread_join(my_pthread_t *thread, void **retval);
void my_pthread_yield(void);

// 2. Synchronization (Smart Mutex)
int my_pthread_mutex_init(my_pthread_mutex_t *mutex);
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

// 3. Synchronization (Condition Variables)
int my_pthread_cond_init(my_pthread_cond_t* cond);
int my_pthread_cond_wait(my_pthread_cond_t* cond, my_pthread_mutex_t* mutex);
int my_pthread_cond_signal(my_pthread_cond_t* cond);

// 4. Synchronization (Semaphores)
int my_pthread_sem_init(my_pthread_sem_t *sem, int value);
int my_pthread_sem_wait(my_pthread_sem_t *sem);
int my_pthread_sem_post(my_pthread_sem_t *sem);

#endif