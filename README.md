# Project 06: Simple File System

项目是构建一个名为**SimpleFS的**[Unix 文件系统的](https://en.wikipedia.org/wiki/Unix_File_System)*简化*版本，如右图所示。在此应用程序中，我们具有三个组件：

1.  **Shell ：第一个组件是一个简单的 shell 应用程序，允许用户在SimpleFS上执行操作，例如打印有关文件系统的调试信息、格式化新文件系统、挂载文件系统、创建文件以及将数据复制入或复制出文件系统的。为此，它会将这些用户命令转换为**文件系统操作，例如`fs_debug`、`fs_format`、`fs_create`、 `fs_remove`、`fs_stat`、`fs_read`和`fs_write`。
2.  **文件系统：第二个组件通过****shell**获取用户指定的操作，并在**SimpleFS**磁盘映像上执行这些操作。该组件负责组织磁盘上的数据结构并执行所有必要的簿记以允许数据的持久存储。为了存储数据，它需要通过和 等函数与**磁盘模拟器**交互，这些函数允许文件系统以字节块的形式读取和写入磁盘映像。`disk_read``disk_write``4096`
3.  **磁盘模拟器**：第三个组件通过将普通文件（称为**磁盘映像**）划分为`4096 byte`块来模拟磁盘，并且仅允许**文件系统**以块为单位进行读写。[该模拟器将使用正常的open](http://man7.org/linux/man-pages/man2/open.2.html)、[read](http://man7.org/linux/man-pages/man2/read.2.html)和[write](http://man7.org/linux/man-pages/man2/write.2.html)系统调用将数据持久存储到磁盘映像中。
