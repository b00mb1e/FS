/* fs.h: SimpleFS 文件系统 */

#ifndef FS_H
#define FS_H

#include "sfs/disk.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* 文件系统常量 */

#define MAGIC_NUMBER        (0xf0f03410)
#define INODES_PER_BLOCK    (128)               /* TODO: 每个块中的inode数 */
#define POINTERS_PER_INODE  (5)                 /* TODO:  每个inode的直接指针数  */
#define POINTERS_PER_BLOCK  (1024)              /* TODO: 每个块中的指针数 */

/* 文件系统结构 */

typedef struct SuperBlock SuperBlock;
struct SuperBlock {
    uint32_t    magic_number;                   /* 文件系统魔数 */
    uint32_t    blocks;                         /* 文件系统中的块数 */
    uint32_t    inode_blocks;                   /* 用于存储inode的保留块数 */
    uint32_t    inodes;                         /* 文件系统中的inode的个数 */
};

typedef struct Inode      Inode;
struct Inode {
    uint32_t    valid;                          /* inode是否有效 */
    uint32_t    size;                           /* 文件大小 */
    uint32_t    direct[POINTERS_PER_INODE];     /* 直接指针 */
    uint32_t    indirect;                       /* 间接指针 */
};

typedef union  Block      Block;
union Block {
    SuperBlock  super;                          /* 将块视为超级块 */
    Inode       inodes[INODES_PER_BLOCK];       /* 将块视为inode表 */
    uint32_t    pointers[POINTERS_PER_BLOCK];   /* 将块视为指针 */
    char        data[BLOCK_SIZE];               /* 将块视为数据 */
};

typedef struct FileSystem FileSystem;
struct FileSystem {
    Disk        *disk;                          /* 挂载文件系统的磁盘 */
    bool        *free_blocks;                   /* 空闲块位图  */
    SuperBlock   meta_data;                     /* 文件系统元数据 */
};

/* 文件系统函数 */

void    fs_debug(Disk *disk);
bool    fs_format(FileSystem *fs, Disk *disk);

bool    fs_mount(FileSystem *fs, Disk *disk);
void    fs_unmount(FileSystem *fs);

ssize_t fs_create(FileSystem *fs);
bool    fs_remove(FileSystem *fs, size_t inode_number);
ssize_t fs_stat(FileSystem *fs, size_t inode_number);

ssize_t fs_read(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset);
ssize_t fs_write(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset);

#endif

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
