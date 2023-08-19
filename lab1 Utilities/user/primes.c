#include "kernel/types.h"
#include "user/user.h"

// input是剩余要判断的数字的数组
// num是剩余要判断的数字的个数
void findPrimes(int *input, int num)
{
    if (num == 1)
    {
        // num为1说明递归到只剩一个数字了,且该数字一定是质数,输出它并停止递归
        printf("prime %d\n", *input);
        return;
    }
    // p作为管道通信工具
    int p[2];
    // input一开始指向的数字一定是质数(第一次调用函数时指向2,在后续递归调用函数时也都是指向质数)
    int prime = *input;
    int temp;
    printf("prime %d\n", prime);
    // 建立管道
    pipe(p);
    // 下面将建立两个子进程,一个向管道输入待筛选的数字,即input指向之后的数字;另一个进程接收数字,并进行筛选
    // 第一个进程,向管道输入待筛选的数字
    if (fork() == 0)
    {
        for (int i = 0; i < num; i++)
        {
            temp = *(input + i);
            // 向管道输入待筛选的数字
            write(p[1], (char *)(&temp), 4);
        }
        exit(0);
    }
    close(p[1]);
    // 第二个进程,接收第一个进程传送的数据,并筛选,筛去*input(即prime)的倍数,它们肯定不是质数.
    // 没被筛去的数字中,最小的肯定是质数
    if (fork() == 0)
    {
        int counter = 0;
        char buffer[4];
        while (read(p[0], buffer, 4) != 0)
        {
            temp = *((int *)buffer);
            if (temp % prime != 0)
            {
                *input = temp;
                input += 1;
                counter++;
            }
        }
        // input - counter意为让函数第一个参数指向没被筛去的最小的数字,它肯定是整数
        findPrimes(input - counter, counter);
        exit(0);
    }
    wait(0);
    wait(0);
}

int main()
{
    // input是存放2-35，共34个数字的数组
    int input[34];
    int i = 0;
    for (; i < 34; i++)
    {
        input[i] = i + 2;
    }

    // 调用寻找质数的函数
    findPrimes(input, 34);
    exit(0);
}
