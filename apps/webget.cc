#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

/*
 * 🌐 计算机网络知识体系1：HTTP客户端实现原理
 * 
 * HTTP (HyperText Transfer Protocol) 是现代互联网的基础协议
 * 
 * 基本工作流程：
 * 1. 域名解析：将 stanford.edu 解析为 IP 地址
 * 2. 建立连接：创建 TCP 套接字连接到服务器的80端口
 * 3. 发送请求：发送 HTTP GET 请求
 * 4. 接收响应：读取服务器的响应数据
 * 5. 关闭连接：清理资源
 * 
 * HTTP 请求格式：
 * GET /class/cs144 HTTP/1.1
 * Host: stanford.edu
 * Connection: close
 * 
 * 这个函数就是要实现一个简单的HTTP客户端
 */

void get_URL( const string& host, const string& path )
{
  /*
   * 🚫 错误分析：TCPSocket构造函数使用错误
   * 
   * 编译错误原因：
   * TCPSocket socket(addr);  // 错误！
   * 
   * 查看 socket.hh 中的 TCPSocket 构造函数：
   * 1. TCPSocket() - 默认构造函数，创建未连接的套接字
   * 2. TCPSocket(FileDescriptor&& fd) - 从文件描述符创建
   * 
   * 没有 TCPSocket(Address&) 这样的构造函数！
   * 
   * 🔧 正确的网络编程步骤：
   * 1. 创建套接字对象
   * 2. 调用 connect() 方法连接到服务器
   * 3. 发送和接收数据
   * 4. 关闭连接（RAII自动处理）
   */
  
  /*
   * 🚫 错误分析：Address构造函数参数错误
   * 
   * 你的代码：Address addr(host, path);
   * 错误信息：getaddrinfo(cs144.keithw.org, /nph-hasher/xyzzy): Servname not supported for ai_socktype
   * 
   * 问题分析：
   * 1. Address构造函数期望的是：Address(主机名, 端口号)
   * 2. 你传入的是：Address(host, path) = Address("cs144.keithw.org", "/nph-hasher/xyzzy")
   * 3. 系统试图将"/nph-hasher/xyzzy"解析为端口号，但这是一个路径！
   * 
   * 🔧 正确的理解：
   * - host: 主机名（如 "cs144.keithw.org"）
   * - path: URL路径（如 "/nph-hasher/xyzzy"）
   * - 端口号: HTTP默认是"80"，HTTPS是"443"
   * 
   * 网络编程基础：
   * - TCP连接需要：IP地址 + 端口号
   * - HTTP协议运行在TCP之上
   * - 路径是HTTP请求的一部分，不是TCP连接的一部分
   */
  
  /*
   * 🌐 计算机网络知识体系2：TCP vs HTTP 层次区别
   * 
   * TCP层（传输层）：
   * - 负责建立可靠的字节流连接
   * - 需要：IP地址 + 端口号
   * - 例如：连接到 cs144.keithw.org:80
   * 
   * HTTP层（应用层）：
   * - 在TCP连接之上传输HTTP消息
   * - 包含：请求方法 + 路径 + 请求头
   * - 例如：GET /nph-hasher/xyzzy HTTP/1.1
   * 
   * 分层原理：
   * 1. 先建立TCP连接（IP + 端口）
   * 2. 再发送HTTP请求（路径在请求中）
   */


  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // cerr << "Warning: get_URL() has not been implemented yet.\n";

  // 1. 首先根据 host 初始化一个 Address 对象
  // Address 有一个构造函数为 Address( const std::string& hostname, const std::string& service );
  // host 就是 参数的那个host
  // service 
  Address addr(host, "80");
  cout<< "Address created: " << addr.ip() << endl;

  // 2. 现在已经有了ip 和 方法 需要去连接 服务端
  // 方法签名   void connect( const Address& address );
  TCPSocket socket;
  socket.connect(addr);

  // 3. 接下来肯定是要用上path 那基本上还是说 需要去看socket.hh吧
  // 看了socket类和TCPSocket类都没有 什么写或者发送东西的方法
  // 应该是最底层的FileDescriptor类有

  /*
   * 🚫 错误分析：字符串拼接语法错误
   *
   * 你的代码：
   * const string request = "GET" + " " + "/" + host + "HTTP/1.1" + "/r/n";
   *
   * 编译错误原因：
   * 在C++中，用双引号括起来的 "GET" 和 " " 被视为C风格的字符串字面量 (const char*)，而不是 std::string 对象。
   * C++ 不支持直接用 `+` 号拼接两个C风格的字符串。编译器不知道如何处理 "GET" + " "，因此会报错。
   *
   * 🔧 正确的拼接方式：
   * 必须确保 `+` 号的左边或右边至少有一个是 std::string 对象。这样C++就会使用 std::string 的拼接功能。
   *
   * 解决方法：
   * 将第一个C风格字符串显式转换成 std::string 对象。
   * string("GET") + " " + ...
   *
   *
   * 🌐 计算机网络知识体系3：HTTP请求格式详解
   *
   * 一个标准的HTTP GET请求必须包含以下部分：
   * 1. 请求行 (Request Line):  方法 + 路径 + 协议版本。例如：GET /path/to/file HTTP/1.1
   * 2. 请求头 (Headers):      至少需要 Host 头。例如：Host: www.example.com
   * 3. 分隔空行 (Empty Line):  一个回车换行符 `\r\n`，用于分隔请求头和请求体。
   *
   * 你的代码中的逻辑错误：
   * 1. 路径错误：请求行中应该是 `path` 变量，而不是 `host` 变量。
   * 2. 缺少 Host 头：HTTP/1.1 协议强制要求必须有 Host 请求头。
   * 3. 缺少 Connection 头：最好明确告诉服务器请求完成后关闭连接。
   * 4. 换行符错误：HTTP协议的换行符是 `\r\n` (回车+换行)，而不是 `/r/n`。
   */

  // ✅ 正确的HTTP请求构建
  const string request = "GET " + path + " HTTP/1.1" + "\r\n" +
                         "Host: " + host + "\r\n" +
                         "Connection: close" + "\r\n" +
                         "\r\n";

  socket.write(request);
  // 循环读取并打印服务器的响应
  string buffer;
  while ( !socket.eof() ) {
    socket.read( buffer );
    cout << buffer;
    buffer.clear();
  }
}

int main( int argc, char* argv[] )
{
  /*
   * 🖥️ C++知识体系：命令行参数处理
   * 
   * argc: argument count - 命令行参数个数
   * argv: argument vector - 命令行参数数组
   * 
   * 例如：./webget stanford.edu /class/cs144
   * argc = 3
   * argv[0] = "./webget"     (程序名)
   * argv[1] = "stanford.edu" (主机名)
   * argv[2] = "/class/cs144" (路径)
   * 
   * span<char*, argc> 是C++20的现代方式：
   * - 提供安全的数组访问
   * - 避免越界访问
   * - 比传统的 argv[i] 更安全
   */
  
  cout<<"argc is "<<argc<<endl;
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cout<<"args 0 是: "<<args[0]<<endl;
      cout<<"args 1 是: "<<args[1]<<endl;
      cout<<"args 2 是: "<<args[2]<<endl;
      cout<<"args 3 是: "<<args[3]<<endl;
      cout<< "zz coming!"<<endl;
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    /*
     * 🛡️ C++知识体系：异常处理
     * 
     * 网络编程中的异常处理至关重要：
     * - 网络连接可能失败
     * - 域名解析可能失败
     * - 服务器可能不响应
     * - 数据传输可能中断
     * 
     * 捕获 const exception& e 的好处：
     * - 捕获所有标准异常
     * - const 引用避免不必要的拷贝
     * - 引用避免对象切片问题
     */
    cerr << "Error occurred: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
 * 🎯 总结：这个webget程序的完整实现
 * 
 * 核心功能：
 * 1. 解析命令行参数（主机名和路径）
 * 2. 建立TCP连接到Web服务器
 * 3. 发送HTTP GET请求
 * 4. 接收并显示响应
 * 
 * 涉及的知识点：
 * - C++现代语法（span, string_view等）
 * - 网络编程基础（TCP套接字）
 * - HTTP协议基础
 * - 异常处理
 * - RAII资源管理
 * 
 * 这是一个完整的网络客户端实现，
 * 展示了如何用C++进行实际的网络编程。
 */
