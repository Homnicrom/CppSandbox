#pragma once
#include<vector>
#include<print>
#include<memory>
#include<type_traits>
#include<typeinfo>
#include<RAIIGarage/Vehicles.h>

class Registry
{
public:
  void RegisterVehicle(const std::shared_ptr<IVehicle>& vehicle);
  void RemoveVehicle(const std::shared_ptr<IVehicle>& vehicle);

  //Access
  static Registry& GetInstance();

  template<typename T = void>
  void CallVehicles() const
  {
    constexpr bool isVoid{ std::is_void_v<T> };
    if constexpr (isVoid)
    {
      std::println("Calling Vehicles:");
    }
    else
    {
      std::println("Calling Vehicles of type {0}:", typeid(T).name());
    }

    for (const std::weak_ptr<IVehicle>& vehicleIt : m_Vehicles)
    {
      const auto vehicleLock{ vehicleIt.lock() };
      if (vehicleLock)
      {
        if constexpr (isVoid)
        {
          vehicleLock->Print();
        }
        else if (typeid(*vehicleLock) == typeid(T))
        {
          vehicleLock->Print();
        }
      }
    }
  }

private:
  std::vector<std::weak_ptr<IVehicle>> m_Vehicles;

  Registry() = default;
  ~Registry() = default;

  Registry(const Registry&) = delete;
  Registry& operator=(const Registry&) = delete;
};