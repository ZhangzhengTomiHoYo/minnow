/*
 * 📚 C++知识体系1：头文件和命名空间
 *
 * #include "address.hh" - 包含对应的头文件，获取类定义
 *
 * C++头文件分类:
 * 1. "" - 项目内部头文件，编译器会先在当前目录查找
 * 2. <> - 标准库或系统头文件，编译器在系统路径查找
 *
 * using namespace std;
 * - 作用：允许我们直接使用标准库的组件（如 string, cout）而无需加 std:: 前缀
 * - 注意：在头文件中应避免使用，但在 .cc/.cpp 文件中是常见的做法
 */
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

/*
 * 🔧 C++知识体系2：类型转换运算符和 reinterpret_cast
 *
 * 1. operator sockaddr*()
 *    - 语法：这是一个类型转换运算符重载
 *    - 作用：定义了如何将 Address::Raw 对象隐式或显式地转换为 sockaddr* 指针
 *    - 示例：Address::Raw raw; sockaddr* p = raw; // 自动调用
 *
 * 2. reinterpret_cast<sockaddr*>
 *    - 语法：C++中最底层的强制类型转换
 *    - 作用：将一个指针类型重新解释为另一个指针类型，不改变其二进制位
 *    - 风险：非常不安全，需要程序员100%确保转换的正确性
 *    - 为何使用：处理C风格的、基于通用结构体（如sockaddr_storage）的多态API所必需
 *
 * 3. const 版本
 *    - 作用：当 Address::Raw 对象是 const 时，会调用这个版本，返回 const sockaddr*
 *    - 保证了const正确性，即不能通过转换后的指针修改const对象
 */
Address::Raw::operator sockaddr*()
{
  return reinterpret_cast<sockaddr*>( &storage ); // NOLINT(*-reinterpret-cast)
}

Address::Raw::operator const sockaddr*() const
{
  return reinterpret_cast<const sockaddr*>( &storage ); // NOLINT(*-reinterpret-cast)
}

/*
 * 🏗️ C++知识体系3：构造函数和内存操作
 *
 * 1. Address::Address( const sockaddr* addr, const size_t size )
 *    - 作用：从一个已有的、C风格的 sockaddr 结构体创建 Address 对象
 *    - const sockaddr* addr: 指向输入数据的指针，const表示函数不会修改它
 *    - const size_t size: 输入数据的大小
 *
 * 2. : _size( size )
 *    - 语法：成员初始化列表
 *    - 作用：在构造函数体执行前，直接初始化成员变量 _size
 *    - 优点：比在函数体内赋值更高效，特别是对于类类型的成员
 *
 * 3. memcpy(void* dest, const void* src, size_t count)
 *    - 作用：C语言库函数，用于按字节复制内存区域
 *    - 在这里：将外部传入的 sockaddr 数据复制到 Address 内部的 _address.storage 中
 *    - 为什么需要：实现深拷贝，确保 Address 对象拥有自己独立的地址数据副本
 */
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

/*
 * 🛡️ C++知识体系4：自定义异常处理
 *
 * 1. class gai_error_category : public error_category
 *    - 作用：创建一个专门用于处理 getaddrinfo 错误的自定义错误类别
 *    - public error_category: 继承自标准库的 error_category 基类
 *
 * 2. const char* name() const noexcept override
 *    - override: 明确表示这个函数重写了基类的同名虚函数，如果基类没有，编译器会报错
 *    - noexcept: 向编译器承诺这个函数不会抛出任何异常，有助于优化
 *
 * 3. string message( const int return_value ) const noexcept override
 *    - 作用：将 getaddrinfo 返回的错误码（一个整数）转换为人类可读的错误信息字符串
 *    - gai_strerror(): C库函数，专门用于转换 getaddrinfo 的错误码
 *
 * 4. 为什么需要自定义错误类别？
 *    - 提供了类型安全的错误处理机制
 *    - 可以集成到C++的 <system_error> 框架中
 *    - 使错误信息更具体、更易于调试
 */
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

/*
 * 🌐 计算机网络知识体系1：DNS解析 (getaddrinfo)
 * 🧠 C++知识体系5：智能指针与RAII
 *
 * 1. getaddrinfo()
 *    - 作用：网络编程的核心函数，将人类可读的主机名和服务名（如 "www.google.com", "http"）
 *           解析为一个或多个机器可用的套接字地址结构（sockaddr）
 *    - C风格API：需要手动管理返回的链表内存（通过 freeaddrinfo 释放）
 *
 * 2. unique_ptr<addrinfo, decltype(addrinfo_deleter)>
 *    - 作用：这是RAII模式的绝佳体现，用C++智能指针包装C风格的API
 *    - unique_ptr: 独占所有权的智能指针，当它离开作用域时，会自动调用其删除器
 *    - decltype(addrinfo_deleter): 获取lambda表达式的类型，作为 unique_ptr 的第二个模板参数
 *    - auto addrinfo_deleter = [](addrinfo* x) { freeaddrinfo(x); };
 *      - 语法：一个lambda表达式，定义了自定义的删除器函数
 *      - 作用：告诉 unique_ptr 在销毁时应该调用 freeaddrinfo() 而不是默认的 delete
 *    - 最终效果：无论函数是正常返回还是因异常退出，getaddrinfo 分配的内存都保证会被释放，杜绝内存泄漏
 *
 * 3. *this = Address(...)
 *    - 作用：调用 Address 类的赋值运算符，用解析出的第一个地址结果来填充当前对象
 */
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

/*
 * ⚙️ C++知识体系6：辅助函数与内联优化
 *
 * 1. inline addrinfo make_hints(...)
 *    - inline: 建议编译器将此函数的代码直接在调用处展开，对于短小的函数可以减少函数调用开销，提高性能
 *
 * 2. addrinfo hints {};
 *    - 语法：C++11的值初始化（大括号初始化）
 *    - 作用：将结构体 `hints` 的所有成员初始化为零或默认值，这是一个安全可靠的初始化方法
 */
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

/*
 * ✨ C++知识体系7：委托构造函数 (Delegating Constructors)
 *
 * 1. Address::Address(...) : Address(...)
 *    - 语法：这是C++11引入的委托构造函数
 *    - 作用：一个构造函数可以调用同一个类的另一个构造函数来完成部分或全部的初始化工作
 *
 * 2. 为什么使用委托构造函数？
 *    - 减少代码重复：核心的解析逻辑都在那个最复杂的构造函数里
 *    - 提高可维护性：只需要在一个地方修改核心逻辑
 *    - 逻辑更清晰：这个构造函数的作用就是为核心构造函数提供一组默认的 `hints` 参数
 *
 * 3. const string& hostname
 *    - 语法：按常量引用传递字符串
 *    - 作用：避免了复制整个字符串的开销，提高了性能。const保证了函数不会修改传入的字符串
 *
 * 4. 这里的逻辑：
 *    - 这个构造函数接收主机名和服务名
 *    - 然后它调用了更底层的构造函数，并为它提供了默认的 `hints` (由 make_hints 创建)
 *    - 这样，用户只需要提供最简单的信息，而复杂的参数由构造函数内部处理
 */
Address::Address( const string& hostname, const string& service )
  : Address( hostname, service, make_hints( AI_ALL, AF_INET ) )
{}

/*****************************************************************************************
 * @brief 公有构造函数：通过主机名和服务名构造 Address 对象 (Public constructor: by hostname and service name)
 * 
 * @details
 * 这是一个方便用户使用的构造函数。它接收人类可读的主机名（如 "cs144.keithw.org"）
 * 和服务名（如 "http"），然后调用另一个更底层的私有构造函数来完成实际的地址解析工作。
 * This is a user-friendly constructor. It takes a human-readable hostname (e.g., "cs144.keithw.org")
 * and a service name (e.g., "http") and then calls another, more low-level private constructor
 * to perform the actual address resolution.
 *
 * C++ 语法点 (C++ Syntax Point): [委托构造函数 (Delegating Constructor)]
 * 这里的 `: Address(hostname, service, make_hints(AI_ALL, AF_INET))` 语法
 * 是 C++11 引入的委托构造函数。它允许一个构造函数调用同一个类中的另一个构造函数。
 * 这样做的好处是避免了在多个构造函数中重复编写相同的初始化代码，使得代码更简洁、易于维护。
 * 在这个例子中，当前构造函数将自己的工作“委托”给了另一个带有三个参数的构造函数。
 * The syntax `: Address(hostname, service, make_hints(AI_ALL, AF_INET))` is a C++11 feature
 * called a delegating constructor. It allows one constructor to call another constructor from the same class.
 * This avoids duplicating initialization code across multiple constructors, making the code cleaner and easier to maintain.
 * In this case, this constructor "delegates" its work to the other three-argument constructor.
 *
 * 网络编程知识点 (Networking Knowledge Point): [地址解析提示 (Address Resolution Hints)]
 * `make_hints(AI_ALL, AF_INET)` 函数创建了一个 `addrinfo` 结构体，用作 `getaddrinfo` 函数的“提示”。
 * - `AF_INET`: 指定我们只对 IPv4 地址感兴趣 (Address Family: INET for IPv4)。
 * - `AI_ALL`: 当与 `AI_V4MAPPED` 结合使用时，它会请求返回 IPv4 映射的 IPv6 地址以及纯 IPv6 地址。
 *   在这里单独使用时，其效果依赖于系统实现，但通常意味着获取所有可用的地址。
 *   这个构造函数为 `getaddrinfo` 设置了通用的默认选项，适用于大多数情况。
 * The `make_hints(AI_ALL, AF_INET)` function creates an `addrinfo` struct that serves as "hints" for the
 * `getaddrinfo` function.
 * - `AF_INET`: Specifies that we are only interested in IPv4 addresses.
 * - `AI_ALL`: When used with `AI_V4MAPPED`, it requests both IPv4-mapped IPv6 addresses and native IPv6 addresses.
 *   When used alone here, its effect can be system-dependent, but generally it implies getting all available addresses.
 *   This constructor sets general-purpose default options for `getaddrinfo` suitable for most cases.
 *
 * @param[in] hostname 要解析的主机名 (e.g., "www.google.com", "localhost")
 * @param[in] service 服务名，通常对应一个端口号 (e.g., "http" for port 80, from `/etc/services`)
 *****************************************************************************************/

/*
 * 🏗️ C++知识体系8：构造函数重载与委托
 *
 * 1. Address::Address( const string& ip, const uint16_t port )
 *    - 作用：这是 Address 类的另一个构造函数重载
 *    - 目的：提供一种通过IP地址字符串和端口号直接创建 Address 对象的方式，绕过DNS解析
 *
 * 2. : Address(ip, ::to_string(port), ...)
 *    - 委托构造：再次使用了委托构造函数
 *    - ::to_string(port): 调用全局命名空间的 to_string 函数将端口号(uint16_t)转换为字符串
 *    - make_hints(AI_NUMERICHOST | AI_NUMERICSERV, AF_INET):
 *      - AI_NUMERICHOST: 告诉 getaddrinfo 不要解析主机名，ip参数就是一个数字地址
 *      - AI_NUMERICSERV: 告诉 getaddrinfo 不要解析服务名，service参数就是一个数字端口
 *      - |: 位或运算符，用于组合多个标志位
 *    - 最终效果：高效地将IP和端口号填充到地址结构中，无需网络查询
 */
Address::Address( const string& ip, const uint16_t port )
  // tell getaddrinfo that we don't want to resolve anything
  // 告诉 getaddrinfo 我们不想解析任何内容（直接使用提供的 IP 和端口）
  // ::to_string: 全局作用域的 to_string 函数，将数字转换为字符串
  // AI_NUMERICHOST: 不解析主机名，treat node 参数为数字 IP 地址
  // AI_NUMERICSERV: 不解析服务名，treat service 参数为数字端口号
  : Address( ip, ::to_string( port ), make_hints( AI_NUMERICHOST | AI_NUMERICSERV, AF_INET ) )
{}

/*
 * 👁️ C++知识体系9：访问器方法与网络字节序
 *
 * 1. pair<string, uint16_t> Address::ip_port() const
 *    - const: 表示这个成员函数不会修改对象的状态，是只读操作
 *    - pair: 一个方便的模板类，用于存储一对值。这里用于同时返回IP和端口
 *
 * 2. getnameinfo()
 *    - 作用：与 getaddrinfo 相反，它将二进制的 sockaddr 地址转换为人类可读的主机名和端口号字符串
 *    - NI_NUMERICHOST | NI_NUMERICSERV: 标志位，强制函数返回数字形式的IP和端口，而不是尝试反向DNS解析
 *
 * 3. uint32_t Address::ipv4_numeric() const
 *    - 作用：将IPv4地址从其结构体表示形式转换为一个32位的无符号整数
 *
 * 4. be32toh() / htobe32()
 *    - 🌐 网络知识：字节序 (Byte Order)
 *    - 网络字节序 (Big-Endian): 高位字节在前，是网络传输的标准
 *    - 主机字节序 (Host Byte Order): 根据CPU架构不同，可能是Big-Endian或Little-Endian
 *    - be32toh: Big-Endian 32-bit to Host (网络转主机)
 *    - htobe32: Host to Big-Endian 32-bit (主机转网络)
 *    - 为什么重要：所有进出网络的数据都必须转换为统一的网络字节序，否则不同架构的机器无法正确通信
 */
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

/*
 * 🏭 C++知识体系10：静态成员函数 (Static Member Functions)
 *
 * 1. static Address Address::from_ipv4_numeric(...)
 *    - static: 表示这是一个静态成员函数
 *    - 特点：
 *      - 不与任何特定的对象实例关联
 *      - 不能访问非静态成员变量（如 _size）
 *      - 可以通过类名直接调用：Address::from_ipv4_numeric(...)
 *    - 作用：通常用作工厂函数，用于创建类的实例
 */
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

/*
 * ⚖️ C++知识体系11：运算符重载
 *
 * 1. bool Address::operator==( const Address& other ) const
 *    - 作用：重载了 `==` 运算符，定义了如何比较两个 Address 对象是否相等
 *
 * 2. memcmp()
 *    - 作用：C库函数，用于按字节比较两块内存区域
 *    - 返回值：0表示相等，<0表示第一块小，>0表示第一块大
 *    - 在这里：这是比较两个地址是否相等的最高效方式，因为它直接比较底层的二进制数据
 */

/*****************************************************************************************
 * @brief 比较两个 Address 对象是否相等 (Compare two Address objects for equality)
 * 
 * @details
 * 这个函数重载了 `==` 运算符，使得可以直接比较两个 `Address` 对象。
 * This function overloads the `==` operator, allowing direct comparison of two `Address` objects.
 *
 * C++ 语法点 (C++ Syntax Point): [运算符重载 (Operator Overloading)]
 * `bool operator==(const Address& a, const Address& b)` 是一个非成员函数，
 * 它定义了 `Address == Address` 的行为。通常将比较运算符实现为非成员函数，
 * 以保持对称性（例如，允许 `a == b` 和 `b == a` 都能工作）。
 * `bool operator==(const Address& a, const Address& b)` is a non-member function that defines
 * the behavior of `Address == Address`. Comparison operators are often implemented as non-member
 * functions to maintain symmetry (e.g., allowing both `a == b` and `b == a` to work).
 *
 * 实现逻辑 (Implementation Logic):
 * 1. 首先比较地址大小 `_size`。如果大小不同，地址肯定不同。
 * 2. 如果大小相同，则使用 `memcmp` 按字节比较 `_address.storage` 的内容。
 *    `memcmp` 是一个 C 库函数，用于比较两块内存区域。如果内容完全相同，它返回 0。
 * 1. First, compare the address sizes `_size`. If the sizes are different, the addresses are definitely different.
 * 2. If the sizes are the same, use `memcmp` to perform a byte-by-byte comparison of the contents of `_address.storage`.
 *    `memcmp` is a C library function for comparing two memory areas. It returns 0 if the contents are identical.
 *
 * @param a 第一个 Address 对象 (The first Address object)
 * @param b 第二个 Address 对象 (The second Address object)
 * @return bool 如果相等则为 true，否则为 false (true if equal, false otherwise)
 *****************************************************************************************/
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

/*
 * 🔬 C++知识体系12：模板元编程 (Template Metaprogramming)
 *
 * 1. template<typename sockaddr_type> constexpr int sockaddr_family = -1;
 *    - 语法：C++14的模板变量
 *    - constexpr: 保证在编译时求值
 *    - 作用：为不同的套接字地址类型（如 sockaddr_in）在编译时关联一个地址族常量（如 AF_INET）
 *
 * 2. template<> constexpr int sockaddr_family<sockaddr_in> = AF_INET;
 *    - 语法：模板特化 (Template Specialization)
 *    - 作用：为特定的类型（这里是 sockaddr_in）提供一个专门的实现
 *
 * 3. template<typename sockaddr_type> const sockaddr_type* Address::as() const
 *    - 作用：一个模板成员函数，可以将通用的 Address 对象安全地转换为具体的地址类型
 *           （如 sockaddr_in* 或 sockaddr_in6*）
 *    - 安全性：在转换前，它会检查大小和地址族是否匹配，如果类型不匹配则抛出异常
 *
 * 4. template const sockaddr_in* Address::as<sockaddr_in>() const;
 *    - 语法：显式模板实例化 (Explicit Template Instantiation)
 *    - 作用：告诉编译器在这个编译单元（.cc文件）中生成特定类型的函数代码
 *    - 为什么需要：可以减少编译时间，并将模板代码的实现细节隐藏在.cc文件中
 */
template<typename sockaddr_type>
constexpr int sockaddr_family = -1;

template<>
constexpr int sockaddr_family<sockaddr_in> = AF_INET;

template<>
constexpr int sockaddr_family<sockaddr_in6> = AF_INET6;

template<>
constexpr int sockaddr_family<sockaddr_ll> = AF_PACKET;

template<typename sockaddr_type>
const sockaddr_type* Address::as() const
{
  const sockaddr* raw { _address };
  if ( sizeof( sockaddr_type ) < size() or raw->sa_family != sockaddr_family<sockaddr_type> ) {
    throw runtime_error( "Address::as() conversion failure" );
  }
  return reinterpret_cast<const sockaddr_type*>( raw ); // NOLINT(*-reinterpret-cast)
}

template const sockaddr_in* Address::as<sockaddr_in>() const;
template const sockaddr_in6* Address::as<sockaddr_in6>() const;
template const sockaddr_ll* Address::as<sockaddr_ll>() const;

/*****************************************************************************************
 * @brief 私有构造函数：通过主机名、服务名和 addrinfo 提示来构造 Address (Private constructor: by hostname, service, and hints)
 * 
 * @details
 * 这是执行实际地址解析的核心构造函数。它调用 POSIX 的 `getaddrinfo` 函数，
 * 该函数是现代网络编程中用于将人类可读的主机名和服务名转换为网络套接字可以使用的
 * `sockaddr` 结构的首选方法。
 * This is the core constructor that performs the actual address resolution. It calls the POSIX function
 * `getaddrinfo`, which is the preferred method in modern network programming for translating human-readable
 * hostnames and service names into `sockaddr` structures that network sockets can use.
 *
 * C++ 语法点 (C++ Syntax Point): [RAII 与智能指针 (RAII and Smart Pointers)]
 * `getaddrinfo` 会在 `res` 指针中动态分配一个 `addrinfo` 结构体链表。这块内存必须
 * 手动调用 `freeaddrinfo` 来释放，否则会造成内存泄漏。为了确保即使在发生异常或
 * 函数提前返回时也能正确释放资源，这里使用了 `std::shared_ptr` 配合自定义删除器 `FreeAddrInfo`。
 * - `shared_ptr<addrinfo> res_ptr(res, FreeAddrInfo())`:
 *   - `res` 是从 `getaddrinfo` 获取的原始指针。
 *   - `FreeAddrInfo()` 是一个函数对象（或带有 `operator()` 的结构体），当 `shared_ptr`
 *     的引用计数降为零时，它会被自动调用来释放 `res` 指向的内存。
 *   - 这就是所谓的“资源获取即初始化”(RAII)模式的完美体现：将资源的生命周期绑定到
 *     一个对象的生命周期上。当 `res_ptr` 这个栈上对象离开作用域时，它会自动调用
 *     `freeaddrinfo`，无需程序员手动管理。
 * `getaddrinfo` dynamically allocates a linked list of `addrinfo` structs in the `res` pointer. This memory
 * must be manually freed by calling `freeaddrinfo`, otherwise it will cause a memory leak. To ensure that
 * the resource is correctly released even if an exception occurs or the function returns early, `std::shared_ptr`
 * is used here with a custom deleter, `FreeAddrInfo`.
 * - `shared_ptr<addrinfo> res_ptr(res, FreeAddrInfo())`:
 *   - `res` is the raw pointer obtained from `getaddrinfo`.
 *   - `FreeAddrInfo()` is a function object (or a struct with `operator()`) that is automatically called
 *     to free the memory pointed to by `res` when the `shared_ptr`'s reference count drops to zero.
 *   - This is a perfect example of the "Resource Acquisition Is Initialization" (RAII) pattern: binding the
 *     lifetime of a resource to the lifetime of an object. When the `res_ptr` object on the stack goes out
 *     of scope, it automatically calls `freeaddrinfo`, requiring no manual memory management from the programmer.
 *
 * 网络编程知识点 (Networking Knowledge Point): [getaddrinfo 的作用]
 * `getaddrinfo` 是一个功能强大的函数，它集成了 DNS 查询和端口号查找等功能。
 * - 它能处理 IPv4 和 IPv6，并且可以根据 `hints` 参数进行筛选。
 * - 它可以返回一个包含多个地址的链表，例如一个主机名可能对应多个 IP 地址（用于负载均衡或冗余）。
 *   这个实现只取用了链表中的第一个结果 (`res->ai_addr`)，这在许多简单客户端中是常见的做法。
 * - 它返回的 `sockaddr` 结构是通用的，需要根据 `ai_family` (e.g., `AF_INET` for IPv4)
 *   来正确地转换和使用。
 * `getaddrinfo` is a powerful function that integrates functionalities like DNS lookup and service name-to-port
 * number translation.
 * - It can handle both IPv4 and IPv6 and can filter results based on the `hints` parameter.
 * - It can return a linked list of multiple addresses, for example, a single hostname might correspond to
 *   multiple IP addresses (for load balancing or redundancy). This implementation only uses the first result
 *   in the list (`res->ai_addr`), which is a common practice in many simple clients.
 * - The `sockaddr` structure it returns is generic and needs to be correctly cast and used based on the
 *   `ai_family` (e.g., `AF_INET` for IPv4).
 *
 * @param[in] hostname 要解析的主机名 (e.g., "127.0.0.1")
 * @param[in] service 服务名或端口号的字符串表示 (e.g., "80", "http")
 * @param[in] hints 一个 `addrinfo` 结构体，用于指导解析过程
 *****************************************************************************************/

/*****************************************************************************************
 * @brief 辅助函数：创建 addrinfo 提示结构体 (Helper function: create an addrinfo hints structure)
 * 
 * @details
 * 这是一个内联辅助函数，用于快速、安全地创建一个 `addrinfo` 结构体，该结构体用作
 * `getaddrinfo` 函数的“提示”或“过滤器”，以控制地址解析的行为。
 * This is an inline helper function for quickly and safely creating an `addrinfo` structure,
 * which is used as "hints" or a "filter" for the `getaddrinfo` function to control the
 * behavior of address resolution.
 *
 * C++ 语法点 (C++ Syntax Point):
 * - `inline`: 这是一个给编译器的建议，将此函数的代码直接嵌入到调用它的地方。对于这样简短的函数，
 *   这可以避免函数调用的开销，从而提高性能。
 *   This is a suggestion to the compiler to embed the code of this function directly at the point
 *   where it is called. For short functions like this one, it can avoid the overhead of a function call,
 *   thus improving performance.
 * - `addrinfo hints {};`: 这是 C++11 引入的值初始化（或称为聚合初始化/列表初始化）。
 *   它能确保 `hints` 结构体的所有成员都被零初始化（对于指针是 `nullptr`，对于数值是 0）。
 *   这是一个非常重要的安全实践，可以防止 `getaddrinfo` 使用未初始化的垃圾值。
 *   This is value initialization (also known as aggregate/list initialization) introduced in C++11.
 *   It ensures that all members of the `hints` struct are zero-initialized (e.g., `nullptr` for pointers,
 *   0 for numeric types). This is a crucial safety practice to prevent `getaddrinfo` from using
 *   uninitialized garbage values.
 *
 * @param[in] ai_flags 控制 `getaddrinfo` 行为的标志位 (e.g., `AI_PASSIVE`, `AI_CANONNAME`)
 * @param[in] ai_family 指定地址族 (e.g., `AF_INET` for IPv4, `AF_INET6` for IPv6, `AF_UNSPEC` for any)
 * @return addrinfo 初始化后的提示结构体 (The initialized hints structure)
 *****************************************************************************************/
