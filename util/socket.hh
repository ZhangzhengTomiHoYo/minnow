#pragma once

#include "address.hh"
#include "file_descriptor.hh"

#include <functional>
#include <sys/socket.h>

/*
  预备知识
    熟悉头文件与源文件的分工
      头文件（.hh/.hpp/.h）：只写声明，不写实现（除了模板类/函数）。
      源文件（.cc/.cpp/.cxx）：写具体实现。
    理解C++项目结构
      头文件放在 include/ 或 src/、util/ 等目录下
      源文件放在 src/、apps/、tests/ 等目录下
      使用 CMake、Makefile 等工具进行项目构建
      (这个Lab0 结构好像并不是这样)
*/

/*
  Doxygen 注释，可以根据注释自动生成文档
  参考：https://zhuanlan.zhihu.com/p/510925324
    以 //! 或 （打不出来） 开头，紧贴类、函数、变量等声明。
    用 \brief、\details、\param、\return 等标签描述接口细节。
    支持 Markdown/HTML 格式，生成美观的网页文档。
  
  1. 基本语法
    //! \brief 简介
    //! \details 详细描述
    //! \param[in] param_name 参数说明
    //! \return 返回值说明
    //! \throws 异常说明
    //! \note 注意事项
    //! \warning 警告信息
    //! \see 相关链接或文档
*/

//! \brief Base class for network sockets (TCP, UDP, etc.)
//! \details Socket is generally used via a subclass. See TCPSocket and UDPSocket for usage examples.
//! \brief 网络套接字（TCP、UDP等）的基类。 （这里的意思是，套接字根据传输层协议的不同，可以分为TCPSocket和UDPSocket）
//! \details 套接字通常通过子类使用。有关使用示例，请参阅TCPSocket和UDPSocket。
class Socket : public FileDescriptor
{
private:
  //! Get the local or peer address the socket is connected to
  //! 获取套接字连接到的本地或对端地址
  /*
  这是一个私有成员函数的声明
  1.std::string& 是“引用”，不是指针。引用和指针的区别是：引用更像别名，不能为 null，语法更安全。
    const std::string& 表示“对一个常量字符串的引用”，这样可以避免拷贝，提高效率。
  2.std::function 是 C++11 标准库引入的“函数对象包装器”。
    它可以存储任何可以调用的对象（普通函数、lambda、函数指针、成员函数等）。
    这里的 std::function<int( int, sockaddr*, socklen_t* )> 
    表示“可以接收3个参数（int, sockaddr*, socklen_t*），返回 int 的可调用对象”。
  */
  Address get_address( const std::string& name_of_function,
                       const std::function<int( int, sockaddr*, socklen_t* )>& function ) const;

protected:
  //! Construct via [socket(2)](\ref man2::socket)
  //! 通过 [socket(2)] 系统调用构造套接字
  //! \param[in] domain 地址族（如 AF_INET 表示 IPv4，AF_INET6 表示 IPv6）
  //! \param[in] type 套接字类型（如 SOCK_STREAM 表示 TCP，SOCK_DGRAM 表示 UDP）
  //! \param[in] protocol 协议（通常为 0，让系统自动选择默认协议）
  Socket( int domain, int type, int protocol = 0 );

  //! Construct from a file descriptor.
  //! 从已有的文件描述符构造套接字
  //! \param[in] fd 移动语义的文件描述符对象
  //! \param[in] domain 地址族
  //! \param[in] type 套接字类型
  //! \param[in] protocol 协议
  //! \note && 表示右值引用，用于移动语义，避免不必要的复制
  Socket( FileDescriptor&& fd, int domain, int type, int protocol = 0 );

  //! Wrapper around [getsockopt(2)](\ref man2::getsockopt)
  //! [getsockopt(2)] 系统调用的包装器，用于获取套接字选项
  //! \tparam option_type 选项值的类型（模板参数）
  //! \param[in] level 选项所在的协议层（如 SOL_SOCKET、IPPROTO_TCP）
  //! \param[in] option 具体的选项名（如 SO_REUSEADDR、SO_KEEPALIVE）
  //! \param[out] option_value 用于存储获取到的选项值
  //! \return 返回实际的选项值长度
  template<typename option_type>
  socklen_t getsockopt( int level, int option, option_type& option_value ) const;

  //! Wrappers around [setsockopt(2)](\ref man2::setsockopt)
  //! [setsockopt(2)] 系统调用的包装器，用于设置套接字选项
  //! \tparam option_type 选项值的类型（模板参数）
  //! \param[in] level 选项所在的协议层
  //! \param[in] option 具体的选项名
  //! \param[in] option_value 要设置的选项值
  template<typename option_type>
  void setsockopt( int level, int option, const option_type& option_value );

  //! 设置字符串类型的套接字选项
  //! \param[in] level 选项所在的协议层
  //! \param[in] option 具体的选项名
  //! \param[in] option_val 字符串选项值（如设备名称）
  //! \note std::string_view 是 C++17 引入的轻量级字符串视图，避免字符串复制
  void setsockopt( int level, int option, std::string_view option_val );

public:
  //! Bind a socket to a specified address with [bind(2)](\ref man2::bind), usually for listen/accept
  //! 使用 [bind(2)] 将套接字绑定到指定地址，通常用于监听/接受连接
  //! \param[in] address 要绑定的地址对象
  //! \note 服务器套接字通常需要先 bind() 再 listen()
  void bind( const Address& address );

  //! Bind a socket to a specified device
  //! 将套接字绑定到指定的网络设备
  //! \param[in] device_name 网络设备名称（如 "eth0", "wlan0"）
  //! \note 这通常用于指定数据包从哪个网络接口发送
  void bind_to_device( std::string_view device_name );

  //! Connect a socket to a specified peer address with [connect(2)](\ref man2::connect)
  //! 使用 [connect(2)] 连接到指定的对端地址
  //! \param[in] address 要连接的目标地址
  //! \note 客户端套接字通常调用 connect() 连接到服务器
  void connect( const Address& address );

  //! Shut down a socket via [shutdown(2)](\ref man2::shutdown)
  //! 通过 [shutdown(2)] 关闭套接字的部分功能
  //! \param[in] how 关闭方式：SHUT_RD（关闭读）、SHUT_WR（关闭写）、SHUT_RDWR（关闭读写）
  //! \note shutdown() 不同于 close()，它只是关闭连接的一个方向，而不释放文件描述符
  void shutdown( int how );

  //! Get local address of socket with [getsockname(2)](\ref man2::getsockname)
  //! 使用 [getsockname(2)] 获取套接字的本地地址
  //! \return 返回本地地址对象
  //! \note 对于已绑定的套接字，返回绑定的地址；对于已连接的套接字，返回本地端点地址
  Address local_address() const;
  
  //! Get peer address of socket with [getpeername(2)](\ref man2::getpeername)
  //! 使用 [getpeername(2)] 获取套接字的对端地址
  //! \return 返回对端地址对象
  //! \note 只有已连接的套接字才能获取对端地址
  Address peer_address() const;

  //! Allow local address to be reused sooner via [SO_REUSEADDR](\ref man7::socket)
  //! 通过 [SO_REUSEADDR] 选项允许本地地址更快地重用
  //! \note 这可以避免 "Address already in use" 错误，特别是在服务器重启时
  void set_reuseaddr();

  //! Check for errors (will be seen on non-blocking sockets)
  //! 检查套接字错误（主要用于非阻塞套接字）
  //! \throws 如果套接字有错误，抛出相应异常
  //! \note 非阻塞套接字的错误可能不会立即报告，需要主动检查
  void throw_if_error() const;
};

//! \brief Datagram socket class for connectionless communication
//! \details DatagramSocket 继承自 Socket，专门用于数据报通信（如 UDP）
//! 
//! 继承关系：FileDescriptor -> Socket -> DatagramSocket
//! 
//! 数据报套接字特点：
//! - 无连接：每次发送都需要指定目标地址
//! - 不可靠：不保证数据包到达或顺序
//! - 消息边界：保持数据包的完整性
//! - 快速：没有连接建立的开销
class DatagramSocket : public Socket
{
public:
  //! Receive a datagram and the Address of its sender
  //! 接收数据报及其发送者地址
  //! \param[out] source_address 用于存储发送者的地址
  //! \param[out] payload 用于存储接收到的数据内容
  //! \note 这是阻塞调用，会等待直到有数据到达
  void recv( Address& source_address, std::string& payload );

  //! Send a datagram to specified Address
  //! 向指定地址发送数据报
  //! \param[in] destination 目标地址
  //! \param[in] payload 要发送的数据内容
  //! \note std::string_view 避免字符串复制，提高效率
  void sendto( const Address& destination, std::string_view payload );

  //! Send datagram to the socket's connected address (must call connect() first)
  //! 向套接字已连接的地址发送数据报（必须先调用 connect()）
  //! \param[in] payload 要发送的数据内容
  //! \note 虽然是数据报套接字，但可以"连接"到特定地址以简化发送操作
  void send( std::string_view payload );

protected:
  //! 受保护的构造函数，供子类使用
  //! \param[in] domain 地址族
  //! \param[in] type 套接字类型
  //! \param[in] protocol 协议
  DatagramSocket( int domain, int type, int protocol = 0 ) : Socket( domain, type, protocol ) {}

  //! Construct from a file descriptor.
  //! 从文件描述符构造数据报套接字
  //! \param[in] fd 移动语义的文件描述符
  //! \param[in] domain 地址族
  //! \param[in] type 套接字类型
  //! \param[in] protocol 协议
  DatagramSocket( FileDescriptor&& fd, int domain, int type, int protocol = 0 )
    : Socket( std::move( fd ), domain, type, protocol )
  {}
};

//! A wrapper around [UDP sockets](\ref man7::udp)
//! [UDP 套接字] 的封装类
//! 
//! 继承关系：FileDescriptor -> Socket -> DatagramSocket -> UDPSocket
//! 
//! UDP (User Datagram Protocol) 特点：
//! - 无连接协议：发送前不需要建立连接
//! - 不可靠传输：不保证数据包到达、顺序或去重
//! - 低开销：头部开销小，传输效率高
//! - 适用场景：实时应用（音视频）、DNS查询、简单请求-应答协议
class UDPSocket : public DatagramSocket
{
private:
  //! 私有构造函数，从文件描述符构造
  //! \param[in] fd 文件描述符对象
  //! \note explicit 防止隐式类型转换
  explicit UDPSocket( FileDescriptor&& fd ) : DatagramSocket( std::move( fd ), AF_INET, SOCK_DGRAM ) {}

public:
  //! Default: construct an unbound, unconnected UDP socket
  //! 默认构造函数：创建未绑定、未连接的 UDP 套接字
  //! \note AF_INET 表示 IPv4，SOCK_DGRAM 表示数据报类型
  UDPSocket() : DatagramSocket( AF_INET, SOCK_DGRAM ) {}
};

//! A wrapper around [TCP sockets](\ref man7::tcp)
//! [TCP 套接字] 的封装类
//! 
//! 继承关系：FileDescriptor -> Socket -> TCPSocket
//! 
//! TCP (Transmission Control Protocol) 特点：
//! - 面向连接：通信前必须建立连接（三次握手）
//! - 可靠传输：保证数据按序到达，有错误重传机制
//! - 流式传输：数据作为字节流传输，没有消息边界
//! - 全双工：双方可以同时发送和接收数据
//! - 适用场景：Web浏览、文件传输、邮件等需要可靠传输的应用
class TCPSocket : public Socket
{
private:
  //! \brief Construct from FileDescriptor (used by accept())
  //! \brief 从文件描述符构造（通常由 accept() 使用）
  //! \param[in] fd 移动语义的文件描述符对象
  //! \note 这个构造函数主要用于服务器接受新连接时创建客户端套接字
  //! \note IPPROTO_TCP 明确指定使用 TCP 协议
  /*
  复杂语法详细解析：
  explicit TCPSocket( FileDescriptor&& fd ) : Socket( std::move( fd ), AF_INET, SOCK_STREAM, IPPROTO_TCP ) {}
  
  1. explicit 关键字：
     - 防止隐式类型转换，必须显式调用构造函数
     - 例如：TCPSocket sock(fd); ✓  但 TCPSocket sock = fd; ✗
  
  2. FileDescriptor&& fd：
     - && 表示右值引用，用于移动语义
     - 可以接收临时对象或 std::move() 转换的对象
     - 避免昂贵的复制操作，直接转移资源所有权
  
  3. : Socket(...) 构造函数初始化列表：
     - 冒号后面是初始化列表，在构造当前对象前先构造父类
     - 这里先用给定参数构造 Socket 父类部分
  
  4. std::move(fd)：
     - 将左值 fd 转换为右值，启用移动语义
     - 表示"我不再需要 fd 了，你可以拿走它的资源"
  
  5. 网络常量：
     - AF_INET: IPv4 地址族
     - SOCK_STREAM: 流式套接字（TCP 特征）
     - IPPROTO_TCP: 明确指定 TCP 协议
  
  6. {} 空函数体：
     - 所有初始化工作都在初始化列表中完成
     - 构造函数本身没有额外工作要做
  
  整体效果：高效地从文件描述符创建 TCP 套接字，避免不必要的复制
  */
  explicit TCPSocket( FileDescriptor&& fd ) : Socket( std::move( fd ), AF_INET, SOCK_STREAM, IPPROTO_TCP ) {}

public:
  //! Default: construct an unbound, unconnected TCP socket
  //! 默认构造函数：创建未绑定、未连接的 TCP 套接字
  //! \note SOCK_STREAM 表示流式套接字（TCP 的特征）
  TCPSocket() : Socket( AF_INET, SOCK_STREAM ) {}

  //! Mark a socket as listening for incoming connections
  //! 将套接字标记为监听传入连接
  //! \param[in] backlog 等待连接队列的最大长度（默认16）
  //! \note 服务器套接字调用此函数后才能接受客户端连接
  void listen( int backlog = 16 );

  //! Accept a new incoming connection
  //! 接受新的传入连接
  //! \return 返回新的 TCPSocket 对象，代表与客户端的连接
  //! \note 这是阻塞调用，会等待直到有客户端连接
  //! \note 服务器使用这个函数来处理客户端连接请求
  TCPSocket accept();
};

//! A wrapper around [packet sockets](\ref man7:packet)
//! [包套接字] 的封装类
//! 
//! 继承关系：FileDescriptor -> Socket -> DatagramSocket -> PacketSocket
//! 
//! 包套接字特点：
//! - 原始套接字：可以访问数据链路层的原始数据包
//! - 绕过协议栈：直接发送/接收以太网帧
//! - 需要特权：通常需要 root 权限才能创建
//! - 适用场景：网络分析工具、自定义协议实现、网络监控
class PacketSocket : public DatagramSocket
{
public:
  //! 构造包套接字
  //! \param[in] type 套接字类型（如 SOCK_RAW）
  //! \param[in] protocol 协议类型（如 ETH_P_ALL 表示所有协议）
  //! \note AF_PACKET 是 Linux 特有的地址族，用于包级别的网络访问
  PacketSocket( const int type, const int protocol ) : DatagramSocket( AF_PACKET, type, protocol ) {}

  //! 设置网卡为混杂模式
  //! \note 混杂模式允许网卡接收所有经过的数据包，不只是发给本机的
  //! \warning 需要管理员权限，且会影响网络性能
  void set_promiscuous();
};

//! A wrapper around [Unix-domain stream sockets](\ref man7::unix)
//! [Unix 域流套接字] 的封装类
//! 
//! 继承关系：FileDescriptor -> Socket -> LocalStreamSocket
//! 
//! Unix 域套接字特点：
//! - 本地通信：只能在同一台机器上的进程间通信
//! - 高效：不需要网络协议栈，直接在内核中传输数据
//! - 可靠：基于流的可靠传输，类似 TCP 但更快
//! - 文件系统命名：使用文件系统路径作为地址
//! - 适用场景：本地进程间通信、数据库连接、Web服务器与应用服务器通信
class LocalStreamSocket : public Socket
{
public:
  //! Construct from a file descriptor
  //! 从文件描述符构造本地流套接字
  //! \param[in] fd 移动语义的文件描述符对象
  //! \note AF_UNIX 表示 Unix 域，SOCK_STREAM 表示流式传输
  explicit LocalStreamSocket( FileDescriptor&& fd ) : Socket( std::move( fd ), AF_UNIX, SOCK_STREAM ) {}
};

//! A wrapper around [Unix-domain datagram sockets](\ref man7::unix)
//! [Unix 域数据报套接字] 的封装类
//! 
//! 继承关系：FileDescriptor -> Socket -> DatagramSocket -> LocalDatagramSocket
//! 
//! Unix 域数据报套接字特点：
//! - 本地数据报通信：在本地进程间传输数据报
//! - 可靠：Unix 域套接字在本地通信中是可靠的（不像网络 UDP）
//! - 保持消息边界：每次发送的数据作为独立的消息
//! - 高效：比网络套接字更快，因为不需要网络协议栈处理
class LocalDatagramSocket : public DatagramSocket
{
private:
  //! 私有构造函数，从文件描述符构造
  //! \param[in] fd 移动语义的文件描述符对象
  //! \note AF_UNIX 表示 Unix 域，SOCK_DGRAM 表示数据报类型
  explicit LocalDatagramSocket( FileDescriptor&& fd ) : DatagramSocket( std::move( fd ), AF_UNIX, SOCK_DGRAM ) {}

public:
  //! Default: construct an unbound, unconnected socket
  //! 默认构造函数：创建未绑定、未连接的本地数据报套接字
  LocalDatagramSocket() : DatagramSocket( AF_UNIX, SOCK_DGRAM ) {}
};
