#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

/*
    将路径格式化为文件名,即将路径中最后一个\号后的字符串提取出来
*/
char *pathToFilename(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // 将路径中最后一个\号后的字符串提取出来
    for (p = path + strlen(path); p >= path && *p != '/'; p--) // 将p定位到指向最后一个/
        ;
    p++; // p++后,p指向路径对应的文件名
    memmove(buf, p, strlen(p) + 1);
    return buf;
}

/*
    在某路径中查找某文件,主干函数
*/
void find(char *path, char *findName)
{
    int fd;
    struct stat st;
    // open指定的文件(夹),同时检查指定路径是否存在
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "Error: cannot open %s\n", path);
        return;
    }

    // 系统调用fstat用于获取文件(夹)信息,并存入stat结构体
    if (fstat(fd, &st) < 0) // 返回-1说明获取信息失败
    {
        fprintf(2, "Error: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 根据path打开的文件的类型,执行不同的策略
    char buf[512], *p;
    struct dirent de;
    switch (st.type)
    {
    case T_FILE: // 文件类型是文件
        if (strcmp(pathToFilename(path), findName) == 0)
        {
            printf("%s\n", path);
        }
        break;
    case T_DIR: // 文件类型是文件夹
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("Error: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) // 在当前目录下依次读取目录项
        {
            // printf("de.name:%s, de.inum:%d\n", de.name, de.inum);
            if (de.inum == 0 || de.inum == 1 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, strlen(de.name));
            p[strlen(de.name)] = 0;
            // 由于遍历到的是文件夹,所以其内要调用find函数遍历
            find(buf, findName);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    // 判断输入是否正确
    if (argc < 3)
    {
        printf("Error: Must 3 argements! Input format: find <path> <fileName>\n");
        exit(0);
    }
    // 调用find函数
    find(argv[1], argv[2]);
    exit(0);
}
