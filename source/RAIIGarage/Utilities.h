#pragma once
#include <concepts>
#include <type_traits>
#include <tuple>

namespace Utilities
{
  template<class... Args>
  concept ComparableNumeric = (std::is_arithmetic_v<std::remove_cvref_t<Args>> && ...);

  template<class... Args>
  requires (sizeof...(Args) > 0) && ComparableNumeric<Args...>
  constexpr std::common_type_t<Args...> Max(Args... args)
  {
    using Common = std::common_type_t<Args...>;
    Common max{ static_cast<Common>(std::get<0>(std::tuple{args...})) };
    ((max = args > max ? args : max), ...);

    return max;
  }
}
