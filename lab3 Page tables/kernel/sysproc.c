#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;

  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

#ifdef LAB_PGTBL
int sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 addr;
  int len, bitmask;
  // 获取三个参数，并且检查错误输入
  if (argaddr(0, &addr) < 0 || argint(1, &len) < 0 || argint(2, &bitmask) < 0)
    return -1;
  if (len > 32 || len < 0)
    return -1;

  // 查找
  int res = 0;
  struct proc *p = myproc();
  for (int i = 0; i < len; i++)
  {
    int va = addr + i * PGSIZE;
    int abit;
    if (va >= MAXVA)
      abit = 0;
    else
    {
      pte_t *pte = walk(p->pagetable, va, 0);
      if (pte == 0)
        abit = 0;
      else if ((*pte & PTE_A) != 0)
      {
        *pte = *pte & (~PTE_A);
        abit = 1;
      }
      else
        abit = 0;
    }
    res = res | abit << i; // 将结果记录在res的低位起第i位
  }

  // 将结果赋予bitmask
  if (copyout(p->pagetable, bitmask, (char *)&res, sizeof(res)) < 0)
    return -1;

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
