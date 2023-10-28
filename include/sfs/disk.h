/* disk.h: SimpleFS 磁盘模拟器 */

#ifndef DISK_H
#define DISK_H

#include <stdbool.h>
#include <stdlib.h>

/* 磁盘常量 */

#define BLOCK_SIZE      (1<<12)
#define DISK_FAILURE    (-1)

/* 磁盘结构 */

typedef struct Disk Disk;

struct Disk {
    int	    fd;	        /* 磁盘映像的文件描述符 */
    size_t  blocks;     /* 磁盘映像中的块数 */
    size_t  reads;      /* 从磁盘映像中读取的次数	*/
    size_t  writes;     /*  写入磁盘映像的次数	*/
}; 

/* 磁盘函数 */

Disk *	disk_open(const char *path, size_t blocks);
void	disk_close(Disk *disk);

ssize_t	disk_read(Disk *disk, size_t block, char *data);
ssize_t	disk_write(Disk *disk, size_t block, char *data);

#endif

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
