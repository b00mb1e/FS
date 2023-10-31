/* fs.c: SimpleFS 文件系统 */

#include "sfs/fs.h"
#include "sfs/logging.h"
#include "sfs/utils.h"

#include <stdio.h>
#include <string.h>

/* 外部函数 */

/**
 * 通过执行以下操作来调试文件系统：
 *
 *  1. 读取超级块并报告其信息。
 *
 *  2. 读取inode表并报告每个i节点的信息。
 *
 * @param       disk        指向Disk结构的指针。
 **/
void fs_debug(Disk *disk) {
    Block block;

    /* 读取超级块 */
    if (disk_read(disk, 0, block.data) == DISK_FAILURE) {
        return;
    }


    printf("SuperBlock:\n");
    printf("    %u blocks\n"         , block.super.blocks);
    printf("    %u inode blocks\n"   , block.super.inode_blocks);
    printf("    %u inodes\n"         , block.super.inodes);

    /* 读取inode表 */
    printf("\nInode Table:\n");
    for (size_t block_number = 1; block_number <= block.super.inode_blocks; block_number ++ ){
        if (disk_read(disk, block_number, block.data) == DISK_FAILURE){
            return;
        }

        Inode *inodes = block.inodes;

        for (size_t i = 0; i < INODES_PER_BLOCK; i++){
            if (inodes[i].valid == 1){
                printf("Inode %d:\n", i + (block_number - 1) * INODES_PER_BLOCK);
                printf("    File size: %u bytes\n", inodes[i].size);
                printf("    Direct pointers: ");
                for (size_t j = 0; j < POINTERS_PER_INODE; j++){
                    printf("%u ", inodes[i].direct[j]);
                }
                printf("\n");
                printf("    Indirect pointers: %u\n", inodes[i].indirect);
                printf("\n");
            }
            
        }
        
        
    }

    

}

/**
 * 格式化磁盘，执行以下操作：
 *
 *  1. 写入超级块（具有适当的魔数、块数、inode块数和inode数）。
 *
 *  2. 清除所有其余的块。
 *
 * 注意：不要格式化已挂载的磁盘！
 *
 * @param       fs      指向FileSystem结构的指针。
 * @param       disk    指向Disk结构的指针。
 * @return      所有磁盘操作是否成功（成功为true，失败为false）。
 **/
bool fs_format(FileSystem *fs, Disk *disk) {
    
    if (fs == NULL || disk == NULL) return false;
    if (fs->disk == disk){
        return false;
    }
    
    Block super_block;
    memset(&super_block, 0, sizeof(super_block));
    super_block.super.magic_number = MAGIC_NUMBER;
    super_block.super.blocks = disk->blocks;
    super_block.super.inode_blocks = (disk->blocks + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
    super_block.super.inodes = super_block.super.inode_blocks * INODES_PER_BLOCK;


    if (disk_write(disk, 0, super_block.data) == DISK_FAILURE)
        return false;
    
    Block empty_block;
    memset(&empty_block, 0, sizeof(Block));
    for (size_t block_number = 1; block_number < disk->blocks; block_number++)
        if (disk_write(disk, block_number, empty_block.data) == DISK_FAILURE)
            return false;

 
    
    return true;
}

/**
 * 将指定的文件系统挂载到给定的磁盘上，执行以下操作：
 *
 *  1. 读取和检查超级块（验证属性）。
 *
 *  2. 验证和记录文件系统磁盘属性。
 *
 *  3. 复制超级块到文件系统元数据属性。
 *
 *  4. 初始化文件系统的空闲块位图。
 *
 * 注意：不要挂载已经挂载过的磁盘！
 *
 * @param       fs      指向FileSystem结构的指针。
 * @param       disk    指向Disk结构的指针。
 * @return      挂载操作是否成功（成功为true，失败为false）。
 **/
bool fs_mount(FileSystem *fs, Disk *disk) {
    if (fs == NULL || disk == NULL){
        return false;
    }
    
    if (fs->disk == disk)
    {
        return false;
    }
    fs->disk = disk;

    Block super_block;
    memset(&super_block, 0, sizeof(super_block));


    if (disk_read(disk, 0, super_block.data) == DISK_FAILURE)
        return false;
   
    

    
    memcpy(&(fs->meta_data), super_block.data, sizeof(SuperBlock));
    fs->free_blocks = (bool *)malloc(fs->meta_data.blocks * sizeof(bool));

    if (fs->free_blocks == NULL)
        return false;
    
    for (size_t i = 0; i < fs->meta_data.blocks; i++)
        fs->free_blocks[i] = true;
    
    
    for (size_t i = 0; i <= fs->meta_data.inode_blocks; i++)
        fs->free_blocks[i] = false;
    
    
    for (size_t block_number = 1; block_number <= fs->meta_data.inode_blocks; block_number++){
        
        Block inode_block;

        if (disk_read(fs->disk,block_number, inode_block.data) == DISK_FAILURE){
            return false;
        }
        
        Inode *inodes = inode_block.inodes;

        for (int i = 0; i < INODES_PER_BLOCK; i ++ )
            if (inodes[i].valid == 1)
            {    
                for (size_t j = 0; j < POINTERS_PER_INODE; j ++ )
                    if (inodes[i].direct[j] != 0){
                        fs->free_blocks[inodes[i].direct[j]] = false;
                    }

                if (inodes[i].indirect != 0)
                {
                    fs->free_blocks[inodes[i].indirect] = false;
                    
                    Block indirect_block;
                    if (disk_read(fs->disk, inodes[i].indirect, indirect_block.data) == DISK_FAILURE)
                        return false;
                    
                    for (size_t j = 0; j < POINTERS_PER_BLOCK; j++){
                        size_t pointer = indirect_block.pointers[j];

                        if (pointer != 0){
                            fs->free_blocks[pointer] = false;
                        }
                        

                    }
                    

                }

            }

       
        
        

    }


    return true;
}

/**
 * 通过执行以下操作从内部磁盘卸载文件系统：
 *
 *  1. 设置文件系统的磁盘属性。
 *
 *  2. 释放空闲块位图。
 *
 * @param       fs      指向FileSystem结构的指针。
 **/
void  fs_unmount(FileSystem *fs) {

    if (fs == NULL) return;
    
    if (fs->free_blocks != NULL){
        free(fs->free_blocks);
    }
    
    
    fs->free_blocks = NULL;
    fs->disk = NULL;

}

/**
 * 通过执行以下操作在文件系统的indoe表中分配一个inode：
 *
 *  1. 在inode表中搜索空闲inode。
 *
 *  2. 在inode表中保留空闲inode。
 *
 * 注意：务必记录对i节点表的更新到磁盘。
 *
 * @param       fs      指向FileSystem结构的指针。
 * @return      分配的inode的inode编号。
 **/
ssize_t fs_create(FileSystem *fs) {

    if (fs == NULL)
        return -1;
    
    for (size_t inode_number  = 0; inode_number  < fs->meta_data.inodes; inode_number ++){
            
        
        size_t block_number = 1 + inode_number / INODES_PER_BLOCK;
        Block inode_block;
        

        if(disk_read(fs->disk, block_number, inode_block.data) == DISK_FAILURE)
            return -1;

           
        size_t index_within_block  = inode_number % INODES_PER_BLOCK;

  

        if (inode_block.inodes[index_within_block].valid == true) continue;
        inode_block.inodes[index_within_block].valid = true;
        
        if(disk_write(fs->disk, block_number, inode_block.data) == DISK_FAILURE)
            return -1;

        inode_block.inodes[index_within_block].size = 0; 

        return inode_number;
        
    }
    
    return -1;
    
}

/**
 * 通过执行以下操作从文件系统中删除inode和相关数据：
 *
 *  1. 加载并检查i节点的状态。
 *
 *  2. 释放所有直接块。
 *
 *  3. 释放所有间接块。
 *
 *  4. 在inode表中标记inode为空闲。
 *
 * @param       fs              指向FileSystem结构的指针。
 * @param       inode_number    要删除的i节点。
 * @return      是否成功删除指定i节点（成功为true，失败为false）。
 **/

bool fs_remove(FileSystem *fs, size_t inode_number) {

    if (fs == NULL || fs->disk == NULL || fs->free_blocks == NULL) {
        return false; 
    }

    if (inode_number >= fs->meta_data.inodes) {
        return false;
    }

    size_t block_number = 1 + inode_number / INODES_PER_BLOCK;
    Block inode_block;
    if (disk_read(fs->disk, block_number, inode_block.data) == DISK_FAILURE){
        return false;
    }
    size_t index_within_block  = inode_number % INODES_PER_BLOCK;

    Inode *inodes  = inode_block.inodes;

    
    uint32_t *direct = inodes[index_within_block].direct;
    uint32_t indirect = inodes[index_within_block].indirect;

    if (inodes[index_within_block].valid == false) return false;
    inodes[index_within_block].valid = false;


    for (size_t i = 0; i < POINTERS_PER_INODE; i++){
        if (direct[i] != 0){
            fs->free_blocks[direct[i]] = true;
            inodes[index_within_block].direct[i] = 0;
        }
    }
    


    if (indirect != 0)
    {
        Block indirect_block;
        
        
        fs->free_blocks[indirect] = true;     

        if (disk_read(fs->disk, indirect, indirect_block.data) == DISK_FAILURE)
            return false;
      
        for (size_t i = 0; i < POINTERS_PER_BLOCK; i ++){
            if (indirect_block.pointers[i] != 0){
                fs->free_blocks[indirect_block.pointers[i]] = true;
            }
            
        }

    }
    
    inodes[index_within_block].size = 0;

    if (disk_write(fs->disk, block_number, inode_block.data) == DISK_FAILURE){
        return false;
    }
    
    

    return true;
}

/**
 * 返回指定inode的大小。
 *
 * @param       fs              指向FileSystem结构的指针。
 * @param       inode_number    要删除的inode。
 * @return      指定inode的大小（如果不存在则为-1）。
 **/
ssize_t fs_stat(FileSystem *fs, size_t inode_number) {
    if (fs == NULL || fs->disk == NULL || fs->free_blocks == NULL) {
        return -1; 
    }

    if (inode_number >= fs->meta_data.inodes) {
        return -1;
    }

    size_t block_number = 1 + inode_number / INODES_PER_BLOCK;
    Block inode_block;
    if (disk_read(fs->disk, block_number, inode_block.data) == DISK_FAILURE){
        return -1;
    }
    size_t index_within_block  = inode_number % INODES_PER_BLOCK;

    Inode *inodes  = inode_block.inodes;

    if (inodes[index_within_block].valid == 1) return  inodes[index_within_block].size;  
    return -1;
}

/**
 * 从指定的i节点中读取数据，从指定的偏移开始精确地读取长度字节，执行以下操作：
 *
 *  1. 加载i节点信息。
 *
 *  2. 连续读取块并将数据复制到缓冲区。
 *
 *  注意：首先从直接块中读取数据，然后从间接块中读取数据。
 *
 * @param       fs              指向FileSystem结构的指针。
 * @param       inode_number    要从中读取数据的inode。
 * @param       data            用于复制数据的缓冲区。
 * @param       length          要读取的字节数。
 * @param       offset          从哪里开始读取的字节偏移。
 * @return      读取的字节数（错误时为-1）。
 **/
ssize_t fs_read(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset) {
    if (fs == NULL || fs->disk == NULL || fs->free_blocks == NULL) {
        return -1; 
    }

    if (inode_number >= fs->meta_data.inodes) {
        return -1;
    }

    size_t block_number = 1 + inode_number / INODES_PER_BLOCK;
    Block inode_block;
    if (disk_read(fs->disk, block_number, inode_block.data) == DISK_FAILURE){
        return -1;
    }
    size_t index_within_block  = inode_number % INODES_PER_BLOCK;

    Inode *inodes  = inode_block.inodes;
    Inode *inode = &inodes[index_within_block];
    if (inode->valid == 0) return -1;

    ssize_t bytes_read = 0; 
    size_t current_offset = offset;
    
    while (current_offset < inode->size && bytes_read < length){
        char buf[BLOCK_SIZE];
       

        // 计算要读取的块和偏移量
        size_t block_offset = current_offset % BLOCK_SIZE;
        size_t block_index = current_offset / BLOCK_SIZE;
        size_t bytes_to_read = BLOCK_SIZE - block_offset;

        if(block_index > POINTERS_PER_INODE) break;


        if (bytes_to_read > length - bytes_read) {
            bytes_to_read = length - bytes_read;
        }

        if (disk_read(fs->disk, inode->direct[block_index], buf) == DISK_FAILURE)
            return -1;

        memcpy(data + bytes_read, buf + block_offset, bytes_to_read);
        bytes_read += bytes_to_read;
        current_offset += bytes_to_read;

    }
    
    if (inode->indirect != 0){
               
        Block indirect_block;
        

        if (disk_read(fs->disk, inode->indirect, indirect_block.data) == DISK_FAILURE)
            return false;

        uint32_t pointer = 0; 
        
        while (current_offset < inode->size && bytes_read < length){
            
           
                char buf[BLOCK_SIZE];
                // 计算要读取的块和偏移量
                size_t block_offset = current_offset % BLOCK_SIZE;
                size_t block_index = current_offset / BLOCK_SIZE - POINTERS_PER_INODE;
                size_t bytes_to_read = BLOCK_SIZE - block_offset;
                if (block_index >= POINTERS_PER_BLOCK || block_index < 0) break;
                
                


                ssize_t pointer = indirect_block.pointers[block_index];
                if (pointer != 0)
                {

                    if (bytes_to_read > length - bytes_read) {
                        bytes_to_read = length - bytes_read;
                    }

                    if (disk_read(fs->disk, pointer, buf) == DISK_FAILURE)
                        return -1; 

                    memcpy(data + bytes_read, buf + block_offset, bytes_to_read);
                    bytes_read += bytes_to_read;
                    current_offset += bytes_to_read;

            }
        }
    }
    
    
    return bytes_read;
}

/**
 * 向指定的inode写入数据，从指定的偏移开始精确地写入长度字节，执行以下操作：
 *
 *  1. 加载i节点信息。
 *
 *  2. 连续从缓冲区复制数据到块。
 *
 *  注意：首先从直接块中读取数据，然后从间接块中读取数据。
 *
 * @param       fs              指向FileSystem结构的指针。
 * @param       inode_number    要写入数据的i节点。
 * @param       data            包含要复制的数据的缓冲区。
 * @param       length          要写入的字节数。
 * @param       offset          从哪里开始写入的字节偏移。
 * @return      写入的字节数（错误时为-1）。
 **/
ssize_t fs_write(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset) {
    if (fs == NULL || fs->disk == NULL || fs->free_blocks == NULL) {
        return -1; 
    }

    if (inode_number >= fs->meta_data.inodes) {
        return -1;
    }


    size_t block_number = 1 + inode_number / INODES_PER_BLOCK;
    Block inode_block;
    if (disk_read(fs->disk, block_number, inode_block.data) == DISK_FAILURE){
        return -1;
    }
    size_t index_within_block  = inode_number % INODES_PER_BLOCK;

    Inode *inodes  = inode_block.inodes;
    Inode *inode = &inodes[index_within_block];
    if (inode->valid == 0) return -1;


    size_t end_offset = offset + length;
    size_t end_block = (end_offset + BLOCK_SIZE - 1) / BLOCK_SIZE;

     
    size_t current_offset = offset;
    size_t current_block = current_offset / BLOCK_SIZE;
    size_t bytes_written = 0;

    while (current_block < end_block&& bytes_written < length){

        char buf[BLOCK_SIZE];
        size_t block_index = current_block;

        if (block_index < POINTERS_PER_BLOCK){
            
            if (inode->direct[block_index] == 0){
                size_t pointer = 0;
                for (size_t i = fs->meta_data.inode_blocks + 1; i < fs->meta_data.blocks; i++){
                    if (fs->free_blocks[i] == true)
                    {
                        pointer = i;
                        inode->direct[block_index] = pointer;
                        fs->free_blocks[i] = false;
                        break;
                    } 
                }
                
                if (pointer == 0) return -1;
            }

            size_t block_offset = current_offset % BLOCK_SIZE;
            size_t bytes_to_write = BLOCK_SIZE - block_offset;

            if (bytes_to_write > length - bytes_written) {
                bytes_to_write = length - bytes_written;
            }

            if (disk_read(fs->disk, inode->direct[block_index], buf) == DISK_FAILURE){
                return -1;
            }
            memcpy(buf + block_offset, data + bytes_written, bytes_to_write);

            if (disk_write(fs->disk, inode->direct[block_index], buf));
            current_offset += bytes_to_write;
            current_block = current_offset / BLOCK_SIZE;
            bytes_written += bytes_to_write;
        }
        else
        {
            if (inode->indirect == 0)
            {
                size_t pointer = 0;
                for (size_t i = fs->meta_data.inode_blocks + 1; i < fs->meta_data.blocks; i++){
                    if (fs->free_blocks[i] == true)
                    {
                        pointer = i;
                        inode->indirect = pointer;
                        fs->free_blocks[i] = false;
                        break;
                    } 
                }
                
                if (pointer == 0) return -1;
            }
            
            Block indirect_block;
            uint32_t indirect_index = block_index - POINTERS_PER_INODE;

            if (indirect_index >= POINTERS_PER_BLOCK) return -1;
           
            
            
            if (disk_read(fs->disk, inode->indirect, indirect_block.data)){
                return -1;
            }

            uint32_t *pointer = &indirect_block.pointers[indirect_index];
            
            if (*pointer == 0)
            {
                for (size_t i = fs->meta_data.inode_blocks + 1; i < fs->meta_data.blocks; i++){
                    if (fs->free_blocks[i] == true)
                    {
                        *pointer = i;
                        fs->free_blocks[i] = false;
                        break;
                    } 
                }

                if (*pointer == 0) return -1;

            }

            if (disk_read(fs->disk, *pointer, buf) == DISK_FAILURE){
                return -1;
            }
            
            size_t block_offset = current_offset % BLOCK_SIZE;
            size_t bytes_to_write = BLOCK_SIZE - block_offset;

            if (bytes_to_write > length - bytes_written) {
                bytes_to_write = length - bytes_written;
            }



            memcpy(buf + block_offset, data + bytes_written, bytes_to_write);

            if (disk_write(fs->disk, *pointer, buf));
            current_offset += bytes_to_write;
            current_block = current_offset / BLOCK_SIZE;
            bytes_written += bytes_to_write;

        }
        
    }
    
    if (end_offset > inode->size) {
        inode->size = end_offset;
    }
    
    return bytes_written;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
