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
  Address addr(host, "HTTP");
  cout<< "Address created: " << addr.ip() << endl;

  // 2. 现在已经有了ip 和 方法 需要去连接 服务端
  TCPSocket socket;
  socket.connect(addr);
  cout<<path;
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
