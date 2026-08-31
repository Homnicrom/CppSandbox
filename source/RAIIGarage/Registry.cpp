#include<RAIIGarage/Registry.h>
#include<cassert>

Registry& Registry::GetInstance()
{
  //Safe thread initialization, automatically destroyed at program exit. Avoids initialization order problems
  static Registry instance{};
  return instance;
}

std::size_t Registry::EntryCount() const
{
  return m_Vehicles.size();
}

bool Registry::IsSameVehicle(const std::weak_ptr<IVehicle>& entry, const std::shared_ptr<IVehicle>& vehicle)
{
  return !entry.owner_before(vehicle) && !vehicle.owner_before(entry); //Comparison without locking
}

void Registry::RegisterVehicle(const std::shared_ptr<IVehicle>& vehicle)
{
  if (!vehicle)
  {
    return;
  }

  for (VehicleEntry& entry : m_Vehicles)
  {
    if (IsSameVehicle(entry.vehicle, vehicle))
    {
      ++entry.refCount;
      return;
    }
  }

  m_Vehicles.push_back({ vehicle, 1 });
}

void Registry::RemoveVehicle(const std::shared_ptr<IVehicle>& vehicle)
{
  if (!vehicle)
  {
    return;
  }

  bool vehicleFound{ false };
  std::erase_if(m_Vehicles, [&](VehicleEntry& entry) {
    if (!vehicleFound && IsSameVehicle(entry.vehicle, vehicle))
    {
      vehicleFound = true;
      assert(entry.refCount > 0 && "Tried to remove count in entry where count is 0");
      --entry.refCount;
    }
    return entry.refCount <= 0 || entry.vehicle.expired();
    });
}

void Registry::Reset()
{
  m_Vehicles.clear();
}