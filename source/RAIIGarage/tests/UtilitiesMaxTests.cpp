#include <RAIIGarage/Utilities.h>

#include <string>
#include <type_traits>

// No TEST() cases on purpose: Max is constexpr, so its value, its return type and the argument
// packs the concept accepts are all decided by the compiler. A runtime EXPECT would be weaker.
static_assert(Utilities::Max(3, 7, 5) == 7, "Max must be usable in a constant expression");
static_assert(Utilities::Max(42) == 42, "Max must accept a single argument");
static_assert(Utilities::Max(9, 1, 4) == 9); // The fold seeds max from the first argument

// common_type_t<int, int, float> is float. Exact equality holds because Max returns one of its arguments.
static_assert(std::is_same_v<decltype(Utilities::Max(-120, -135, -2345.5f)), float>);
static_assert(Utilities::Max(-120, -135, -2345.5f) == -120.0f);

static_assert(Utilities::ComparableNumeric<int, float, double>);
static_assert(Utilities::ComparableNumeric<int>);
static_assert(!Utilities::ComparableNumeric<int, std::string>);
static_assert(!Utilities::ComparableNumeric<const char*>);

// Mixed signedness is rejected rather than answered wrongly: unsigned common types make Max(-1, 2u) 4294967295.
static_assert(!Utilities::ComparableNumeric<int, unsigned>);
static_assert(Utilities::ComparableNumeric<unsigned, unsigned long>);  // nothing signed in the pack
static_assert(Utilities::ComparableNumeric<int, float>);               // common type float is signed

// bool and the character types satisfy is_arithmetic_v, so the concept excludes them: Max(true, 'a') is 97.
static_assert(!Utilities::ComparableNumeric<bool, char>);
static_assert(!Utilities::ComparableNumeric<wchar_t>);
static_assert(Utilities::ComparableNumeric<signed char, unsigned char>); // int8_t and uint8_t

// Not asserted: that a bad call such as Max(1, "x") is rejected. On MSVC a negative
// requires-expression over a constrained template is a hard error (C2672) instead of evaluating to
// false, so that check cannot be written. Read the compiler error instead; Utilities.h deduces its
// return type so the error names ComparableNumeric rather than pointing into <type_traits>.
