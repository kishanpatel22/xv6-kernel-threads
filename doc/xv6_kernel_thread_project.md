# xv6 kernel threads project 

* The project involves implementation of kernel threads for xv6 operating
  system. The project aims in deeper understanding of clone and join system
  calls incoporating the idea of shared virtual memory between the threads.

* What are threads ? 
    + The bookish defination -> "lightweight processes"
    + Some hardware definations -> "stream of instructions"
    + My defination -> thread is process which uses the concept of shared
      memory region, which makes it light weight. The threads execution is
      equivalent to some execution of routine / function and thus called
      as stream of instructions.

## Implementation Details 

* The **struct proc** in xv6 which represents structure for **Process Control
  Block** (PCB) is used for representing threads as well. This is due to fact that,
  threads have indivual context of execution, kernel stack for system calls, 
  list of open files array, state, file system information, etc.

* The **struct proc** in xv6 is modified accordingly to distinguish between the 
  thread and process with the given fields as shown below.
  
  ```c
    struct proc {
        
        // ....

        int tid;                // Thread ID
        char *tstack;           // Thread execution stack
        int tstackalloc;        // If non-zero, stack is allocated by kernel

        // ...
    }
  ```

## Creating threads : CLONE system call

* The concept of creating the threads, makes use of the clone system call
  functionality.







    ```c
        void foo(void *args) {
            /* ... */
            return a * b;
        }
    ```

* The above code results into trap 14, after successful execution for the above
  thread executing the foo function. The error is due to the return statement

* trap number 14 corresponds to page fault, which occurs when the page table 
  entry for the corresponding address is not set.

* When you return the instruction pointer is set some **fake pc**, remember
  this was set by the clone system call, while creating handcrafted stack arguments.
  Thus the fake pc in return statement doesn't exits in the page table, and 
  thus leads to the page fault trap for the thread process.




