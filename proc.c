#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "cflags.h"

// ptable external varaible 
struct table ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
    
  // by default the process is not thread
  p->tid = -1;
  
  // initializing the stack for the process 
  p->tstack = 0;

  // the stack is not allocated by kernel
  p->tstackalloc = 0;
    
  // initializes the sleep for 
  initlock(&(p->tlock), "thread lock");

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S
  
  // thread stack of the init process 
  p->tstack = (char *)PGSIZE;

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc(), *tleader;
  tleader = THREAD_LEADER(curproc);

  // page directory is shared but actual size of process is with group leader
  sz = tleader->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
    
  // update the thread size 
  curproc->sz = sz;
  // update the thread leader size 
  tleader->sz = sz;
   
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  void *eip = (void *)(myproc()->tf->eip);
  return clone(eip, 0, CLONE_FS | CLONE_FILES, 0); 
}

// clone system call implementation for xv6 
// The clone child process shares the same virtual 
// address as of the parent process except the
// stack frame of the child process 
// clone returns the new thread id of the child
int 
clone(int (*func)(void *args), void *child_stack, int flags, void *args) 
{
  struct proc *np;
  struct proc *curproc = myproc(), *tleader;
  char *guard_page;
  int retid;
  uint sp, stack_args[2];
      
  // modify or duplicate all the fileds of struct proc accordingly 
  // to create a child process which is clone of the current process 
  if((np = allocproc()) == 0){
    return -1;
  }

  // thread leader in the group of threads executed with same pid
  tleader = THREAD_LEADER(curproc);
  
  // size of the child clone is same as parent 
  np->sz = tleader->sz;
    
  // the parent of the child process 
  if((flags & CLONE_PARENT)){
    np->parent = tleader->parent;
  } else{
    np->parent = tleader;
  }
  
  // virtual address space 

  // child and parent sharing same virtual address space 
  if(flags & CLONE_VM){
    // page directory will be same, since child shares virtual memory 
    np->pgdir = tleader->pgdir; 
  } 
  // child and parent not sharing same virtual address space
  else{
    // Copy out text / data + heap + stack region of process from thread leader
    if((np->pgdir = copyuvm(tleader->pgdir, tleader->sz)) == 0){
      kfree(np->kstack);
      np->kstack = 0;
      np->state = UNUSED;
      return -1;
    }
  }
  
  // child stack for execution 
  
  // child process sharing the virtual address space 
  if((CLONE_VM & flags)){
    // the child process needs stack for execution 
    if(!child_stack){
      // guard page of the group leader thread 
      guard_page = tleader->tstack - 2 * PGSIZE;
      // modify the page directory entry by extending the virtual address 
      if((np->tstack = cloneuvm(tleader->pgdir, tleader->sz, guard_page)) == 0){
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1; 
      }
      // the stack is allocated by the kernel 
      np->tstackalloc = 1;
    } 
    // allocated stack for child process 
    else{
      np->tstack = (char *)child_stack;
    }
  }
  // child process not sharing the virtual address space
  else{
    np->tstack = tleader->tstack;
    // thread trying to clone without sharing any virtual address space 
    if(curproc != tleader){
      if(copy_thread_stack(np->pgdir, np->tstack - PGSIZE, 
                           tleader->pgdir, curproc->tstack - PGSIZE) == -1) {
        return -1; 
      }
    }
  }

  // context of parent for child process 
 
  // trap frame 
  *np->tf = *curproc->tf;
  
  // sharing of virtual address space 
  if((flags & CLONE_VM)){
    // build the handcrafted stack frame for the function 
    stack_args[0] = (uint)0xffffffff;   // 4 bytes fake instruction pointer  
    stack_args[1] = (uint)args;         // 4 bytes of the argument pointer 
    
    // point the sp to the child stack
    sp = (uint)np->tstack;
    sp -= 2 * 4;
 
    // add the return address and argument pointer on the stack 
    if(copyout(np->pgdir, sp, stack_args, 2 * sizeof(uint)) == -1){
      kfree(np->kstack);
      np->kstack = 0;
      np->state = UNUSED;
      return -1;
    }
  } 
  // not sharing the same virtual address space 
  else{
    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;
    if(curproc != tleader){
      np->tf->esp = (uint)np->tstack - ((uint)curproc->tstack - np->tf->esp);
    }
    sp = np->tf->esp;
  } 
  
  // change of execution point depending upon the flag passed 
  np->tf->eip = (uint)func;           // change instruction pointer for execution 
  np->tf->esp = sp;                   // change stack pointer for execution 
  
  for(uint i = 0; i < NOFILE; i++){
    if(curproc->ofile[i]){
      // duplicate all the file descripters 
      if((flags & CLONE_FILES)){
        np->ofile[i] = filedup(curproc->ofile[i]);
      } else{
        np->ofile[i] = filealloc();
        np->ofile[i]->type = curproc->ofile[i]->type;
        np->ofile[i]->ip = idup(curproc->ofile[i]->ip);
        np->ofile[i]->off = 0;
        np->ofile[i]->readable = curproc->ofile[i]->readable;
        np->ofile[i]->writable = curproc->ofile[i]->writable;
      }
    }
  }
  
  if((flags & CLONE_FS)){
    np->cwd = idup(curproc->cwd);
  } else{
  }
  
  // name of the child process is same as that of the original process 
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));
  
  if((flags & CLONE_THREAD)){
    // child process formed would be thread, thus pid is same as thread id
    np->tid = np->pid;
    
    // pid of the clone child process is same as the parent process 
    np->pid = curproc->pid;
    
    // return id is the thread id 
    retid = np->tid;
  } else{
    
    // the child process is thread leader in different thread group
    np->tid = -1;
    
    // return id is the process id
    retid = np->pid;
  }
  
  acquire(&ptable.lock);
  
  // child process state is set to runnable for scheduling
  np->state = RUNNABLE;
  
  release(&ptable.lock);

  // returns the thread id of the child process 
  return retid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;
    
  if(curproc == initproc)
    panic("init exiting");
    
  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }
  
  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;
  
  // thread group leader doing exit kills all the threads in group.
  if(curproc->tid == -1) {
    tgkill();
  }

  acquire(&ptable.lock);

  // thread doing exits informs the group leader
  if(curproc->tid != -1){
    // wake up the thread group leader as well
    wakeup1(THREAD_LEADER(curproc));
  } 

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);
  
  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    // only process are passed to init
    if(p->parent == curproc && p->tid == -1){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  } 
  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(THREAD_LEADER(p)->parent != THREAD_LEADER(curproc))
        continue;
      havekids = 1;
      if(p->state == ZOMBIE && p->tid == -1){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->tid = 0;
        p->tstack = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }
    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(THREAD_LEADER(curproc), &ptable.lock);  //DOC: wait-sleep
  }
}


// join i.e wait untill the execution of the thread with the given tid 
// join returns 0 in case of success otherwise returns -1
// Note that thread to be joined and thread calling join must belong to same group
int 
join(int tid) 
{
  struct proc *p, *curproc = myproc(), *tleader;
  int join_thread_exits, jtid;
  
  // cannot join any process 
  if(tid == -1){
    return -1;
  }
  
  tleader = THREAD_LEADER(curproc);

  join_thread_exits = 0;
  // check if the thread joining the tid both belong to same thread group
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->tid == tid && p->parent == tleader) {
      join_thread_exits = 1; 
      break;
    }
  }

  // join thread either doesn't exists or it doesn't belong to same group
  if(!join_thread_exits || curproc->killed){
    return -1;
  }
  
  acquire(&ptable.lock);

  // suspend execution of current thread and wait for completion of tid thread
  for(;;){
    // thread is killed by some other thread in group
    if(curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    if(p->state == ZOMBIE){
      // Found the thread 
      jtid = p->tid;
      kfree(p->kstack);
      p->kstack = 0;
      if(p->tstackalloc){
          freecloneuvm(p->pgdir, p->tstack);
      }
      p->pgdir = 0;
      p->pid = 0;
      p->tid = 0;
      p->tstack = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      p->state = UNUSED;
      release(&ptable.lock);
      return jtid;
    } 
    // Wait for thread to complete (See wakeup1 call in proc_exit.)
    sleep(tleader, &ptable.lock);  
  }     
  return -1;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
    
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
       
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
    
  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid && p->tid == -1){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// kills thread with given thread id
// system call doesn't block returns immediately
int
tkill(int tid) 
{
  struct proc *curproc = myproc(), *p;
  int kill_thread_exits;

  // cannot kill the main thread 
  if(tid == -1)
    return -1;
    
  kill_thread_exits = 0;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    // threads share same pid
    if(p->pid == curproc->pid && p->tid == tid){
      p->killed = 1;  
      // wakeup the process to kill 
      if(p->state == SLEEPING){
        p->state = RUNNABLE; 
      }
      kill_thread_exits = 1;
      break;
    }  
  }
  release(&ptable.lock);
  
  if(!kill_thread_exits){
    return -1;
  }
  return 0;
}

// process exit must call tgkill, before becoming ZOMBIE
// tgkill kills all the thread present in the thread group
// should only be called by functions holding pagetable locks
int 
tgkill(void) 
{
  struct proc *curproc = myproc(), *p;
  int havethreads;
  
  // only thread leader can kill threads in group 
  if(THREAD_LEADER(curproc) != curproc){
    return -1;
  }
      
  acquire(&ptable.lock);

  // make all the threads in group to die (all process with same pid will be killed)
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == curproc->pid && p->tid != -1){
      p->killed = 1; 
      // some threads might be sleeping wake up them to kill
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
        p->chan = 0;
      }
    }
  }
  // now let all the threads finish and wait for them become zombie
  for(;;){
    havethreads = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pid != curproc->pid || p->tid == -1)
        continue;
      // thread in group has already died
      if(p->state == ZOMBIE){
        kfree(p->kstack);
        p->kstack = 0;
        if(p->tstackalloc){
          freecloneuvm(p->pgdir, p->tstack);
        }
        p->pid = 0;
        p->tid = 0;
        p->tstack = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
      } 
      // thread in group is not died yet so suspend untill it dies.
      else {
        havethreads = 1; 
        break;
      }
    }
    // group leader doesn't have any threads 
    if(!havethreads){
        break;
    } 
    // the thread leader gets killed
    if(curproc->killed) {
      release(&ptable.lock);
      return -1;
    }
    // sleep for an exisiting thread in group to be killed
    sleep(curproc, &ptable.lock);
  }

  release(&ptable.lock);
  // successfully killed all threads in group
  return 0;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %d %s %s", p->pid, p->tid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// suspends execution of thread (thread goes on self sleep)
// system allows the thread to sleep 
int
tsuspend(void)
{
    struct proc *curproc = myproc();
    // cannot suspend the main thread leader which the process 
    if(curproc == THREAD_LEADER(curproc)) {
        return -1;
    }

    // thread is suspended and it sleeps
    acquire(&ptable.lock);
    sleep(curproc, &ptable.lock); 
    release(&ptable.lock);
    
    return 0;
}


// resumes exeuction of thread with given tid
// system call allows thread to wake up another thread
int
tresume(int tid)
{
    struct proc *curproc = myproc(), *p;
    int resume_thread_exits;

    // main leader thread cannot be resumed 
    if(tid == -1){
        return -1;
    }
    
    resume_thread_exits = 0;
    // check if thread belongs to same thread group
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->tid == tid && p->pid == curproc->pid){
            resume_thread_exits = 1;
            break;
        }
    }

    // the thread with tid doesn't exits in group
    // or the current thread has been killed 
    if(!resume_thread_exits || curproc->killed){
        return -1;
    }
    
    // make the thread runnable again 
    acquire(&ptable.lock);
    p->state = RUNNABLE;
    p->chan = 0;
    release(&ptable.lock);

    return 0;
}


