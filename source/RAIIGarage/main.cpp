#include <iostream>
#include <RAIIGarage/Buffer.h>
#include <RAIIGarage/Dealer.h>
#include <RAIIGarage/Registry.h>
#include <RAIIGarage/ScopedTimer.h>
#include <RAIIGarage/Utilities.h>
#include <RAIIGarage/Vehicles.h>

int main()
{
  ScopedTimer scopedTimer{};

  std::println("{}", "***MAX***");
  constexpr auto maxResult = Utilities::Max(-120, -135, -2345.5f);
  std::println("Max Result {}", maxResult);
  
  std::println("\n{}", "***IVehicle***");
  {
    Dealer d1{ IVehicle::Helper::CreateVehicleShared<Car>(10, "My Car") };
  }
  Dealer d2{ IVehicle::Helper::CreateVehicleShared<Van>(10, 100) };
  Dealer d3{ IVehicle::Helper::CreateVehicleShared<Forklift>(10, 1000, 7.8f) };
  Dealer d4{ IVehicle::Helper::CreateVehicleShared<Truck>(5, 1, 100.5f) };

  Registry::GetInstance().CallVehicles();
  Registry::GetInstance().CallVehicles<Van>();

  UniquePtr<IVehicle, IVehicle::Helper> vehicleCustomPtr{ IVehicle::Helper::CreateVehicleCustom<Car>(11, "My Custom Car") };

  std::println("\n{}", "***Buffer***");
  Buffer buffer{};
  Buffer buffer1{5};
  Buffer buffer2{ buffer1 };
  Buffer buffer3{ Buffer{45} }; //elided
  Buffer buffer4{88};
  buffer4 = buffer3;
  Buffer buffer5 = Buffer{ 345 }; //elided
  Buffer buffer6{ std::move(buffer1) };

  std::println("\n{}", "***End of Execution***");
}