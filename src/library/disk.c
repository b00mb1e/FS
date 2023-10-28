/* disk.c: SimpleFS 磁盘模拟器 */

#include "sfs/disk.h"
#include "sfs/logging.h"

#include <fcntl.h>
#include <unistd.h>

/* 内部属性 */

bool    disk_sanity_check(Disk *disk, size_t blocknum, const char *data);

/* 外部函数 */

/**
 *
 * 通过执行以下操作，打开具有指定路径和指定块数的磁盘：
 *
 *  1. 分配Disk结构并设置适当的属性。
 *
 *  2. 打开指定路径的文件描述符。
 *
 *  3. 将文件截断到所需的文件大小（块数 * BLOCK_SIZE）。
 *
 * @param       path        要创建的磁盘映像的路径。
 * @param       blocks      为磁盘映像分配的块数。
 *
 * @return      指向新分配并配置的Disk结构的指针（失败时为NULL）。
 **/
Disk *disk_open(const char *path, size_t blocks) {

    Disk *disk = (Disk *)malloc(sizeof(Disk));
    if (disk == NULL){
        return NULL;
    }
    
    disk->fd = open(path, O_RDWR|O_CREAT, 0644);
    if (disk->fd == -1)
    {
        free(disk);
        return NULL;
    }
    
    disk->blocks = blocks;
    disk->reads = 0;
    disk->writes = 0;


    off_t file_size = blocks * BLOCK_SIZE;

    if (truncate(path, file_size) == -1)
    {
        close(disk->fd);
        free(disk);
        return NULL;
    }

    return disk;
}

/**
 * 关闭磁盘结构，执行以下操作：
 *
 *  1. 关闭磁盘文件描述符。
 *
 *  2. 报告磁盘读取和写入的次数。
 *
 *  3. 释放磁盘结构的内存。
 *
 * @param       disk        指向Disk结构的指针。
 */
void disk_close(Disk *disk) {
    if (disk == NULL)
        return;

    close(disk->fd);

    printf("Number of reads: %zu\n", disk->reads);
    printf("Number of writes: %zu\n", disk->writes);
    
    free(disk);

}

/**
 * 从磁盘中读取指定块的数据到数据缓冲区，执行以下操作：
 *
 *  1. 执行合法性检查。
 *
 *  2. 定位到指定的块。
 *
 *  3. 从块中读取数据到数据缓冲区（必须为BLOCK_SIZE）。
 *
 * @param       disk        指向Disk结构的指针。
 * @param       block       要执行操作的块编号。
 * @param       data        数据缓冲区。
 *
 * @return      读取的字节数。
 *              （成功时为BLOCK_SIZE，失败时为DISK_FAILURE）。
 **/
ssize_t disk_read(Disk *disk, size_t block, char *data) {
    
    if (!disk_sanity_check(disk, block, data))
    {
        return DISK_FAILURE;
    }
    

    off_t offset = (off_t)BLOCK_SIZE * block;

    if (lseek(disk->fd, offset, SEEK_SET) == -1){
        return DISK_FAILURE;
    }

    ssize_t bytes_read = read(disk->fd, data, BLOCK_SIZE);

    if (bytes_read == -1)
    {
        return DISK_FAILURE;
    }

    disk->reads ++;
    
    return bytes_read;

    
}

/**
 * 将数据从数据缓冲区写入磁盘中的指定块，执行以下操作：
 *
 *  1. 执行合法性检查。
 *
 *  2. 定位到指定的块。
 *
 *  3. 将数据缓冲区（必须为BLOCK_SIZE）写入磁盘块。
 *
 * @param       disk        指向Disk结构的指针。
 * @param       block       要执行操作的块编号。
 * @param       data        数据缓冲区。
 *
 * @return      写入的字节数。
 *              （成功时为BLOCK_SIZE，失败时为DISK_FAILURE）。
 **/
ssize_t disk_write(Disk *disk, size_t block, char *data) {

    if (!disk_sanity_check(disk, block, data))
    {
        return DISK_FAILURE;
    }
    

    off_t offset = (off_t)BLOCK_SIZE * block;

    if (lseek(disk->fd, offset, SEEK_SET) == -1){
        return DISK_FAILURE;
    }
    

    
    ssize_t bytes_written = write(disk->fd, data, BLOCK_SIZE);

    if (bytes_written == -1)
    {
        return DISK_FAILURE;
    }
    


    disk->writes++;

    return bytes_written;
}

/* 内部函数 */

/**
 * 在执行读取或写入操作之前进行合法性检查：
 *
 *  1. 检查磁盘是否有效。
 *
 *  2. 检查块是否有效。
 *
 *  3. 检查数据是否有效。
 *
 * @param       disk        指向Disk结构的指针。
 * @param       block       要执行操作的块编号。
 * @param       data        数据缓冲区。
 *
 * @return      是否安全执行读取/写入操作（安全为true，不安全为false）。
 **/
bool disk_sanity_check(Disk *disk, size_t block, const char *data) {

    if (disk == NULL || data == NULL || block >= disk->blocks || block < 0)
        return false;

    return true;

}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
