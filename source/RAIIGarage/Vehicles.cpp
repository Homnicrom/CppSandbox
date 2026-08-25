#include<RAIIGarage/Vehicles.h>
#include<print>

IVehicle::~IVehicle()
{
  std::println("{0}", "IVehicle deleted!");
}

///////////////////////

void Car::Setup(const int speed, std::string name)
{
  m_Speed = speed;
  m_Name = std::move(name);
}

void Car::Print() const
{
  std::println("{0} {1} {2}", "This is a Car!", m_Speed, m_Name);
}

void Car::DeleteVehicle()
{
  std::println("{0}", "This Car is deleted!");
  delete this;
}

///////////////////////

void Van::Setup(const int speed, const int cargoMax)
{
  m_Speed = speed;
  m_CargoMax = cargoMax;
}

void Van::Print() const
{
  std::println("{0} {1} {2}", "This is a Van!", m_Speed, m_CargoMax);
}

void Van::DeleteVehicle()
{
  std::println("{0}", "This Van is deleted!");
  delete this;
}

///////////////////////

void Forklift::Setup(const int speed, const int cargoMax, const float balance)
{
  m_Speed = speed;
  m_CargoMax = cargoMax;
  m_Balance = balance;
}

void Forklift::Print() const
{
  std::println("{0} {1} {2} {3}", "This is a Forklift!", m_Speed, m_CargoMax, m_Balance);
}

void Forklift::DeleteVehicle()
{
  std::println("{0}", "This Forklift is deleted!");
  delete this;
}

///////////////////////

void Truck::Print() const
{
  std::println("{0} {1} {2} {3}", "This is a Truck!", m_Speed, m_TrailerCount, m_MaxLoad);
}

void Truck::Setup(const int speed, const int trailerCount, const float maxLoad)
{
  m_Speed = speed;
  m_TrailerCount = trailerCount;
  m_MaxLoad = maxLoad;
}

void Truck::DeleteVehicle()
{
  std::println("{0}", "This Truck is deleted!");
  delete this;
}