struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// implementation of clone system call
int clone(int (*func)(void *args), void *child_stack, int flags, void *args);
// implementation of join system call
int join(int tid);
// kill the thread with given id
int tkill(int tid);
// kills the all threads in thread group 
int tgkill(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

// kthreads user library 
#define KERNEL_STACK_ALLOC      (1)
#define SBRK_STACK_ALLOC        (2)

// change the macro to change the implementation of the library
#define LIB_IMPLEMENTATION      (KERNEL_STACK_ALLOC)

#define KTHREAD_STACK_SIZE      (4096)

enum tstate {NEW, RUNNING, SUSPENDED, DEAD};

typedef struct kthread_t {
    void *tstack;               // thread stack of execution 
    int tid;                    // thread id 
    int state;                  // state of the thread 
} kthread_t;

int create_thread_stack(void **stack);
void destory_thread_stack(void **stack);
int kthread_create(kthread_t *kthread, int func(void *args), void *args);
int kthread_join(kthread_t *kthread);
int kthread_exit()__attribute__((noreturn));

