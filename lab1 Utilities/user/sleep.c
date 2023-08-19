#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        // 把错误信息输出到标准错误流
        fprintf(2, "Error: Must 1 argument for sleep\n");
        exit(1);
    }
    int sleepNum = atoi(argv[1]);
    printf("Sleep for a little while...\n");
    sleep(sleepNum);
    exit(0);
}
