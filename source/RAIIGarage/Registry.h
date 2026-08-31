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
  //Access
  static Registry& GetInstance();

  std::size_t EntryCount() const;

  void RegisterVehicle(const std::shared_ptr<IVehicle>& vehicle);
  void RemoveVehicle(const std::shared_ptr<IVehicle>& vehicle);

 //Mainly for tests
  void Reset();

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

    for (const VehicleEntry& entry : m_Vehicles)
    {
      const auto vehicleLock{ entry.vehicle.lock() };
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

  template<typename T = void>
  std::size_t CountLiveVehicles() const
  {
    constexpr bool isVoid{ std::is_void_v<T> };
    std::size_t count{ 0 };

    for (const VehicleEntry& entry : m_Vehicles)
    {
      const auto vehicleLock{ entry.vehicle.lock() };
      if (vehicleLock)
      {
        if constexpr (isVoid)
        {
          ++count;
        }
        else if (typeid(*vehicleLock) == typeid(T))
        {
          ++count;
        }
      }
    }

    return count;
  }

private:
  struct VehicleEntry
  {
    std::weak_ptr<IVehicle> vehicle;
    int refCount{ 1 };
  };

  std::vector<VehicleEntry> m_Vehicles;

  Registry() = default;
  ~Registry() = default;

  Registry(const Registry&) = delete;
  Registry& operator=(const Registry&) = delete;

  static bool IsSameVehicle(const std::weak_ptr<IVehicle>& entry, const std::shared_ptr<IVehicle>& vehicle);
};