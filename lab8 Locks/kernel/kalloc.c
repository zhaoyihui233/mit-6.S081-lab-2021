// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

// 为了定义多个kmem，下述代码需要修改
struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void kinit()
{
  // 修改代码
  for (int i = 0; i < NCPU; i++) // 初始化所有的kmem的锁
    initlock(&kmem[i].lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 把pa指向的页表的数据全置为1
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  push_off();                     // 函数cpuid返回当前cpu核编号，但仅在中断关闭时可以调用它并使用它的结果
  int CPUID = cpuid();            // 获取当前CPU编号
  acquire(&kmem[CPUID].lock);     // 上锁
  r->next = kmem[CPUID].freelist; // 与下句一起，把新页表接入空闲页表链
  kmem[CPUID].freelist = r;       // 与上句一起，把新页表接入空闲页表链
  release(&kmem[CPUID].lock);     // 解锁
  pop_off();                      // 与push_off配合使用
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();                       // 函数cpuid返回当前cpu核编号，但仅在中断关闭时可以调用它并使用它的结果
  int CPUID = cpuid();              // 获取当前CPU编号
  acquire(&kmem[CPUID].lock);       // 上锁
  r = kmem[CPUID].freelist;         // 获取空闲页表链头指针
  if (r)                            // r不为0，说明该CPU的空闲页表链不为空
    kmem[CPUID].freelist = r->next; // 空闲页表链长度减一
  else                              // r为0，说明该CPU的空闲页表链为空，需要从其他CPU的空闲页表里“偷”空闲页表
  {
    for (int i = (CPUID + 1) % NCPU; i != CPUID; i = (i + 1) % NCPU)
    {
      if (kmem[i].freelist) // 找到一个还有空闲空间的空闲链表
      {
        acquire(&(kmem[i].lock));   // 分配空闲空间时务必上锁
        r = kmem[i].freelist;       // 分配
        kmem[i].freelist = r->next; // 分配
        release(&(kmem[i].lock));   // 解锁
        break;
      }
    }
  }
  release(&kmem[CPUID].lock); // 解锁
  pop_off();                  // 与push_off配合使用

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
