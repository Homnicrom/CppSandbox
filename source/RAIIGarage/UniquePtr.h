#pragma once
#include<concepts>
#include<print>

template<class T>
class DefaultDelete 
{
public:
  void operator()(T* ptr) const noexcept
  {
    delete ptr;
  }
};

template<class T, class Dx = DefaultDelete<T>>
requires std::invocable<Dx&, T*>
class UniquePtr
{
public:
  UniquePtr() = default;

  explicit UniquePtr(T* ptr) noexcept : m_Ptr(ptr) 
  {}

  explicit UniquePtr(T* ptr, Dx deleter) noexcept : m_Ptr{ ptr }, m_Deleter{deleter}
  {}

  //Converting move constructor. Allows for coverting derived classes to base
  template<class U, class E>
  requires std::convertible_to<U*, T*> && std::constructible_from<Dx, E&&>
  UniquePtr(UniquePtr<U, E>&& uniquePtr) noexcept : m_Ptr(uniquePtr.Release()), m_Deleter(std::forward<E>(uniquePtr.GetDeleter()))
  {}

  UniquePtr(UniquePtr&& uniquePtr) noexcept : m_Ptr{ uniquePtr.m_Ptr }, m_Deleter{std::move(uniquePtr.m_Deleter)}
  {
    uniquePtr.m_Ptr = nullptr;
  }

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  ~UniquePtr()
  {
    if (m_Ptr)
    {
      m_Deleter(m_Ptr);
    }
  }

  UniquePtr& operator=(UniquePtr&& uniquePtr) noexcept
  {
    if (this != &uniquePtr)
    {
      if (m_Ptr)
      {
        m_Deleter(m_Ptr);
      }
      m_Ptr = uniquePtr.m_Ptr;
      m_Deleter = std::move(uniquePtr.m_Deleter);
      uniquePtr.m_Ptr = nullptr;
    }

    return *this;
  }

  T& operator*() const noexcept
  {
    return *m_Ptr;
  }

  T* operator->() const noexcept
  {
    return m_Ptr;
  }

  void Reset(T* ptr = nullptr) noexcept
  {
    if (m_Ptr)
    {
      m_Deleter(m_Ptr);
    }

    m_Ptr = ptr;
  }

  T* Release() noexcept
  {
    T* ptr = m_Ptr;
    m_Ptr = nullptr;
    return ptr;
  }

  Dx& GetDeleter()
  {
    return m_Deleter;
  }

private:
  T* m_Ptr{};
  Dx m_Deleter{};
};