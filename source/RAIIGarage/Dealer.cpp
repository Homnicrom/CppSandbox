#include <RAIIGarage/Dealer.h>
#include <RAIIGarage/Registry.h>

Dealer::Dealer(std::shared_ptr<IVehicle> vehicle) : m_Vehicle{ std::move(vehicle) } //sink parameter
{
  Registry::GetInstance().RegisterVehicle(m_Vehicle);
}

Dealer::Dealer(const Dealer& other) : m_Vehicle{ other.m_Vehicle }
{
  Registry::GetInstance().RegisterVehicle(m_Vehicle);
}

Dealer& Dealer::operator=(const Dealer& other)
{
  if (this != &other)
  {
    if (m_Vehicle)
    {
      Registry::GetInstance().RemoveVehicle(m_Vehicle);
    }
    m_Vehicle = other.m_Vehicle;
    Registry::GetInstance().RegisterVehicle(m_Vehicle);
  }
  return *this;
}

Dealer::~Dealer()
{
  Registry::GetInstance().RemoveVehicle(m_Vehicle);
}
