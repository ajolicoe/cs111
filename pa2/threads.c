//
// threads.c
//
// Created by Scott Brandt on 5/6/13.
// Modified by Nicholas Brower     <nbrower>
//             Christopher Selling <cselling>
//             Alex Jolicoeur      <ajolicoe>
//
// Implementation of a user level thread package,
// with a timed priority lottery scheduler.

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

#define NUM_THREADS    20
#define NUM_TICKETS    100
#define LOTTERY_SIZE   1000
#define STACK_SIZE     8192
#define TIMER_INTERVAL 250000

static ucontext_t context[NUM_THREADS];
int tickets[NUM_TICKETS];
int lottery[LOTTERY_SIZE];

static void test_thread(void);
static int thread = 0;
void thread_exit(int);

// This is the main thread. In a real program, it should probably start
// all of the threads and then wait for them to finish without doing any
// "real" work
void thread_yield(int);
struct itimerval watchdog;

int main(void) {
   srand(time(NULL));
   printf("Main starting\n");
   printf("Main calling thread_create\n");

   // create the other threads
   int i;
   for(i = 0; i < NUM_THREADS; i++){
      thread_create(&test_thread);
   }
   for(i = 0; i < LOTTERY_SIZE; i++) {
      lottery[i] = i % NUM_THREADS;
   }
   // set timer
   struct itimerval watchdog;
   watchdog.it_interval.tv_sec = 0;
   watchdog.it_interval.tv_usec = 0;
   watchdog.it_value.tv_sec = 0;
   watchdog.it_value.tv_usec = TIMER_INTERVAL;

   // set signal
   setitimer(ITIMER_REAL, &watchdog,0);
   signal(SIGALRM, thread_yield);
   printf("Main returned from thread_create\n");

   // Loop, doing a little work then yielding to the other thread
   while(1) {
      printf("start switching between %d threads\n", NUM_THREADS);
      // thread_yield() will be called by timer
   }
   // We should never get here
   exit(0);
}

// This is the thread that gets started by thread_create
static void test_thread(void) {
   printf("In test_thread\n");
   // Loop, doing a little work then yielding to the other thread
   while(1) {
      printf("Thread %d in test_thread\n", thread);
      // thread_yield() will be called by timer
   }
   // thread_exit(0);
}

// Yield to another thread
void thread_yield(int unused) {
   int old_thread = thread;

   // This is the lottery scheduler
   thread = lottery[rand() % LOTTERY_SIZE]; // lottery picks a new thread to run

   struct itimerval watchdog;
   signal(SIGALRM, thread_yield); //set up signal for new thread
   watchdog.it_interval.tv_sec = 0;
   watchdog.it_interval.tv_usec = 0;
   watchdog.it_value.tv_sec = 0;
   watchdog.it_value.tv_usec = TIMER_INTERVAL;

   printf("Thread %d yielding to thread %d\n", old_thread, thread);
   printf("Thread %d calling swapcontext\n", old_thread);

   // This will stop us from running and restart the other thread
   setitimer(ITIMER_REAL, &watchdog, 0);
   swapcontext(&context[old_thread], &context[thread]);

   // The other thread yielded back to us
   printf("Thread %d back in thread_yield\n", thread);
}

// Create a new thread
int thread_create(int (*thread_function)(void)) {
   int newthread = (++thread) % NUM_THREADS;

   printf("Thread %d in thread_create\n", thread);
   printf("Thread %d calling getcontext and makecontext\n", thread);

   // First, create a valid execution context the same as the current one
   getcontext(&context[newthread]);

   // Now set up its stack
   context[newthread].uc_stack.ss_sp = malloc(STACK_SIZE);
   context[newthread].uc_stack.ss_size = STACK_SIZE;

   // This is the context that will run when this thread exits
   context[newthread].uc_link = &context[thread];

   // Now create the new context and specify what function it should run
   makecontext(&context[newthread], test_thread, 0);
   printf("Thread %d done with thread_create\n", thread);
}

// This doesn't do anything at present
void thread_exit(int status) {
   printf("Thread %d exiting\n", thread);
}
