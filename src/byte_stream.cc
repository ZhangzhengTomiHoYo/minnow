#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  (void)data; // Your code here.
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

/*
为什么需要加 const？
1.保证代码正确性：
编译器会强制检查 const 成员函数 —— 如果函数内部试图修改非 mutable 的成员变量（比如 closed_ = true），或者调用非 const 的成员函数（比如 close()），编译器会直接报错，防止意外修改对象状态。
2.支持 const 对象调用：
如果有一个 const Writer& 类型的常量引用（比如在某些函数参数中），常量对象只能调用 const 成员函数。如果 is_closed() 不加 const，常量对象就无法调用它，会导致编译错误。
3.明确函数意图：
从代码可读性角度，const 告诉其他开发者：这个函数是 “只读” 的，调用它不会改变对象的状态，减少理解成本。

然后上面提到了mutable
在 C++ 中，mutable 是一个关键字，用于修饰类的非静态成员变量。它的核心作用是：允许在 const 成员函数中修改被 mutable 修饰的成员变量，打破了 “const 成员函数不能修改成员变量” 的默认限制。
c++98就有了
*/
bool Writer::is_closed() const
{
  return {}; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return {}; // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return {}; // Your code here.
}

string_view Reader::peek() const
{
  return {}; // Your code here.
}

void Reader::pop( uint64_t len )
{
  (void)len; // Your code here.
}

bool Reader::is_finished() const
{
  return {}; // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return {}; // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return {}; // Your code here.
}

