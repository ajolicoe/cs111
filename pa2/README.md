Purpose:
This document specifies the design of a user level thread package.

Data:
1. context - the context of each thread created
   - represented as an array of ucontext_t, length of number of threads

2. lottery - lottery tickets for each thread
   - represented as an array of integers corresponding to a thread's ID
   - the size of the lottery can be changed, the data of the array is some
     combination of thread IDs.

Operations:

1. static void test_thread(void)
   - Description: test function for each thread to run
   - Input: 	  NONE
   - Output:      NONE
   - Result:      executes a loop printing out the current thread's ID until
   	          our timer goes off and thread_yield() is called.
   	
2. int thread_yield()
   - Description: yield current thread to another thread chosen by our scheduler
   - Input:       NONE
   - Output:      NONE
   - Result:      a new thread is chosen at random from our lottery array, then
   		  we setup the timer and signal for this new thread, and swap it's
   		  context with that of the old thread.
   		  
3. int thread_create(int (*thread_function)(void))
   - Description: create a new thread, and give it some function to execute.
   - Input: 	  the thread function to be run, in this case it is test_thread().
   - Output:      NONE
   - Result:      create a new execution context same as the current, setup it's
   		  stack and link, and then create the new context and set the test
   		  function it should run.
   		  
4. int thread_exit(int status) 
   - Description: exit the current thread
   - Input: 	  exit status
   - Output:      NONE
   - Result:      still does nothing, and is unused in our program.

Algorithms:

Main algorithm:
   1. create the threads
   2. fill the lottery array
   3. setup the timer
   4. set the signal to thread_yield for the thread
   5. loop until timer causes thread_yield to be called
   
test_thread:
   1. loop, printing current thread ID until timer causes thread_yield to be called.
   
thread_yield:
   1. set old thread to current thread
   2. choose a random thread from the lottery array (this is our scheduler)
   3. set the timer for the new thread
   4. set the signal to thread_yield for the new thread
   5. swap the context of the old thread, with that of the new thread.

thread_create:
   1. next thread is (current thread + 1 % NUM_THREADS)
   2. create a valid execution context for this next thread (same as the current thread)
   3. setup the new thread's stack
   4. set the new thread's uc_link to the current thread
   5. make the new context and set the test function (test_thread)
   
