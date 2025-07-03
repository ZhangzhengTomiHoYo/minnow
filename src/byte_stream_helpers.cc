#include "byte_stream.hh"

#include <cstdint>
#include <stdexcept>

using namespace std;

/*
 * read: A helper function thats peeks and pops up to `max_len` bytes
 * from a ByteStream Reader into a string;
 */

/*
在计算机编程语境中，peek 常见的意思是 “窥视；查看但不取出”，
在这段代码注释里也是这个意思，即查看 ByteStream Reader 里的数据，但不将其从流中移除。
而pop，查看并出栈。

总结而言，对应 栈 中的 peek 和 pop 方法。
*/
 
// read 是一个辅助函数，从ByteStream Reader中窥视最长max_len的字节
//   并将这些数据 弹出/pops up 到一个字符串中

void read( Reader& reader, uint64_t max_len, string& out )
{
  // 清空一个字符串
  // 具体原理：https://blog.csdn.net/qq_43684922/article/details/90270115
  out.clear();

  while ( reader.bytes_buffered() and out.size() < max_len ) {
    auto view = reader.peek();

    if ( view.empty() ) {
      throw runtime_error( "Reader::peek() returned empty string_view" );
    }

    view = view.substr( 0, max_len - out.size() ); // Don't return more bytes than desired.
    out += view;
    reader.pop( view.size() );
  }
}

Reader& ByteStream::reader()
{
  static_assert( sizeof( Reader ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Reader." );

  return static_cast<Reader&>( *this ); // NOLINT(*-downcast)
}

const Reader& ByteStream::reader() const
{
  static_assert( sizeof( Reader ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Reader." );

  return static_cast<const Reader&>( *this ); // NOLINT(*-downcast)
}

Writer& ByteStream::writer()
{
  static_assert( sizeof( Writer ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Writer." );

  return static_cast<Writer&>( *this ); // NOLINT(*-downcast)
}

const Writer& ByteStream::writer() const
{
  static_assert( sizeof( Writer ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Writer." );

  return static_cast<const Writer&>( *this ); // NOLINT(*-downcast)
}
