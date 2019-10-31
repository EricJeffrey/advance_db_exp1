# 高级数据库实验
USTC高级数据库课程实验1，实现缓冲区管理与数据仓库管理接口。[课程链接](http://staff.ustc.edu.cn/~jpq/courses/adb-cs.html)，[实验链接](http://staff.ustc.edu.cn/~jpq/courses/lab_adb-cs.html)。

## 数据管理 ds_mgr
- 上层为缓冲区管理提供数据操作接口
- 下层为操作系统文件系统
- 外部接口
  - 初始化相关文件信息
  - 读取一个page
  - 写入一个新page
  - 获取页数
- 内部接口
  - 读取目录

### ds_mgr 数据结构设计
页大小为8KB，使用一页作为目录，存储`N=(8*1024/4)=2*1024`个页地址和下一个目录表的地址。其中页地址与目录表地址为偏移量，使用整数(4Byte)表示。

- 文件指针: FILE* fp
- 页数: numpages
- 目录表: contents -> int[]

### ds_mgr 流程设计
每个目录能够索引x个页
- 读一个页p
  - 跳过`p / x`个目录项
  - 读取目录第`p % x`个项
- 写一个页p
  - 跳过`p / x`个目录项
  - 写入目录第`p % x`个项
- 写一个新页
  - `numpages == 0` 文件为空
    - 建新目录 更新目录
    - 目录写入磁盘
    - 数据写入磁盘
  - `p % x == 0`，需要新建目录
    - 跳过`p / x - 1`个目录，得到文件中的最后一个目录
    - 更新最后一个目录中的下一个目录表的值为最后一个页的`offset + page_size`
    - 当前目录写入磁盘
    - 建立新的目录，其第0页的offset为 最后一个页的`offset + page_size + content_size`
    - 新目录写入磁盘
    - 将新页写到第0页处
  - `p % x != 0`
    - 跳过`p / x`个目录项
    - 新页的偏移量为最后一个页的`offset + page_size`
    - 更新目录，写入磁盘
    - 将新页写入磁盘
  - 增加numpages



## 缓冲区管理 buffer_mgr
- 上层为记录查询提供增删改查功能
- 下层调用数据管理接口的功能
- 外部接口
  - 读取某个page内容
  - 更新某个page内容
  - 增加一个新page
  - 获取可用frame数目
- 内部接口
  - 根据淘汰算法选择受害者
  - 设置/取消设置一个页为脏页
  - 将所有脏页写入磁盘
  - 打印一个frame的内容
  - 根据LRU算法更新一个缓冲块
  - 根据LRU算法插入一个新的缓冲块


### buffer_mgr 流程
- 读page
  - 在缓冲区
    - 获得frame标识
    - 调用ds_mgr读取数据
    - 返回数据
  - 不在缓冲区
    - 缓冲区满
      - 淘汰一个frame - LRU
      - frame是dirty的
      - 调用ds_mgr写会磁盘
    - 缓冲区不满
      - 选择空缓冲块
    - 调用ds_mgr读取page数据到缓冲区
    - 返回数据
- 写page
  - 在缓冲区
    - 更新缓冲区数据
    - 设置为dirty
    - LRU更新
  - 不在缓冲区
    - 缓冲区满
      - 淘汰一个frame - LRU
      - dirty?
        - 调用ds_mgr写会磁盘
    - 缓冲区未满
       - 选择一个空缓冲块
    - 调用ds_mgr读取page数据到缓冲区
    - 更新数据
    - 设置为dirty
    - LRU更新

### buffer_mgr 数据结构设计
- 缓冲区: buffer[]
- 缓冲区大小: size
- 链表首/尾: head_frame_id, tail_frame_id
- frame->page: frame2page[] 数组
- page->frame: page2frame[] 哈希表
- 缓冲块信息(链表节点): bcb[] -> { frame_id, page_id, dirty, count, prev_frame_id, next_frame_id }


## 问题与解决
- *fread莫名遇到eof，fseek、ftell都没问题*: 打开文件时指定`b`，即以二进制方式打开，如下为`fseek`相关信息  
  > For streams open in binary mode, the new position is defined by adding offset to a reference position specified by origin. For streams open in text mode, offset shall either be zero or a value returned by a previous call to ftell, and origin shall necessarily be SEEK_SET. 
- *目录读取IO过多*: 重用内存中的目录
- *vscode莫名抛出 无法打开....cygwin.s*: 由于栈内数组开的过大，造成了调试器启动失败！！！