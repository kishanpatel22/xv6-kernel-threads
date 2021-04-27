// CLONE VM : flag is set when the child process and parent process share same
//            memory region, else child executes is different virtual addresss space
#define CLONE_VM        (1)

#define CLONE_FS        (2)

#define CLONE_FILES     (4)

#define CLONE_THREAD    (8)

#define CLONE_VFORK     (16)
