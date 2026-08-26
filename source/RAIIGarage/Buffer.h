#pragma once
#include<print>

class Buffer
{
public:
  Buffer() : m_Num{ new int{} }
  {
    std::println("Buffer constructor! {0}", *m_Num);
  }

  explicit Buffer(const int num) : m_Num{new int{num}}
  {
    std::println("Buffer int argument constructor! {0}", *m_Num);
  }

  Buffer(const Buffer& buffer) : m_Num{ buffer.m_Num ? new int{ *buffer.m_Num } : nullptr }
  {
    m_Num != nullptr ? std::println("Buffer COPY constructor! {0}", *m_Num) : std::println("Buffer COPY constructor! Nullptr");
  }

  Buffer(Buffer&& buffer) noexcept : m_Num{buffer.m_Num}
  {
    buffer.m_Num = nullptr;
    m_Num != nullptr ? std::println("Buffer MOVE constructor! {0}", *m_Num) : std::println("Buffer MOVE constructor! Nullptr");
  }

  Buffer& operator=(const Buffer& buffer)
  {
    if (this != &buffer)
    {
      int* temp = buffer.m_Num ? new int(*buffer.m_Num) : nullptr; //For possible allocation failure, good pratice. Move assignment does not allocate
      delete m_Num;
      m_Num = temp;
    }
    m_Num != nullptr ? std::println("Buffer assignment! {0}", *m_Num) : std::println("Buffer assignment! Nullptr");

    return *this;
  }

  Buffer& operator=(Buffer&& buffer) noexcept
  {
    if (this != &buffer)
    {
      delete m_Num;
      m_Num = buffer.m_Num;
      buffer.m_Num = nullptr;
    }
    m_Num != nullptr ? std::println("Buffer MOVE assignment!{0}", *m_Num) : std::println("Buffer MOVE assignment! Nullptr");

    return *this;
  }

  ~Buffer()
  {
    m_Num != nullptr ? std::println("Buffer DELETE!{0}", *m_Num) : std::println("Buffer DELETE! Nullptr");
    delete m_Num;
  }

private:
  int* m_Num{};
};