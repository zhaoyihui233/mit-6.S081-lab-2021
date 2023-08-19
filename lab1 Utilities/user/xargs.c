#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    int i = 0;
    char *lineSplit[MAXARG];       // lineSplit作为给exec执行argv[1]的参数
    for (int j = 1; j < argc; j++) // 从j=1开始,因为argv[0]=xargs,argv[1]才是要执行的命令
    {
        lineSplit[i++] = argv[j];
    }

    // 下面将标准输入处理后接到lineSplit
    int m = 0, k;
    char block[32];                                 // 标准输入读取到block里
    char buf[32];                                   // 将block的内容先存入buf
    char *p = buf;                                  // 在给lineSplit赋值时，p总指向一个完整的参数
    while ((k = read(0, block, sizeof(block))) > 0) // 循环处理能处理长输入而正常运行,只不过输出可能会出乎意料
    {
        for (int l = 0; l < k; l++)
        {
            // 读取到回车,说明标准输入已全部读取
            if (block[l] == '\n')
            {
                buf[m] = 0;
                m = 0;
                lineSplit[i++] = p;
                p = buf;
                lineSplit[i] = 0; // 最后要加上\0
                i = argc - 1;
                if (fork() == 0) // 子进程调用exec执行argv[1]命令
                {
                    exec(argv[1], lineSplit);
                }
                wait(0);
            }
            else if (block[l] == ' ')
            {
                buf[m++] = 0; // buf[m]设为0,对应的block[m]为空格
                lineSplit[i++] = p;
                p = &buf[m];
            }
            else
            {
                buf[m++] = block[l]; // 将block赋值到buf里
            }
        }
    }
    exit(0);
}
