#pragma once

#include "ref.hh"
#include <cstddef>
#include <memory>
#include <vector>

/*
 * 📚 C++知识体系1：头文件保护和包含机制
 * 
 * #pragma once - 现代C++的头文件保护，防止重复包含
 * 替代传统的 #ifndef/#define/#endif 方式
 * 
 * <memory> - 智能指针头文件，包含shared_ptr等
 * <vector> - 动态数组容器
 * <cstddef> - 基本类型定义，如size_t
 */

/*
 * 🌐 计算机网络知识体系1：文件描述符基础
 * 
 * 文件描述符(File Descriptor)是Unix/Linux系统的核心概念：
 * - 本质：一个非负整数，操作系统用来标识打开的文件/网络连接/设备
 * - 网络编程中：socket就是一种特殊的文件描述符
 * - 标准描述符：0(stdin), 1(stdout), 2(stderr)
 * - 系统限制：每个进程有最大文件描述符数量限制
 * 
 * 为什么重要？
 * 1. 网络服务器需要处理大量客户端连接，每个连接都是一个文件描述符
 * 2. 如果不正确管理，会导致文件描述符泄漏，系统崩溃
 * 3. 现代服务器(如Nginx)可以同时处理数万个连接
 */

// A reference-counted handle to a file descriptor
// 一个文件描述符的引用计数句柄
class FileDescriptor
{
  /*
   * 🏗️ C++知识体系2：嵌套类设计模式
   * 
   * 什么是嵌套类？
   * - 在一个类内部定义另一个类
   * - 内部类可以访问外部类的所有成员(包括private)
   * - 提供更好的封装性和逻辑组织
   * 
   * 为什么这里要用嵌套类？
   * 1. 封装性：FDWrapper只为FileDescriptor服务，外部不应直接使用
   * 2. 逻辑清晰：两个类紧密相关，放在一起便于理解
   * 3. 避免污染全局命名空间
   * 
   * 类比理解：
   * FileDescriptor = 汽车
   * FDWrapper = 汽车引擎
   * 引擎只为汽车服务，用户不应该直接操作引擎
   */
  
  // FDWrapper: A handle on a kernel file descriptor.
  // FDWrapper: 内核文件描述符的句柄包装器
  // FileDescriptor objects contain a std::shared_ptr to a FDWrapper.
  // FileDescriptor对象包含一个指向FDWrapper的智能指针
  class FDWrapper  // 【嵌套类】：在FileDescriptor类内部定义的私有辅助类
  {
  public:
    /*
     * 🌐 计算机网络知识体系2：文件描述符状态管理
     * 
     * 网络编程中的文件描述符需要跟踪多种状态：
     * 1. fd_: 实际的文件描述符编号
     * 2. eof_: 是否到达数据流末尾(对网络连接表示对方关闭)
     * 3. closed_: 本地是否已关闭
     * 4. non_blocking_: 是否为非阻塞模式
     * 5. read_count_/write_count_: 性能统计和调试
     * 
     * 阻塞vs非阻塞模式详解：
     * 
     * 阻塞模式(blocking)：
     * - read()会等待直到有数据到达才返回
     * - write()会等待直到所有数据写完才返回
     * - 简单但低效，一个线程只能处理一个连接
     * 
     * 非阻塞模式(non-blocking)：
     * - read()立即返回，没数据就返回EAGAIN错误
     * - write()立即返回，写不完就返回已写的字节数
     * - 复杂但高效，一个线程可以处理多个连接
     * - 现代高性能服务器的基础(epoll/kqueue/IOCP)
     */
    int fd_;                    // The file descriptor number returned by the kernel / 内核返回的文件描述符数字
    bool eof_ = false;          // Flag indicating whether FDWrapper::fd_ is at EOF / 标志位：表示文件描述符是否到达文件末尾
    bool closed_ = false;       // Flag indicating whether FDWrapper::fd_ has been closed / 标志位：表示文件描述符是否已关闭
    bool non_blocking_ = false; // Flag indicating whether FDWrapper::fd_ is non-blocking / 标志位：表示文件描述符是否为非阻塞模式
    unsigned read_count_ = 0;   // The number of times FDWrapper::fd_ has been read / 文件描述符被读取的次数统计
    unsigned write_count_ = 0;  // The numberof times FDWrapper::fd_ has been written / 文件描述符被写入的次数统计

    /*
     * 🛠️ C++知识体系3：RAII设计模式
     * 
     * RAII = Resource Acquisition Is Initialization
     * 核心思想：用对象的生命周期管理资源
     * 
     * 传统C语言问题：
     * int fd = open("file.txt", O_RDONLY);
     * // 如果这里发生异常或提前return
     * close(fd); // 这行可能永远不会执行！
     * 
     * RAII解决方案：
     * - 构造函数获取资源(open文件)
     * - 析构函数释放资源(close文件)
     * - 无论如何退出作用域，析构函数都会被调用
     * 
     * 网络编程中的重要性：
     * - 服务器可能同时处理数千个连接
     * - 一个文件描述符泄漏都可能导致系统崩溃
     * - RAII确保资源100%被正确释放
     */
    
    // Construct from a file descriptor number returned by the kernel
    // 从内核返回的文件描述符数字构造FDWrapper对象
    explicit FDWrapper( int fd );
    // Closes the file descriptor upon destruction
    // 析构时关闭文件描述符
    ~FDWrapper();
    // Calls [close(2)](\ref man2::close) on FDWrapper::fd_
    // 调用系统函数close(2)关闭文件描述符
    void close();

    /*
     * 🔧 C++知识体系4：模板编程基础
     * 
     * template<typename T> 解释：
     * - template: 告诉编译器这是一个模板
     * - typename T: T是一个类型参数，可以是任何类型
     * - 编译时多态：编译器为每种使用的类型生成专门的函数
     * 
     * 为什么这里需要模板？
     * 系统调用返回不同类型：
     * - read() 返回 ssize_t
     * - write() 返回 ssize_t  
     * - fcntl() 返回 int
     * - lseek() 返回 off_t
     * 
     * 模板让一个函数处理所有这些类型！
     * 
     * 🌐 计算机网络知识体系3：系统调用错误处理
     * 
     * Unix系统调用约定：
     * - 成功：返回 >= 0 的值
     * - 失败：返回 -1，并设置errno
     * - 特殊情况：非阻塞模式下，EAGAIN表示"暂时无数据"
     * 
     * 为什么需要统一的错误处理？
     * - 网络程序需要处理大量系统调用
     * - 手动检查每个调用容易出错
     * - 统一处理提高代码质量和调试效率
     */
    template<typename T>
    // 模板函数：T是一个类型占位符，可以是int、ssize_t等任何系统调用返回类型
    // 作用：检查系统调用是否成功，失败时抛出异常
    // 参数：s_attempt是操作名（如"read"），return_value是系统调用返回值
    T CheckSystemCall( std::string_view s_attempt, T return_value ) const;

    /*
     * 🚫 C++知识体系5：禁用拷贝和移动
     * 
     * = delete 语法（C++11特性）：
     * - 明确禁止编译器生成某些函数
     * - 比private声明更清晰，编译时就报错
     * 
     * 为什么要禁用FDWrapper的拷贝和移动？
     * 
     * 1. 安全性：文件描述符是系统资源
     *    如果允许拷贝：
     *    FDWrapper fd1(5);
     *    FDWrapper fd2 = fd1;  // 现在fd1和fd2都有文件描述符5
     *    // fd1析构时close(5)
     *    // fd2析构时再次close(5) -> 错误！可能关闭了其他文件
     * 
     * 2. 语义清晰：FDWrapper代表文件描述符的"唯一所有权"
     *    一个文件描述符编号在同一时刻只能有一个拥有者
     * 
     * 3. 强制使用shared_ptr：想要共享文件描述符，必须通过智能指针
     *    这样引用计数可以确保只在最后一个引用销毁时才close()
     */
    
    // An FDWrapper cannot be copied or moved
    // FDWrapper对象不能被复制或移动（防止意外的文件描述符复制）
    FDWrapper( const FDWrapper& other ) = delete;
    FDWrapper& operator=( const FDWrapper& other ) = delete;
    FDWrapper( FDWrapper&& other ) = delete;
    FDWrapper& operator=( FDWrapper&& other ) = delete;
  };

  /*
   * 🧠 C++知识体系6：智能指针深度解析
   * 
   * std::shared_ptr<FDWrapper> 详解：
   * 
   * 问题：多个FileDescriptor对象可能需要共享同一个文件描述符
   * 例如：通过dup()系统调用复制的文件描述符
   * 
   * 传统指针的问题：
   * FDWrapper* ptr = new FDWrapper(fd);
   * FileDescriptor fd1(ptr);
   * FileDescriptor fd2(ptr);
   * // 谁负责delete ptr？如果fd1和fd2都delete会导致崩溃！
   * 
   * shared_ptr解决方案：
   * - 内部维护引用计数
   * - 每次复制时引用计数+1
   * - 每次销毁时引用计数-1
   * - 引用计数为0时自动删除对象
   * 
   * 在网络编程中的应用：
   * - 一个socket可能被多个对象引用（读线程、写线程等）
   * - 只有当所有引用都销毁时，才真正关闭socket
   * - 避免了"提前关闭"或"重复关闭"的问题
   */
  
  // A reference-counted handle to a shared FDWrapper
  // 指向共享FDWrapper的引用计数句柄（使用智能指针管理内存）
  std::shared_ptr<FDWrapper> internal_fd_;

  /*
   * 🔒 C++知识体系7：访问控制和设计意图
   * 
   * private构造函数的作用：
   * - 阻止外部直接创建FileDescriptor对象
   * - 只能通过public构造函数或duplicate()方法创建
   * - 确保所有FileDescriptor都正确初始化
   * 
   * 设计意图：
   * - public FileDescriptor(int fd): 从文件描述符编号创建
   * - private FileDescriptor(shared_ptr): 从已有的shared_ptr创建
   * - 这样duplicate()可以创建共享同一FDWrapper的FileDescriptor
   */
  
  // private constructor used to duplicate the FileDescriptor (increase the reference count)
  // 私有构造函数：用于复制FileDescriptor对象（增加引用计数）
  explicit FileDescriptor( std::shared_ptr<FDWrapper> other_shared_ptr );

protected:
  /*
   * 🌐 计算机网络知识体系4：缓冲区管理
   * 
   * 为什么需要缓冲区？
   * 1. 系统调用开销：每次read/write都是昂贵的系统调用
   * 2. 网络特性：TCP数据可能分片到达，需要缓冲
   * 3. 性能优化：批量处理比单字节处理快很多
   * 
   * 16384字节(16KB)的选择：
   * - 不太小：减少系统调用次数
   * - 不太大：避免内存浪费
   * - 常见选择：4KB、8KB、16KB、64KB
   * - 考虑因素：CPU缓存大小、内存页大小、网络MTU
   */
  
  // size of buffer to allocate for read()
  // 为read()操作分配的缓冲区大小（16KB）
  static constexpr size_t kReadBufferSize = 16384;

  void set_eof() { internal_fd_->eof_ = true; }
  void register_read() { ++internal_fd_->read_count_; }   // increment read count / 增加读取次数计数
  void register_write() { ++internal_fd_->write_count_; } // increment write count / 增加写入次数计数

  template<typename T>
  // 模板函数：检查系统调用返回值，处理错误情况
  T CheckSystemCall( std::string_view s_attempt, T return_value ) const;

public:
  /*
   * 🏗️ C++知识体系8：构造函数设计
   * 
   * explicit关键字的重要性：
   * 
   * 没有explicit的危险：
   * void process_file(FileDescriptor fd);
   * process_file(5);  // 意外！5被隐式转换为FileDescriptor
   * 
   * 有explicit的安全：
   * void process_file(FileDescriptor fd);
   * process_file(5);              // 编译错误！
   * process_file(FileDescriptor(5)); // 必须显式构造
   * 
   * 在网络编程中的意义：
   * - 文件描述符是珍贵的系统资源
   * - 意外创建FileDescriptor对象可能导致资源泄漏
   * - explicit强制程序员明确表达意图
   */
  
  // Construct from a file descriptor number returned by the kernel
  // 从内核返回的文件描述符数字构造FileDescriptor对象
  explicit FileDescriptor( int fd );

  /*
   * 🎯 C++知识体系9：现代C++移动语义
   * 
   * = default 的含义：
   * - 告诉编译器生成默认的析构函数
   * - 与 = delete 相对，明确表达设计意图
   * - C++11引入，提高代码清晰度
   * 
   * 为什么析构函数可以是default？
   * - FileDescriptor只包含一个shared_ptr成员
   * - shared_ptr的析构函数会自动处理引用计数
   * - 当引用计数归零时，FDWrapper的析构函数会被调用
   * - FDWrapper的析构函数负责close()文件描述符
   * 
   * 这展示了RAII的优雅：
   * - 外层对象不需要手动管理资源
   * - 智能指针自动处理内存管理
   * - 内层对象负责具体的资源释放
   * - 整个系统自动、安全、高效
   */
  
  // Free the std::shared_ptr; the FDWrapper destructor calls close() when the refcount goes to zero.
  // 释放智能指针；当引用计数归零时，FDWrapper析构函数会调用close()
  ~FileDescriptor() = default;

  /*
   * 🌐 计算机网络知识体系5：I/O操作模式
   * 
   * 网络编程的核心：高效的数据传输
   * 
   * 读操作(read)：
   * - 从socket接收数据
   * - 从文件读取内容
   * - 可能阻塞等待数据到达
   * 
   * 写操作(write)：
   * - 向socket发送数据
   * - 向文件写入内容
   * - 可能因为缓冲区满而阻塞
   * 
   * 多种重载的设计意图：
   * 1. std::string& buffer - 简单字符串读写
   * 2. std::vector<std::string>& buffers - 分散读取(scatter)
   * 3. std::vector<std::string_view> - 聚集写入(gather)，零拷贝
   * 4. std::vector<Ref<std::string>> - 引用计数字符串，内存高效
   * 
   * 这些模式在高性能网络编程中很重要：
   * - HTTP协议：头部和正文可能分开处理
   * - 数据库：批量操作比单条操作快
   * - 文件传输：大文件需要分块处理
   */
  
  // Read into `buffer`
  // 读取数据到缓冲区
  void read( std::string& buffer );
  void read( std::vector<std::string>& buffers );

  // Attempt to write a buffer
  // 尝试写入缓冲区数据
  // returns number of bytes written
  // 返回实际写入的字节数
  size_t write( std::string_view buffer );
  size_t write( const std::vector<std::string_view>& buffers );
  size_t write( const std::vector<Ref<std::string>>& buffers );

  /*
   * 🔧 C++知识体系10：方法设计和const正确性
   * 
   * duplicate()方法的设计意图：
   * - 显式复制，而不是隐式复制构造函数
   * - 增加shared_ptr的引用计数
   * - 返回新的FileDescriptor对象，共享同一FDWrapper
   * 
   * const方法的重要性：
   * - fd_num()等访问器方法标记为const
   * - 表示这些方法不会修改对象状态
   * - 编译器可以优化，代码更安全
   * - const FileDescriptor对象也可以调用这些方法
   */
  
  // Close the underlying file descriptor
  // 关闭底层文件描述符
  void close() { internal_fd_->close(); }

  // Copy a FileDescriptor explicitly, increasing the FDWrapper refcount
  // 显式复制FileDescriptor对象，增加FDWrapper的引用计数
  FileDescriptor duplicate() const;

  // Set blocking(true) or non-blocking(false)
  // 设置阻塞模式(true)或非阻塞模式(false)
  void set_blocking( bool blocking );

  // Size of file
  // 获取文件大小
  off_t size() const;

  // FDWrapper accessors
  // FDWrapper访问器函数（获取内部状态信息）
  int fd_num() const { return internal_fd_->fd_; }                        // underlying descriptor number / 底层描述符编号
  bool eof() const { return internal_fd_->eof_; }                         // EOF flag state / 文件结束标志状态
  bool closed() const { return internal_fd_->closed_; }                   // closed flag state / 关闭标志状态
  unsigned int read_count() const { return internal_fd_->read_count_; }   // number of reads / 读取次数
  unsigned int write_count() const { return internal_fd_->write_count_; } // number of writes / 写入次数

  /*
   * 🌐 计算机网络知识体系6：现代C++移动语义详解
   * 
   * 移动语义解决的问题：
   * 
   * 传统拷贝的低效：
   * FileDescriptor create_connection() {
   *     FileDescriptor fd(socket(...));
   *     return fd;  // 传统：昂贵的拷贝操作
   * }
   * 
   * 移动语义的高效：
   * FileDescriptor create_connection() {
   *     FileDescriptor fd(socket(...));
   *     return fd;  // 现代：高效的移动操作，只转移所有权
   * }
   * 
   * 为什么禁用拷贝，允许移动？
   * 
   * 拷贝的问题：
   * FileDescriptor fd1(5);
   * FileDescriptor fd2 = fd1;  // 如果允许，fd1和fd2都"拥有"同一文件描述符
   * // 这在逻辑上不清晰：谁负责关闭？
   * 
   * 移动的优势：
   * FileDescriptor fd1(5);
   * FileDescriptor fd2 = std::move(fd1);  // 所有权从fd1转移到fd2
   * // fd1不再拥有文件描述符，只有fd2拥有
   * 
   * 现代网络编程模式：
   * - 文件描述符通常有唯一所有权
   * - 需要共享时使用duplicate()显式操作
   * - 需要转移时使用移动语义
   * - 这样的设计清晰、安全、高效
   */
  
  // Copy/move constructor/assignment operators / 复制/移动构造函数/赋值操作符
  // FileDescriptor can be moved, but cannot be copied implicitly (see duplicate())
  // FileDescriptor可以移动，但不能隐式复制（请使用duplicate()函数）
  FileDescriptor( const FileDescriptor& other ) = delete;            // copy construction is forbidden / 禁止复制构造
  FileDescriptor& operator=( const FileDescriptor& other ) = delete; // copy assignment is forbidden / 禁止复制赋值
  FileDescriptor( FileDescriptor&& other ) = default;                // move construction is allowed / 允许移动构造
  FileDescriptor& operator=( FileDescriptor&& other ) = default;     // move assignment is allowed / 允许移动赋值
};

/*
 * 🎓 总结：这个FileDescriptor类的设计精髓
 * 
 * 1. 安全性：
 *    - RAII确保资源不泄漏
 *    - 智能指针管理共享所有权
 *    - 禁用危险的隐式拷贝
 * 
 * 2. 高效性：
 *    - 移动语义避免不必要的拷贝
 *    - 引用计数只在需要时增加开销
 *    - 模板避免重复代码
 * 
 * 3. 易用性：
 *    - 统一的错误处理
 *    - 清晰的方法命名
 *    - 类型安全的接口
 * 
 * 4. 可维护性：
 *    - 嵌套类保持封装
 *    - const正确性
 *    - 明确的设计意图
 * 
 * 这是现代C++和系统编程的完美结合，
 * 展现了如何用高级语言特性解决底层系统问题。
 */
