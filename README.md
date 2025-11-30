# MyPthread - Custom User-Level Thread Library

**Course:** Operating Systems  
**Project:** User-Level Thread Library Implementation  
**Language:** C (using `ucontext.h`)

---

## 📖 Overview
This project is a fully functional **Many-to-One User-Level Thread Library** built from scratch in C. It mimics the behavior of the standard POSIX `pthread` library but runs entirely in user space without kernel-level thread management.

The library acts as a mini-Operating System, managing execution contexts, scheduling threads, and handling synchronization primitives.

---

## 🚀 Features Implemented

### 1. Thread Management
*   **Creation:** `my_pthread_create` initializes a new execution context with a dedicated 64KB stack.
*   **Termination:** `my_pthread_exit` allows threads to exit gracefully and pass return values.
*   **Joining:** `my_pthread_join` implements blocking waits to retrieve return values from finished threads.
*   **Yielding:** `my_pthread_yield` allows voluntary context switching.

### 2. Advanced Scheduling
*   **Preemptive Multitasking:** Uses `setitimer` to fire `SIGALRM` interrupts every **50ms**. This forces context switches, preventing CPU-bound tasks from freezing the system.
*   **Priority Scheduling:** Implements a **Priority Queue**. High-priority threads (created via `my_pthread_create_priority`) are automatically inserted at the front of the ready queue (O(N) Insertion Sort).
*   **Round-Robin:** Threads of equal priority are scheduled in a FIFO manner.

### 3. Synchronization Primitives
*   **Smart Blocking Mutexes:** Unlike inefficient spinlocks, these mutexes put threads to sleep (Blocked state) if the lock is busy, conserving CPU cycles.
*   **Condition Variables:** Implements `wait` and `signal` logic to handle complex synchronization scenarios (e.g., Producer-Consumer).
*   **Counting Semaphores:** Implemented resource counting to manage pools of resources (e.g., limiting concurrent access).

### 4. System Stability
*   **Critical Section Protection:** Uses `sigprocmask` to block timer interrupts during queue modifications, ensuring the scheduler does not crash due to race conditions.
*   **Atomic Operations:** Uses `__sync_lock_test_and_set` for thread-safe locking.

---

## 🛠️ File Structure

*   `my_pthread.c`: The core library implementation (Scheduler, API, Context Switching).
*   `my_pthread_t.h`: Header file defining TCB, Mutex, Semaphore structs, and API prototypes.
*   `main.c`: Comprehensive test suite (Producer/Consumer, Preemption, Return Values).
*   `main_advanced.c`: Advanced test suite (Priority Scheduling, Semaphores).
*   `README.md`: Project documentation.

---

## ⚙️ How to Compile & Run

### 1. Basic Test (Preemption, Mutexes, CondVars)
This test demonstrates the Producer-Consumer problem and Timer Interrupts.

```bash
gcc -o os_test main.c my_pthread.c
./os_test 
Expected Output:
Producer signals Consumer.
"Burner" threads print interleaved progress (proving Preemption works).
Main thread successfully joins and retrieves exit codes.
2. Advanced Test (Priority & Semaphores)
This test demonstrates that High Priority threads finish before Low Priority ones, and Semaphores limit resource access.
code
Bash
gcc -o advanced_test main_advanced.c my_pthread.c
./advanced_test
Expected Output:
Priority: The "Rabbit" (High Priority) finishes before the "Turtle" (Low Priority), even if started later.
Semaphores: Only 2 users print at a time; others wait until a spot opens.
🧠 Design Decisions
Context Switching: We used the <ucontext.h> family (getcontext, makecontext, swapcontext) to manage CPU registers and stack pointers in user space.
Queue Design:
Ready Queue: A linked list sorted by Priority (High -> Low).
Wait Queues: Separate linked lists for each Mutex and Condition Variable to hold blocked threads.
Signal Handling: The timer signal handler calls yield(). However, we explicitly block this signal when enqueue() is modifying the linked list to prevent memory corruption (Segfaults).
📝 Authors
[Amam Shamil]
[Mohsin Fawad]
[Soban]
