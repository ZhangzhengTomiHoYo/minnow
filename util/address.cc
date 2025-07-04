#include "address.hh"

#include "exception.hh"

#include <array>      // 提供 std::array 容器
#include <cstring>    // 提供 memcpy, memcmp 等C风格字符串函数
#include <linux/if_packet.h>  // 提供 sockaddr_ll 结构体（Linux包级别套接字）
#include <memory>     // 提供 unique_ptr 智能指针
#include <netdb.h>    // 提供 getaddrinfo, getnameinfo 等网络数据库函数
#include <stdexcept>  // 提供 runtime_error 等异常类
#include <sys/socket.h>  // 提供 sockaddr 等套接字相关结构体
#include <system_error>  // 提供 error_category 等系统错误处理

using namespace std;

//! Converts Raw to `sockaddr *`.
//! 将 Raw 对象转换为 `sockaddr *` 指针。
//! 这是一个类型转换运算符，允许 Raw 对象自动转换为 sockaddr* 类型
//! reinterpret_cast: 强制类型转换，将 sockaddr_storage 的地址重新解释为 sockaddr*
Address::Raw::operator sockaddr*()
{
  return reinterpret_cast<sockaddr*>( &storage ); // NOLINT(*-reinterpret-cast)
}

//! Converts Raw to `const sockaddr *`.
//! 将 Raw 对象转换为 `const sockaddr *` 指针（常量版本）。
//! const 版本的类型转换运算符，用于 const Raw 对象
//! 返回的指针指向常量，不能通过此指针修改数据
Address::Raw::operator const sockaddr*() const
{
  return reinterpret_cast<const sockaddr*>( &storage ); // NOLINT(*-reinterpret-cast)
}

//! \param[in] addr points to a raw socket address
//! \param[in] size is `addr`'s length
//! \param[in] addr 指向原始套接字地址的指针
//! \param[in] size 是 `addr` 的长度（以字节为单位）
//! 从现有的 sockaddr 结构体构造 Address 对象
//! 这个构造函数将外部的 sockaddr 数据复制到内部的 storage 中
Address::Address( const sockaddr* addr, const size_t size ) : _size( size )
{
  // make sure proposed sockaddr can fit
  // 确保提供的 sockaddr 结构体能够放入我们的存储空间
  if ( size > sizeof( _address.storage ) ) {
    throw runtime_error( "invalid sockaddr size" );
  }

  // memcpy: C 库函数，用于内存复制
  // 将 addr 指向的数据复制 size 个字节到 _address.storage
  memcpy( &_address.storage, addr, size );
}

//! Error category for getaddrinfo and getnameinfo failures.
//! 用于 getaddrinfo 和 getnameinfo 函数失败时的错误类别。
//! error_category: C++ 标准库中用于分类系统错误的基类
//! getaddrinfo: 将主机名/服务名转换为地址信息的函数
//! getnameinfo: 将地址信息转换为主机名/服务名的函数
class gai_error_category : public error_category
{
public:
  //! The name of the wrapped error
  //! 被封装错误的名称
  //! noexcept: 保证此函数不会抛出异常
  //! override: 明确表示这是重写父类的虚函数
  const char* name() const noexcept override { return "gai_error_category"; }
  
  //! \brief An error message
  //! \param[in] return_value the error return value from [getaddrinfo(3)](\ref man3::getaddrinfo)
  //!                         or [getnameinfo(3)](\ref man3::getnameinfo)
  //! \brief 错误消息
  //! \param[in] return_value 来自 [getaddrinfo(3)] 或 [getnameinfo(3)] 的错误返回值
  //! gai_strerror: 将 getaddrinfo 错误码转换为可读字符串的函数
  string message( const int return_value ) const noexcept override { return gai_strerror( return_value ); }
};

//! \param[in] node is the hostname or dotted-quad address
//! \param[in] service is the service name or numeric string
//! \param[in] hints are criteria for resolving the supplied name
//! \param[in] node 是主机名或点分四组地址（如 "www.google.com" 或 "192.168.1.1"）
//! \param[in] service 是服务名或数字字符串（如 "http" 或 "80"）
//! \param[in] hints 是解析提供名称的条件/参数
//! 这是私有构造函数，用于执行实际的 DNS 解析工作
Address::Address( const string& node, const string& service, const addrinfo& hints ) : _size()
{
  // prepare for the answer
  // 准备接收解析结果
  // addrinfo*: 指向地址信息链表的指针，getaddrinfo 会分配内存并填充结果
  addrinfo* resolved_address = nullptr;

  // look up the name or names
  // 查找名称或多个名称
  // getaddrinfo: 网络地址和服务转换函数，将主机名和服务名转换为套接字地址
  // c_str(): 将 C++ string 转换为 C 风格的字符串指针
  const int gai_ret = getaddrinfo( node.c_str(), service.c_str(), &hints, &resolved_address );
  if ( gai_ret != 0 ) {
    // tagged_error: 项目自定义的异常类，包含错误类别和描述信息
    throw tagged_error( gai_error_category(), "getaddrinfo(" + node + ", " + service + ")", gai_ret );
  }

  // if success, should always have at least one entry
  // 如果成功，应该总是至少有一个条目
  if ( resolved_address == nullptr ) {
    throw runtime_error( "getaddrinfo returned successfully but with no results" );
  }

  // put resolved_address in a wrapper so it will get freed if we have to throw an exception
  // 将 resolved_address 放入包装器中，这样如果我们必须抛出异常时它会被自动释放
  // lambda 表达式: []() {} 语法，这里定义了一个删除器函数
  // freeaddrinfo: 释放 getaddrinfo 分配的内存
  auto addrinfo_deleter = []( addrinfo* const x ) { freeaddrinfo( x ); };
  // unique_ptr: 智能指针，自动管理内存，当对象销毁时自动调用删除器
  // decltype: 获取表达式的类型
  // move: 将左值转换为右值引用，避免不必要的复制
  unique_ptr<addrinfo, decltype( addrinfo_deleter )> wrapped_address( resolved_address, move( addrinfo_deleter ) );

  // assign to our private members (making sure size fits)
  // 赋值给我们的私有成员（确保大小合适）
  // *this = : 使用赋值运算符给当前对象赋值
  // ai_addr: addrinfo 结构体中的套接字地址指针
  // ai_addrlen: addrinfo 结构体中的地址长度
  *this = Address( wrapped_address->ai_addr, wrapped_address->ai_addrlen );
}

//! \brief Build a `struct addrinfo` containing hints for [getaddrinfo(3)](\ref man3::getaddrinfo)
//! \param[in] ai_flags is the value of the `ai_flags` field in the [struct addrinfo](\ref man3::getaddrinfo)
//! \param[in] ai_family is the value of the `ai_family` field in the [struct addrinfo](\ref
//! man3::getaddrinfo)
//! \brief 构建一个包含 [getaddrinfo(3)] 提示信息的 `struct addrinfo`
//! \param[in] ai_flags 是 [struct addrinfo] 中 `ai_flags` 字段的值（控制解析行为的标志）
//! \param[in] ai_family 是 [struct addrinfo] 中 `ai_family` 字段的值（地址族，如 AF_INET）
//! inline: 内联函数，建议编译器在调用处展开函数代码以提高性能
//! NOLINT: 告诉代码检查工具忽略特定的警告
inline addrinfo make_hints( int ai_flags, int ai_family ) // NOLINT(*-swappable-parameters)
{
  // value initialized to all zeros
  // 值初始化为全零
  // {}: 大括号初始化，将结构体的所有成员初始化为零
  addrinfo hints {}; 
  hints.ai_flags = ai_flags;    // 设置解析标志（如 AI_NUMERICHOST 表示不解析主机名）
  hints.ai_family = ai_family;  // 设置地址族（如 AF_INET 表示 IPv4）
  return hints;
}

//! \param[in] hostname to resolve
//! \param[in] service name (from `/etc/services`, e.g., "http" is port 80)
//! \param[in] hostname 要解析的主机名（如 "www.google.com" 或 "localhost"）
//! \param[in] service 服务名（来自 `/etc/services`，例如 "http" 对应端口 80）
//! 公有构造函数：通过主机名和服务名构造 Address
//! 使用委托构造函数调用私有构造函数
//! AI_ALL: 返回所有匹配的地址，AF_INET: 仅限 IPv4
Address::Address( const string& hostname, const string& service )
  : Address( hostname, service, make_hints( AI_ALL, AF_INET ) )
{}

//! \param[in] ip address as a dotted quad ("1.1.1.1")
//! \param[in] port number
//! \param[in] ip 点分十进制形式的 IP 地址（如 "1.1.1.1"）
//! \param[in] port 端口号（0-65535）
//! 公有构造函数：通过 IP 地址字符串和端口号构造 Address
//! uint16_t: 16位无符号整数类型，范围 0-65535，正好适合端口号
Address::Address( const string& ip, const uint16_t port )
  // tell getaddrinfo that we don't want to resolve anything
  // 告诉 getaddrinfo 我们不想解析任何内容（直接使用提供的 IP 和端口）
  // ::to_string: 全局作用域的 to_string 函数，将数字转换为字符串
  // AI_NUMERICHOST: 不解析主机名，treat node 参数为数字 IP 地址
  // AI_NUMERICSERV: 不解析服务名，treat service 参数为数字端口号
  : Address( ip, ::to_string( port ), make_hints( AI_NUMERICHOST | AI_NUMERICSERV, AF_INET ) )
{}

// accessors
// 访问器函数（用于获取 Address 对象的属性）
// pair<string, uint16_t>: 标准库的 pair 类型，存储一对值（IP字符串，端口号）
pair<string, uint16_t> Address::ip_port() const
{
  // 检查地址族是否为 Internet 地址（IPv4 或 IPv6）
  // ss_family: sockaddr_storage 结构体中的地址族字段
  // and/or: C++ 中的替代运算符，等价于 && 和 ||
  if ( _address.storage.ss_family != AF_INET and _address.storage.ss_family != AF_INET6 ) {
    throw runtime_error( "Address::ip_port() called on non-Internet address" );
  }

  // array<char, N>: 标准库的固定大小数组
  // NI_MAXHOST: 最大主机名长度常量（通常是 1025）
  // NI_MAXSERV: 最大服务名长度常量（通常是 32）
  // {}: 初始化数组元素为零
  array<char, NI_MAXHOST> ip {};
  array<char, NI_MAXSERV> port {};

  // getnameinfo: 将套接字地址转换为主机名和服务名的函数
  // static_cast: 安全的类型转换，用于相关类型之间的转换
  // data(): 返回数组底层数据的指针
  // size(): 返回数组的大小
  // NI_NUMERICHOST: 返回数字 IP 地址而不是主机名
  // NI_NUMERICSERV: 返回数字端口号而不是服务名
  const int gni_ret = getnameinfo( static_cast<const sockaddr*>( _address ),
                                   _size,
                                   ip.data(),
                                   ip.size(),
                                   port.data(),
                                   port.size(),
                                   NI_NUMERICHOST | NI_NUMERICSERV );
  if ( gni_ret != 0 ) {
    throw tagged_error( gai_error_category(), "getnameinfo", gni_ret );
  }

  // stoi: string to integer，将字符串转换为整数
  // {}: 大括号初始化，创建 pair 对象
  return { ip.data(), stoi( port.data() ) };
}

// 将 Address 转换为可读的字符串表示形式
string Address::to_string() const
{
  // 如果是 Internet 地址（IPv4 或 IPv6）
  if ( _address.storage.ss_family == AF_INET or _address.storage.ss_family == AF_INET6 ) {
    // auto: 自动类型推导，这里推导为 pair<string, uint16_t>
    const auto ip_and_port = ip_port();
    // first: pair 的第一个元素（IP 地址字符串）
    // second: pair 的第二个元素（端口号）
    // ::to_string: 将端口号转换为字符串
    return ip_and_port.first + ":" + ::to_string( ip_and_port.second );
  }

  // 如果不是 Internet 地址，返回通用描述
  return "(non-Internet address)";
}

// 将 IPv4 地址转换为 32 位数字表示
// uint32_t: 32位无符号整数，IPv4 地址刚好是 32 位
uint32_t Address::ipv4_numeric() const
{
  // 检查是否为 IPv4 地址且大小正确
  // sizeof(sockaddr_in): IPv4 地址结构体的大小
  if ( _address.storage.ss_family != AF_INET or _size != sizeof( sockaddr_in ) ) {
    throw runtime_error( "ipv4_numeric called on non-IPV4 address" );
  }

  // 创建一个 IPv4 专用的地址结构体
  sockaddr_in ipv4_addr {};
  // 将通用存储中的数据复制到 IPv4 专用结构体
  memcpy( &ipv4_addr, &_address.storage, _size );

  // be32toh: big-endian 32-bit to host byte order
  // 将网络字节序（大端）转换为主机字节序
  // sin_addr.s_addr: IPv4 地址的 32 位数字表示
  return be32toh( ipv4_addr.sin_addr.s_addr );
}

// 从 32 位数字创建 IPv4 Address 对象
// static: 静态成员函数，不需要对象实例就可以调用
Address Address::from_ipv4_numeric( const uint32_t ip_address )
{
  // 创建 IPv4 地址结构体
  sockaddr_in ipv4_addr {};
  ipv4_addr.sin_family = AF_INET;  // 设置为 IPv4 地址族
  // htobe32: host to big-endian 32-bit
  // 将主机字节序转换为网络字节序（大端）
  ipv4_addr.sin_addr.s_addr = htobe32( ip_address );

  // 使用构造函数创建 Address 对象
  // reinterpret_cast: 将 sockaddr_in* 重新解释为 sockaddr*
  return { reinterpret_cast<sockaddr*>( &ipv4_addr ), sizeof( ipv4_addr ) }; // NOLINT(*-reinterpret-cast)
}

// equality
// 相等性比较
// 重载 == 运算符，比较两个 Address 对象是否相等
bool Address::operator==( const Address& other ) const
{
  // 首先比较大小，如果大小不同则肯定不相等
  if ( _size != other._size ) {
    return false;
  }

  // memcmp: 内存比较函数，逐字节比较两块内存
  // 返回 0 表示两块内存内容完全相同
  // 比较两个 Address 对象的原始地址数据
  return 0 == memcmp( &_address, &other._address, _size );
}

// address families that correspond to each sockaddr type
// 与每种 sockaddr 类型对应的地址族
// 模板变量：为不同的 sockaddr 类型定义对应的地址族常量
// constexpr: 编译时常量，值在编译时确定
// 默认值 -1 表示未知或不支持的类型
template<typename sockaddr_type>
constexpr int sockaddr_family = -1;

// 模板特化：为特定的 sockaddr 类型指定具体的地址族值
// sockaddr_in: IPv4 地址结构体，对应 AF_INET 地址族
template<>
constexpr int sockaddr_family<sockaddr_in> = AF_INET;

// sockaddr_in6: IPv6 地址结构体，对应 AF_INET6 地址族
template<>
constexpr int sockaddr_family<sockaddr_in6> = AF_INET6;

// sockaddr_ll: Linux 包级别套接字，对应 AF_PACKET 地址族
template<>
constexpr int sockaddr_family<sockaddr_ll> = AF_PACKET;

// safely cast the address to its underlying sockaddr type
// 安全地将地址转换为其底层的 sockaddr 类型
// 模板函数：可以转换为任意指定的 sockaddr 类型
// typename: 告诉编译器 sockaddr_type 是一个类型名
template<typename sockaddr_type>
const sockaddr_type* Address::as() const
{
  // 获取通用 sockaddr 指针
  const sockaddr* raw { _address };
  
  // 安全性检查：
  // 1. 检查目标类型的大小是否不超过实际存储的大小
  // 2. 检查地址族是否匹配
  // sa_family: sockaddr 结构体中的地址族字段
  if ( sizeof( sockaddr_type ) < size() or raw->sa_family != sockaddr_family<sockaddr_type> ) {
    throw runtime_error( "Address::as() conversion failure" );
  }

  // 类型转换：将通用 sockaddr* 转换为特定的 sockaddr_type*
  return reinterpret_cast<const sockaddr_type*>( raw ); // NOLINT(*-reinterpret-cast)
}

// 显式实例化模板函数：强制编译器为这些类型生成具体的函数代码
// 这样其他编译单元就可以使用这些特定类型的转换函数
// template: 显式实例化关键字
template const sockaddr_in* Address::as<sockaddr_in>() const;     // IPv4 转换
template const sockaddr_in6* Address::as<sockaddr_in6>() const;   // IPv6 转换  
template const sockaddr_ll* Address::as<sockaddr_ll>() const;     // 包级别转换
