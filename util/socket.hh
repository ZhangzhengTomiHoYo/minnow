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
  Socket( int domain, int type, int protocol = 0 );

  //! Construct from a file descriptor.
  Socket( FileDescriptor&& fd, int domain, int type, int protocol = 0 );

  //! Wrapper around [getsockopt(2)](\ref man2::getsockopt)
  template<typename option_type>
  socklen_t getsockopt( int level, int option, option_type& option_value ) const;

  //! Wrappers around [setsockopt(2)](\ref man2::setsockopt)
  template<typename option_type>
  void setsockopt( int level, int option, const option_type& option_value );

  void setsockopt( int level, int option, std::string_view option_val );

public:
  //! Bind a socket to a specified address with [bind(2)](\ref man2::bind), usually for listen/accept
  void bind( const Address& address );

  //! Bind a socket to a specified device
  void bind_to_device( std::string_view device_name );

  //! Connect a socket to a specified peer address with [connect(2)](\ref man2::connect)
  void connect( const Address& address );

  //! Shut down a socket via [shutdown(2)](\ref man2::shutdown)
  void shutdown( int how );

  //! Get local address of socket with [getsockname(2)](\ref man2::getsockname)
  Address local_address() const;
  //! Get peer address of socket with [getpeername(2)](\ref man2::getpeername)
  Address peer_address() const;

  //! Allow local address to be reused sooner via [SO_REUSEADDR](\ref man7::socket)
  void set_reuseaddr();

  //! Check for errors (will be seen on non-blocking sockets)
  void throw_if_error() const;
};

class DatagramSocket : public Socket
{
public:
  //! Receive a datagram and the Address of its sender
  void recv( Address& source_address, std::string& payload );

  //! Send a datagram to specified Address
  void sendto( const Address& destination, std::string_view payload );

  //! Send datagram to the socket's connected address (must call connect() first)
  void send( std::string_view payload );

protected:
  DatagramSocket( int domain, int type, int protocol = 0 ) : Socket( domain, type, protocol ) {}

  //! Construct from a file descriptor.
  DatagramSocket( FileDescriptor&& fd, int domain, int type, int protocol = 0 )
    : Socket( std::move( fd ), domain, type, protocol )
  {}
};

//! A wrapper around [UDP sockets](\ref man7::udp)
class UDPSocket : public DatagramSocket
{
  //! \param[in] fd is the FileDescriptor from which to construct
  explicit UDPSocket( FileDescriptor&& fd ) : DatagramSocket( std::move( fd ), AF_INET, SOCK_DGRAM ) {}

public:
  //! Default: construct an unbound, unconnected UDP socket
  UDPSocket() : DatagramSocket( AF_INET, SOCK_DGRAM ) {}
};

//! A wrapper around [TCP sockets](\ref man7::tcp)
class TCPSocket : public Socket
{
private:
  //! \brief Construct from FileDescriptor (used by accept())
  //! \param[in] fd is the FileDescriptor from which to construct
  explicit TCPSocket( FileDescriptor&& fd ) : Socket( std::move( fd ), AF_INET, SOCK_STREAM, IPPROTO_TCP ) {}

public:
  //! Default: construct an unbound, unconnected TCP socket
  TCPSocket() : Socket( AF_INET, SOCK_STREAM ) {}

  //! Mark a socket as listening for incoming connections
  void listen( int backlog = 16 );

  //! Accept a new incoming connection
  TCPSocket accept();
};

//! A wrapper around [packet sockets](\ref man7:packet)
class PacketSocket : public DatagramSocket
{
public:
  PacketSocket( const int type, const int protocol ) : DatagramSocket( AF_PACKET, type, protocol ) {}

  void set_promiscuous();
};

//! A wrapper around [Unix-domain stream sockets](\ref man7::unix)
class LocalStreamSocket : public Socket
{
public:
  //! Construct from a file descriptor
  explicit LocalStreamSocket( FileDescriptor&& fd ) : Socket( std::move( fd ), AF_UNIX, SOCK_STREAM ) {}
};

//! A wrapper around [Unix-domain datagram sockets](\ref man7::unix)
class LocalDatagramSocket : public DatagramSocket
{
  //! \param[in] fd is the FileDescriptor from which to construct
  explicit LocalDatagramSocket( FileDescriptor&& fd ) : DatagramSocket( std::move( fd ), AF_UNIX, SOCK_DGRAM ) {}

public:
  //! Default: construct an unbound, unconnected socket
  LocalDatagramSocket() : DatagramSocket( AF_UNIX, SOCK_DGRAM ) {}
};
