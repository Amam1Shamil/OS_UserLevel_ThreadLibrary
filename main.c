#include <stdio.h>
#include <unistd.h>
#include "my_pthread_t.h"

// --- Global Resources ---
my_pthread_mutex_t lock;
my_pthread_cond_t cond;
int data_available = 0;
int global_counter = 0;

// --- Task 1: Producer (Uses Cond Signal) ---
void* producer(void* arg) {
    printf("[Producer] Starting...\n");
    
    // Simulate work
    sleep(1); 
    
    my_pthread_mutex_lock(&lock);
    data_available = 1;
    printf("[Producer] Data ready! Signaling consumer...\n");
    my_pthread_cond_signal(&cond);
    my_pthread_mutex_unlock(&lock);
    
    // Return a value to test join
    static int exit_code = 100;
    return &exit_code;
}

// --- Task 2: Consumer (Uses Cond Wait) ---
void* consumer(void* arg) {
    printf("[Consumer] Starting...\n");
    
    my_pthread_mutex_lock(&lock);
    while (data_available == 0) {
        printf("[Consumer] Waiting for data...\n");
        my_pthread_cond_wait(&cond, &lock);
    }
    printf("[Consumer] Woke up! Data detected.\n");
    my_pthread_mutex_unlock(&lock);
    
    return NULL;
}

// --- Task 3: CPU Bound (Tests Preemption/Timer) ---
void* cpu_burner(void* arg) {
    char* name = (char*)arg;
    printf("[%s] Starting long calculation...\n", name);
    
    // This loop is HUGE. Without Preemption, this would freeze the program.
    // Because we have a timer, it should switch between Burner A and B.
    unsigned long i;
    for (i = 0; i < 200000000; i++) {
        if (i % 50000000 == 0) {
             printf("[%s] Progress: %lu\n", name, i);
        }
    }
    printf("[%s] Finished.\n", name);
    return NULL;
}

int main() {
    printf("=== MyPthread Library Test ===\n");
    
    my_pthread_t t_prod, t_cons, t_burn1, t_burn2;
    void* ret_val;

    my_pthread_mutex_init(&lock);
    my_pthread_cond_init(&cond);

    // 1. Create Producer/Consumer
    my_pthread_create(&t_cons, consumer, NULL);
    my_pthread_create(&t_prod, producer, NULL);

    // 2. Create CPU Burners (to test Timer Interrupts)
    my_pthread_create(&t_burn1, cpu_burner, "Burner A");
    my_pthread_create(&t_burn2, cpu_burner, "Burner B");

    // 3. Join Threads
    my_pthread_join(&t_prod, &ret_val);
    printf("=== MAIN: Producer joined with exit code: %d ===\n", *(int*)ret_val);
    
    my_pthread_join(&t_cons, NULL);
    my_pthread_join(&t_burn1, NULL);
    my_pthread_join(&t_burn2, NULL);

    printf("=== All threads completed successfully ===\n");
    return 0;
}