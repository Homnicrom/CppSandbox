#pragma once
#include <string>
#include <string_view>

// Only for cases whose subject is the printing itself. Lifetime is asserted with counters instead.
namespace TestSupport
{
  inline bool Contains(const std::string& haystack, const std::string_view needle)
  {
    return haystack.find(needle) != std::string::npos;
  }
}
