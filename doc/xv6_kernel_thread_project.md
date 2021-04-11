# xv6 kernel thread project 


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


