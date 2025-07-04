#pragma once

#include <cstddef>
#include <cstdint>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <utility>

//! Wrapper around [IPv4 addresses](@ref man7::ip) and DNS operations.
//! 对 [IPv4 地址] 和 DNS 操作的封装器。
class Address
{
public:
/* Raw 嵌套类 （知识补充）
  在 C++ 中，一个类（如 Address）可以在其内部声明另一个类（如 Raw），这种做法叫做“嵌套类”或“内部类”。
  为什么要这样做？
  1.逻辑归属清晰
    Raw 只为 Address 服务，不希望被 Address 以外的代码直接使用。把 Raw 放在 Address 里面，表达了“Raw 是 Address 的内部实现细节”。
  2.封装实现细节
    Raw 主要用于封装底层的 sockaddr_storage 结构体和相关转换操作。外部用户只需要用 Address，不需要关心 Raw 的存在。
  3.避免命名冲突
    Raw 这个名字很通用，放在 Address 里可以避免和其他地方的 Raw 冲突。
*/
  //! \brief Wrapper around [sockaddr_storage](@ref man7::socket).
  //! \details A `sockaddr_storage` is enough space to store any socket address (IPv4 or IPv6).
  //! \brief 对 [sockaddr_storage] 的封装。
  //! \details 一个 `sockaddr_storage` 结构体有足够的空间来存储任何套接字地址（无论是 IPv4 还是 IPv6）。
  /*
  sockaddr_storage 是操作系统提供的一个结构体，用于存储任意类型的网络地址（如 IPv4、IPv6、Unix 域等），它比 sockaddr_in（只支持 IPv4）更通用。
  在你的项目中，Address::Raw 这个内部类就是用来封装 sockaddr_storage，让 Address 类可以方便地存储和管理各种类型的网络地址。
  这样设计的好处是：无论你用的是 IPv4 还是 IPv6 地址，Address 类都能统一处理，简化了网络编程的复杂性。
  举例说明
  当你用 Address 类表示一个 IP 地址时，底层其实就是把地址信息存进了 sockaddr_storage 结构体里。
  这样，Socket 类等网络相关的类就可以通过 Address 统一地获取、传递和操作地址信息，而不用关心底层到底是 IPv4 还是 IPv6。
  */
  class Raw
  {
  public:
    //这个类型是在 <sys/socket.h> 头文件中定义的，不属于 C++ 的标准命名空间（std），而是 POSIX（Unix/Linux）网络编程的标准类型。
    sockaddr_storage storage {}; //!< The wrapped struct itself. 被封装的结构体本身。
    // 这行注释的作用是：让代码检查工具不要对下面的类型转换运算符报“缺少 explicit 关键字”的警告。
    // 对理解和使用 C++ 代码没有影响，只是为了让代码规范检查更灵活。
    // NOLINTBEGIN (*-explicit-*)
    /*
      /NOTE 基础知识大讲堂
      在 C++ 里，operator 关键字可以用来重载运算符，也可以用来定义“类型转换运算符”。

      1. operator sockaddr*(); 和 operator const sockaddr*() const; 的区别
          operator sockaddr*();
            没有 const，表示这是一个普通成员函数。
            可以被非 const 对象调用。
            返回的是 sockaddr*，也就是可以通过这个指针修改底层数据。
          operator const sockaddr*() const;
            第一个 const：修饰返回值，表示返回的是指向常量的指针，不能通过这个指针修改数据。
            最后的 const：修饰成员函数，表示这个函数不会修改类的成员变量，可以被 const 对象调用。
    举例说明：
      Address::Raw raw;
      const Address::Raw& c_raw = raw;
      sockaddr* p1 = raw;      // 调用 operator sockaddr*()
      const sockaddr* p2 = c_raw; // 调用 operator const sockaddr*() const

    另外没有{}
    没有 {}，只有分号 ; 的意思是  声明，只是告诉编译器有这个函数，具体实现稍后在.cc给
    */
    operator sockaddr*();
    operator const sockaddr*() const;
    // NOLINTEND (*-explicit-*)
    //它的作用是：
    // 标记“忽略警告”的范围到此结束。也就是说，从 // NOLINTBEGIN (-explicit-) 到 // NOLINTEND (-explicit-) 之间的代码，
    // 静态分析工具不会对 explicit 相关的警告进行提示。
  };

private:
  socklen_t _size; //!< Size of the wrapped address. 被封装地址的大小。
  Raw _address {}; //!< A wrapped [sockaddr_storage](@ref man7::socket) containing the address. 一个封装了 [sockaddr_storage] 的对象，包含了地址信息。

  //! Constructor from ip/host, service/port, and hints to the resolver.
  //! 从 ip/主机名、服务/端口 和 解析器参数 构造 Address 对象的构造函数。
  Address( const std::string& node, const std::string& service, const addrinfo& hints );
  // 一个私有构造函数，只能在类内部或友元函数中调用，不能被外部代码直接使用。它可能是作为其他公有构造函数的内部实现辅助函数。

public:
  //! Construct by resolving a hostname and servicename.
  //! 通过解析主机名和服务名来构造 Address 对象。
  Address( const std::string& hostname, const std::string& service );

  //! Construct from dotted-quad string ("18.243.0.1") and numeric port.
  //! 从点分十进制字符串（如 "18.243.0.1"）和数字端口号构造 Address 对象。
  explicit Address( const std::string& ip, std::uint16_t port = 0 );

  //! Construct from a [sockaddr *](@ref man7::socket).
  //! 从 [sockaddr 指针] 构造 Address 对象。
  Address( const sockaddr* addr, std::size_t size );

  //! Equality comparison.
  //! 相等性比较操作符。
  bool operator==( const Address& other ) const;
  bool operator!=( const Address& other ) const { return not operator==( other ); }

  //! \name Conversions
  //!@{

  //! Dotted-quad IP address string ("18.243.0.1") and numeric port.
  std::pair<std::string, uint16_t> ip_port() const;
  //! Dotted-quad IP address string ("18.243.0.1").
  std::string ip() const { return ip_port().first; }
  //! Numeric port (host byte order).
  uint16_t port() const { return ip_port().second; }
  //! Numeric IP address as an integer (i.e., in [host byte order](\ref man3::byteorder)).
  uint32_t ipv4_numeric() const;
  //! Create an Address from a 32-bit raw numeric IP address
  static Address from_ipv4_numeric( uint32_t ip_address );
  //! Human-readable string, e.g., "8.8.8.8:53".
  std::string to_string() const;
  //!@}

  //! \name Low-level operations
  //!@{

  //! Size of the underlying address storage.
  socklen_t size() const { return _size; }
  //! Const pointer to the underlying socket address storage.
  const sockaddr* raw() const { return static_cast<const sockaddr*>( _address ); }
  //! Safely convert to underlying sockaddr type
  template<typename sockaddr_type>
  const sockaddr_type* as() const;

  //!@}
};
