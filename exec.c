#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc(), *p, *tleader, *tleader_parent;
  int thread_exits_in_group;
  
  // thread group leader 
  tleader = THREAD_LEADER(curproc);
  // thread group leader parent  
  tleader_parent = tleader->parent;
 
  acquire(&ptable.lock);
  // make all the threads in group to die (all process with same pid will be killed)
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == curproc->pid && p != curproc){
      p->killed = 1; 
      // some threads might be sleeping wake up them to kill
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
        p->chan = 0;
      }
    }
  }
  release(&ptable.lock);
  
  acquire(&ptable.lock);
  // wait for the threads in the group to die
  // similar to join but happens after killing threads
  for(;;) {
    thread_exits_in_group = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      // the thread has not died
      if(p->pid == curproc->pid && p != curproc && p->state != ZOMBIE){
        thread_exits_in_group = 1;
        break;
      } else if(p->pid == curproc->pid && p != curproc && p->state == ZOMBIE){
        kfree(p->kstack);
        p->kstack = 0;
        if(p->tstackalloc) {
            freecloneuvm(p->pgdir, p->tstack);
        }
        // page directory is not deallocated here 
        p->pgdir = 0; 
        p->pid = 0;
        p->tid = 0;
        p->tstack = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
      }
    }
    // there are no threads in group to wait for 
    if(!thread_exits_in_group) {
        break;
    }
    // sleep untill the excecution 
    sleep(tleader, &ptable.lock);
  }
  // free the stack of thread if it was allocated by kernel
  if(curproc->tstackalloc) {
    freecloneuvm(curproc->pgdir, curproc->tstack);
  }
  release(&ptable.lock);
 
  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;
    
  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));
   
  // the new process is attached earlier group leaders parent 
  curproc->parent = tleader_parent;

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  // thread stack page of new process 
  curproc->tstack = (char *)curproc->sz;
  // thread doing exec is now new process 
  curproc->tid = -1;
  // stack is not allocated since it's not thread
  curproc->tstackalloc = 0;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
