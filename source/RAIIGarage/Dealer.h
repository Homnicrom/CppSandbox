#pragma once
#include<memory>

class IVehicle;

class Dealer
{
public:
  Dealer(const std::shared_ptr<IVehicle>& vehicle);

  ~Dealer();

private:
  std::shared_ptr<IVehicle> m_Vehicle{};
};