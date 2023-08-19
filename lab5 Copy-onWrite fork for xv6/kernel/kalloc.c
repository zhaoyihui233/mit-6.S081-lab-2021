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

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

// 新增的内容
// reference_count数组用于记录所有页表的引用次数
static int reference_count[(PHYSTOP - KERNBASE) / PGSIZE + 1];

// idx_rc函数用于计算物理地址pa对应的页表在reference_count数组里的索引
static int idx_rc(uint64 pa)
{
  return (pa - KERNBASE) / PGSIZE;
}

// add_rc函数给pa对应的页表的引用次数加一
void add_rc(uint64 pa)
{
  reference_count[idx_rc(pa)]++;
}

// sub_rc函数给pa对应的页表的引用次数减一
void sub_rc(uint64 pa)
{
  reference_count[idx_rc(pa)]--;
}
// 结束

void kinit()
{
  initlock(&kmem.lock, "kmem");
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

  // 引用次数大于1，则不能释放空间，只需要引用次数减一
  if (reference_count[idx_rc((uint64)pa)] > 1)
  {
    sub_rc((uint64)pa);
    return;
  }
  // 否则，释放空间
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  reference_count[idx_rc((uint64)pa)] = 0; // 引用次数归零
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk

  if (r)
    reference_count[idx_rc((uint64)r)] = 1; // 页表刚创建，引用次数置为1
  return (void *)r;
}