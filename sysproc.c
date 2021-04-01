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


/* @brief sys_clone function copies the arguments to clone from the user process
 *        address space to the xv6 kernel memory. The function after copying
 *        the values passes the control 
 *
 * @Note  xv6 kernel only reads user process arguments on stack.
 */
int sys_clone(void) {
    /* actaul clone calling convention : 
     * clone(int (*fun)(void *), void *child_stack), int flags, void *args)
     */ 
    int (*func)(void *);
    //void *child_stack, *args;
    //int flags;
    char *fptr;

    if(argptr(0, &fptr, 4) == -1) {
        return -1;
    }
    func = (int (*)(void *))fptr;
    cprintf("%d\n", func);
    
    return -1;
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


