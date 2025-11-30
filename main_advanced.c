#include <stdio.h>
#include <unistd.h>
#include "my_pthread_t.h"

// --- Priority Test ---
void* slow_task(void* arg) {
    char* name = (char*)arg;
    printf("[%s] (Low Priority) Started. I should finish LAST.\n", name);
    
    unsigned long i;
    for (i = 0; i < 30000000; i++); // Work
    
    printf("[%s] Finished.\n", name);
    return NULL;
}

void* fast_task(void* arg) {
    char* name = (char*)arg;
    printf("[%s] (High Priority) Started later, but I should finish FIRST!\n", name);
    
    unsigned long i;
    for (i = 0; i < 30000000; i++); // Same amount of work
    
    printf("[%s] Finished.\n", name);
    return NULL;
}

// --- Semaphore Test ---
my_pthread_sem_t printer_sem; // 2 Printers available

void* print_job(void* arg) {
    char* name = (char*)arg;
    printf("User %s wants to print...\n", name);
    
    my_pthread_sem_wait(&printer_sem); // Grab a printer
    
    printf("User %s is PRINTING now.\n", name);
    sleep(1); // Simulate printing time
    printf("User %s is DONE.\n", name);
    
    my_pthread_sem_post(&printer_sem); // Release printer
    return NULL;
}

int main() {
    printf("=== TEST 1: PRIORITY SCHEDULING ===\n");
    
    my_pthread_t t_low, t_high;
    
    // Create Low priority first (Priority 0)
    my_pthread_create_priority(&t_low, slow_task, "Turtle", 0);
    
    // Create High priority second (Priority 10)
    my_pthread_create_priority(&t_high, fast_task, "Rabbit", 10);
    
    my_pthread_join(&t_low, NULL);
    my_pthread_join(&t_high, NULL);

    printf("\n=== TEST 2: SEMAPHORES (Resource Limit = 2) ===\n");
    
    my_pthread_sem_init(&printer_sem, 2); // Only 2 resources!
    my_pthread_t p1, p2, p3, p4;
    
    // 4 people try to use 2 printers
    my_pthread_create(&p1, print_job, "Alice");
    my_pthread_create(&p2, print_job, "Bob");
    my_pthread_create(&p3, print_job, "Charlie");
    my_pthread_create(&p4, print_job, "Dave");
    
    my_pthread_join(&p1, NULL);
    my_pthread_join(&p2, NULL);
    my_pthread_join(&p3, NULL);
    my_pthread_join(&p4, NULL);
    
    return 0;
}
