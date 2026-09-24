// Dangling reference
//
// ASan is on by default for this target on both platforms (MSVC needs VS 2019 16.9+):
//   cmake --preset <windows|linux>
//   cmake --build --preset <windows|linux>-debug --target MemorySafetyDrills
//   build/bin/MemorySafetyDrills/Debug/MemorySafetyDrills   (backslashes on Windows)
//
// Without CMake, from this folder (g++-14, not plain g++: 13 has no <print>):
//   g++-14 -std=c++23 -fsanitize=address -g main.cpp -o d1 && ./d1

#include <print>
#include <vector>
#include <string>

class Vehicle
{
public:
  Vehicle(std::string name) : m_Name{ std::move(name) } {}
  const std::string& GetName() const { return m_Name; }
private:
  std::string m_Name;
};

// Returns a reference into the vector.
const std::string& FindName(std::vector<Vehicle>& fleet, int speed)
{
  Vehicle v{ "temp-" + std::to_string(speed) };
  fleet.push_back(v);
  return fleet.back().GetName();
}

int main()
{
  std::vector<Vehicle> fleet;
  fleet.reserve(2);

  const std::string& a = FindName(fleet, 10);
  const std::string& b = FindName(fleet, 20);
  const std::string& c = FindName(fleet, 30); // triggers reallocation, dangling reference

  std::println("{} {} {}", a, b, c);
}
