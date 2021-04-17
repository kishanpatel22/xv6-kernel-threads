#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}


// sys_clone function copies the arguments to clone from the user process
// address space to the xv6 kernel memory. The function after copying
// the values passes the control 
// Note : xv6 kernel only reads user process arguments on stack.
int sys_clone(void) {
    
    // actual clone call prototype : 
    // clone(int (*fun)(void *), void *child_stack), int flags, void *args)
      
    int (*func)(void *);
    char *child_stack, *args;
    int flags;

    // read the function pointer 
    if(argptr(0, (char **)&(func), 0) == -1) {
        return -1;
    }
    
    // read the child stack address 
    if(argint(1, (int *)&child_stack) == -1) {
        return -1;
    }
    
    // read the flags 
    if(argint(2, &flags) == -1) {
        return -1;
    }
    
    // read the address of argument 
    if(argptr(3, &args, 0) == -1) {
        return -1;
    }

    // call to actual kernel clone function 
    return clone(func, (void *)child_stack, flags, (void *)args);
}

// sys join copies the contents from user stack to the kernel memory 
// sys join calls join which waits for the given thread to be over
int sys_join(void) {

    int tid;
    // read the thread id to be joined 
    if(argint(0, &tid) == -1) {
        return -1;
    }
    return join(tid);
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


