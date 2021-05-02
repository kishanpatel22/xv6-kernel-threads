// CLONE VM : flag is set when the child process and parent process share same
//            memory region, else child executes is different virtual addresss space
#define CLONE_VM        (1)

// CLONE_FS : flag is set, when the child process clones the file system
//            information from parent process 
#define CLONE_FS        (2)

// CLONE_FILES : flag is set when child process clones the file descripters
//               information from parent process 
#define CLONE_FILES     (4)

// CLONE_THREAD : the child process becomes a thread in the parent process 
//                thread group group
#define CLONE_THREAD    (8)

#define CLONE_PARENT    (16)

// macro that can be used to create thread by passing 
// appropriate flags arguments to clone system call
#define TFLAGS          (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_THREAD)
