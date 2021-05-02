#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "cflags.h"

#define eprintf(fmt, ...)                                                   \
    printf(1, fmt ": FAILED"                                                \
              "\nfile = %s, line number = %d, in function = %s()\n"         \
               ##__VA_ARGS__, __FILE__, __LINE__, __func__);                \
    exit();                                                                 \

#define sprintf(fmt, ...)                                                   \
    printf(1, fmt " : PASSED\n");                                           \


#define KB          (1024)
#define MB          (KB * KB)

// ===========================================================================
// ====================== OVERLAP TESTS ======================================
// ===========================================================================

int 
sh_overlap_func(void *agrs) 
{
  sleep(100);
  exit();
}

// the kernel clone system call allocates stacks from KERNEBASE towards size
// while the heap region of the process grows for size to KERNBASE 
// the following code checks if heap and stack overlapp is avoided 
// SOME ASSUMPTIONS FOR THIS TEST :
// 1) KERNBASE is change to 200 MB 
//    (less than PHYSTOP whhich is 222 MB, this ensures enough pages that 
//    can be allocated for crossing stack overlapping heap)
// 2) kernel.ld file is modified accordingly 
// 
// overlap test proc size is 16 KB 
// making first clone will ===>  1 page entry stack + 1 page entry guard
//                         ===>  8 KB
//
// Thus we if malloc = 200 * MB - 16 KB - 8 KB - (4 KB just for stack of second clone)
// there is enough space for one thread to accomodated 
// the second clone system call would fail 

int 
stack_heap_overlap_test()
{
  uint mem_size = 200 * MB - 16 * KB - 8 * KB - 4 *KB;
  int tid1, tid2;
  
  // should be able to allocate the memory 
  void *mem_buffer = malloc(mem_size);
  if(mem_buffer == 0){
    eprintf("malloc failed");
    exit();
  }
  
  // the first thread is created as enough pages are there 
  tid1 = clone(sh_overlap_func, 0, TFLAGS, 0);
  if(tid1 == -1){
    eprintf("stack heap overlap");
    exit();
  }

  // the second thread is not created due to no page allocation 
  tid2 = clone(sh_overlap_func, 0, TFLAGS, 0);
  if(tid2 != -1) {
    eprintf("stack heap overlap");
  }

  join(tid1);
  free(mem_buffer);
  
  sprintf("stack heap overlap");
  // success
  return 0;
}

// ===========================================================================

// infinite loop 
int 
hs_overlap_func(void *args)
{
  while(1){
    ;
  }
  exit();
}

// heap crossing the stack leads to panic 
// see sbrk system call in xv6
int
heap_stack_overlap_test()
{
  void *mem_buffer;
  uint mem_size = 200 * MB - 16 * KB;

  // creating a thread 
  int tid1 = clone(hs_overlap_func, 0, TFLAGS, 0);
    
  // should be able to allocate the thread 
  mem_buffer = malloc(mem_size);
  if(mem_buffer != 0){
    eprintf("heap stack overlap");
    exit();
  }
  tkill(tid1);
  
  sprintf("heap stack overlap");
  // success
  return 0;
}

// ===========================================================================

int
main(int argc, char *argv[])
{
  stack_heap_overlap_test();          // stack crossing heap test
  heap_stack_overlap_test();          // heap crossing stack test
  exit();
}

