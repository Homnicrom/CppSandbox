#include<RAIIGarage/Registry.h>

void Registry::RegisterVehicle(const std::shared_ptr<IVehicle>& vehicle)
{
  for (Entry& entry : m_Vehicles)
  {
    if (entry.vehicle.lock().get() == vehicle.get())
    {
      ++entry.refCount;
      return;
    }
  }

  m_Vehicles.push_back({ vehicle, 1 });
}

void Registry::RemoveVehicle(const std::shared_ptr<IVehicle>& vehicle)
{
  for (Entry& entry : m_Vehicles)
  {
    if (entry.vehicle.lock().get() == vehicle.get())
    {
      --entry.refCount;
      break;
    }
  }

  std::erase_if(m_Vehicles, [](const Entry& entry) {
    return entry.refCount <= 0 || entry.vehicle.expired();
    });
}

Registry& Registry::GetInstance()
{
  static Registry instance{};
  return instance;
}