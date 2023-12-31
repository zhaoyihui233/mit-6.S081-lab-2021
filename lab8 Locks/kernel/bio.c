// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct
{
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

// 利用哈希表解决锁竞争
#define NBUCKET 17      // number of hashing buckets
#define NBUF_PER_BUC 20 // number of available buckets per bucket

extern uint ticks; // system time clock

// 我们要维护的结构体数组
struct
{
  struct spinlock lock;
  struct buf buf[NBUF_PER_BUC];
} bcacheBucket[NBUCKET];

void binit(void)
{
  // struct buf *b;

  // initlock(&bcache.lock, "bcache");

  // // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  // {
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  for (int i = 0; i < NBUCKET; i++)
  {
    initlock(&bcacheBucket[i].lock, "bcachebucket");
    for (int j = 0; j < NBUF_PER_BUC; j++)
      initsleeplock(&bcacheBucket[i].buf[j].lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  // struct buf *b;

  // acquire(&bcache.lock);

  // // Is the block already cached?
  // for (b = bcache.head.next; b != &bcache.head; b = b->next)
  // {
  //   if (b->dev == dev && b->blockno == blockno)
  //   {
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for (b = bcache.head.prev; b != &bcache.head; b = b->prev)
  // {
  //   if (b->refcnt == 0)
  //   {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // panic("bget: no buffers");

  struct buf *b;
  int bucket = blockno % NBUCKET;
  acquire(&bcacheBucket[bucket].lock); // 只对该bucket上锁

  for (int i = 0; i < NBUF_PER_BUC; i++) // 在缓冲区里寻找指定磁盘块
  {
    b = &bcacheBucket[bucket].buf[i];
    if (b->dev == dev && b->blockno == blockno) // 利用dev与blockno两个字段寻找
    {
      b->refcnt++;                         // 引用次数增加
      b->lastUse = ticks;                  // 更新上次使用时间
      release(&bcacheBucket[bucket].lock); // 解锁
      acquiresleep(&b->lock);
      return b; // 返回块
    }
  }

  // 指定的块不在缓存区，下面检查缓冲区是否有空闲空间
  uint least = 0xffffffff; // 这个是最大的unsigned int
  int least_idx = -1;
  for (int i = 0; i < NBUF_PER_BUC; i++)
  {
    b = &bcacheBucket[bucket].buf[i];
    if (b->refcnt == 0 && b->lastUse < least) // 查看cache是否有剩余空间
    {
      least = b->lastUse; // 若有，则分配该空间
      least_idx = i;
    }
  }

  if (least_idx == -1) // 说明没有空闲缓存了
  {
    // 这里可以添加去其他bucket偷取空闲buffer的代码,但是比较复杂
    panic("bget: no unused buffer for recycle");
  }

  // 将磁盘块分配到空闲位置
  b = &bcacheBucket[bucket].buf[least_idx];
  b->dev = dev;
  b->blockno = blockno;
  b->lastUse = ticks;
  b->valid = 0;
  b->refcnt = 1;
  release(&bcacheBucket[bucket].lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  // if (!holdingsleep(&b->lock))
  //   panic("brelse");

  // releasesleep(&b->lock);

  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0)
  // {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }

  // release(&bcache.lock);
  if (!holdingsleep(&b->lock))
    panic("brelse");

  int bucket = b->blockno % NBUCKET;
  acquire(&bcacheBucket[bucket].lock);
  b->refcnt--;
  release(&bcacheBucket[bucket].lock);
  releasesleep(&b->lock);
}

void bpin(struct buf *b)
{
  int bucket = b->blockno % NBUCKET;
  acquire(&bcacheBucket[bucket].lock);
  b->refcnt++;
  release(&bcacheBucket[bucket].lock);
}

void bunpin(struct buf *b)
{
  int bucket = b->blockno % NBUCKET;
  acquire(&bcacheBucket[bucket].lock);
  b->refcnt--;
  release(&bcacheBucket[bucket].lock);
}
