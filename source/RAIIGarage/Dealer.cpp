#include <RAIIGarage/Dealer.h>
#include <RAIIGarage/Registry.h>

Dealer::Dealer(const std::shared_ptr<IVehicle>& vehicle) : m_Vehicle { vehicle }
{
  Registry::GetInstance().RegisterVehicle(vehicle);
}

Dealer::~Dealer()
{
  Registry::GetInstance().RemoveVehicle(m_Vehicle);
}
