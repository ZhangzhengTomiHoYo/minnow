#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  // 辅助函数（已提供），用于访问ByteStream的Reader和Writer接口
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; };       // Signal that the stream suffered an error. // 表示流发生了错误
  bool has_error() const { return error_; }; // Has the stream had an error?              // 流是否发生了错误？

protected:
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  // 请将任何额外的状态添加到ByteStream中，而不是Writer和Reader接口中
  uint64_t capacity_;
  /*
  <1>. c++开发者常识1
  uint64_t 是一个固定宽度整型：

  u = unsigned (无符号)
  int = integer (整数)
  64 = 64位 (8字节)
  _t = type (类型后缀，表示这是一个类型定义)

  _t 后缀的含义
  _t 后缀是 C/C++ 中的命名约定，表示这是一个类型定义 (type definition)。这些类型通常通过 typedef 定义：
  
  <2>. c++开发者常识2
  capacity_后面带_   提高代码可读性
  1.一眼就能看出 capacity_ 是类的成员变量
  2.不需要使用 this->capacity 来明确指定
  */

  bool error_ {};
  /*
  在 C++ 中，{} 可用于对变量进行零初始化或者默认初始化：
  bool error_ {};  // 初始化为false（布尔类型的默认值）
  int x {};         // 初始化为0（整数类型）
  double y {};      // 初始化为0.0（浮点数类型）
  std::string s {}; // 初始化为空字符串（调用默认构造函数）
  int* ptr {};      // 初始化为nullptr（指针类型）
  */
};

class Writer : public ByteStream
{
public:
  void push( std::string data );       // Push data to stream, but only as much as available capacity allows.          // 将数据推送到流中，但仅限于可用容量允许的范围
  void close();                        // Signal that the stream has reached its ending. Nothing more will be written. // 表示流已到达结束位置，不会再写入任何内容

  bool is_closed() const;              // Has the stream been closed?                                   // 流是否已关闭？
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?         // 当前可以推送到流中的字节数是多少？
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream       // 累计推送到流中的字节总数
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const;   // Peek at the next bytes in the buffer                          // 查看缓冲区中的下一个字节
  void pop( uint64_t len );        // Remove `len` bytes from the buffer                            // 从缓冲区中移除`len`字节

  bool is_finished() const;        // Is the stream finished (closed and fully popped)?           // 流是否已完成（已关闭且完全弹出）？
  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)  // 当前缓冲的字节数（已推送但未弹出）
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream       // 从流中累计弹出的字节总数
};

/*
 * read: A (provided) helper function thats peeks and pops up to `max_len` bytes
 * from a ByteStream Reader into a string;
 * read: 一个（已提供的）辅助函数，用于从ByteStream Reader中窥视并弹出最多`max_len`字节到字符串中
 */
void read( Reader& reader, uint64_t max_len, std::string& out );
