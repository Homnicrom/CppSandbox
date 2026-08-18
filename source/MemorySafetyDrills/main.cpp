// Drill 1: Dangling reference
//
// Windows: the normal build already gives you ASan, no extra toolchain needed.
// cl.exe has had native /fsanitize=address support since VS 2019 16.9.
//   cmake -B build -G "Visual Studio 17 2022" -A x64
//   cmake --build build --config Debug --target MemorySafetyDrills
//   build\bin\MemorySafetyDrills\Debug\MemorySafetyDrills.exe
// (MemorySafetyDrills/CMakeLists.txt adds /fsanitize=address for MSVC and copies the
// matching clang_rt.asan*dynamic-x86_64.dll next to the .exe automatically.)
//
// Linux/macOS: g++ -std=c++23 -fsanitize=address -g main.cpp -o d1 && ./d1
//
// Run under ASan.

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
