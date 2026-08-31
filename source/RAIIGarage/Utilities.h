#pragma once
#include <concepts>
#include <type_traits>
#include <tuple>

namespace Utilities
{
  template<class T, class... Args>
  concept AnyOf = (std::same_as<T, Args> || ...); //Just to avoid repeating std::same_as<std::remove_cvref_t<T>, U> in next concept. Common patern.

  template<class T>
  concept Numeric = std::is_arithmetic_v<std::remove_cvref_t<T>> && !AnyOf<std::remove_cvref_t<T>, bool, char, wchar_t, char8_t, char16_t, char32_t>; //Exclude bool and chars from comparison

  template<class... Args>
  concept ConsistentSignedness = std::is_signed_v<std::common_type_t<Args...>> || !(std::is_signed_v<std::remove_cvref_t<Args>> || ...); //Avoid diferent signed types mixing

  template<class... Args>
  concept ComparableNumeric = sizeof...(Args) > 0 && (Numeric<Args> && ...) && ConsistentSignedness<Args...>;

  template<class... Args>
  requires ComparableNumeric<Args...>
  constexpr auto Max(Args... args)
  {
    using Common = std::common_type_t<Args...>;
    Common max{ static_cast<Common>(std::get<0>(std::tuple{args...})) };
    ((max = args > max ? args : max), ...);

    return max;
  }
}
