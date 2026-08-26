#pragma once
#include<memory>

class IVehicle;

class Dealer
{
public:
  Dealer(std::shared_ptr<IVehicle> vehicle);

  Dealer(const Dealer& other);
  Dealer& operator=(const Dealer& other);

  ~Dealer();

private:
  std::shared_ptr<IVehicle> m_Vehicle{};
};