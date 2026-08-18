#include<RAIIGarage/Registry.h>

void Registry::RegisterVehicle(const std::shared_ptr<IVehicle>& vehicle)
{
  m_Vehicles.push_back(vehicle);
}

void Registry::RemoveVehicle(const std::shared_ptr<IVehicle>& vehicle)
{
  std::erase_if(m_Vehicles, [&vehicle](const std::weak_ptr<IVehicle>& vehicleIt) {
    auto vehicleLock{ vehicleIt.lock() };
    return !vehicleLock || vehicle.get() == vehicleLock.get();
    });
}

Registry& Registry::GetInstance()
{
  static Registry instance{};
  return instance;
}