#pragma once
#include <chrono>
#include <print>

class ScopedTimer
{
public:
  ScopedTimer() : m_Begin{ std::chrono::steady_clock::now() }, m_End{}
  {}

  ~ScopedTimer()
  {
    m_End = std::chrono::steady_clock::now();
    std::println("ScopedTimer({0}) destroyed after {1} milliseconds", (void*)this, std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_Begin).count());
  }

  std::chrono::steady_clock::time_point m_Begin{};
  std::chrono::steady_clock::time_point m_End{};

};