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
// gets id of currently executing thread 
int gettid(void);
// thread suspending it's exceution
int             tsuspend(void);
// make thread resume it's execution 
int             tresume(int tid);

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

// userland spin locks : atomic synchronization primitives
typedef struct splock {
    uint locked; 
} splock;

void init_splock(splock* s);
void acquire_splock(splock* s);
void release_splock(splock* s);

// semaphore implementation for xv6

// queue of threads suspended from execution

#define MAX_QUEUE_SIZE  (1024)

typedef struct queue {
    int arr[MAX_QUEUE_SIZE];
    int front, rear;
} queue;

void init_queue(queue *);
int isempty(queue q);
int enqueue(queue *, int);
int dequeue(queue *);

typedef struct semaphore {
    int val;
    queue q;
    splock sl;
} semaphore;

void semaphore_init(semaphore *s, int initval);
void semaphore_wait(semaphore *s);
void semaphore_signal(semaphore *s);
void block(semaphore *s);

