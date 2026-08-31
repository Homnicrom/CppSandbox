#pragma once
#include<string>
#include<memory>
#include<concepts>
#include<RAIIGarage/ScopedTimer.h>
#include<RAIIGarage/UniquePtr.h>

class IVehicle
{
public:
  class Helper
  {
  public:
    template<typename T, class... Args>
    requires std::derived_from<T, IVehicle>
    static std::unique_ptr <T, IVehicle::Helper> CreateVehicle(Args&&... args)
    {
      std::unique_ptr<T, IVehicle::Helper> vehicle{new T, IVehicle::Helper{}};
      vehicle->Setup(std::forward<Args>(args)...);
      return vehicle;
    }

    template<typename T, class... Args>
    requires std::derived_from<T, IVehicle>
    static UniquePtr<T, IVehicle::Helper> CreateVehicleCustom(Args&&... args)
    {
      UniquePtr<T, IVehicle::Helper> vehicle{ new T, IVehicle::Helper{} };
      vehicle->Setup(std::forward<Args>(args)...);
      return vehicle;
    }

    template<typename T, class... Args>
    requires std::derived_from<T, IVehicle>
    static std::shared_ptr <T> CreateVehicleShared(Args&&... args)
    {
      std::shared_ptr<T> vehicle{new T, IVehicle::Helper{}};
      vehicle->Setup(std::forward<Args>(args)...);
      return vehicle;
    }

    void operator()(IVehicle* vehicle) const
    {
      if (vehicle)
      {
        vehicle->DeleteVehicle();
      }
    }

  private:
    Helper() = default;
  };

  virtual void Print() const = 0;

  int GetSpeed() const { return m_Speed; }

protected:
  int m_Speed{};

  IVehicle() = default;
  IVehicle(const IVehicle&) = delete;
  IVehicle& operator=(const IVehicle&) = delete;

  virtual ~IVehicle();

private:
  ScopedTimer vehicleScopedTimer{};

  virtual void DeleteVehicle() = 0;
};

class Car : public IVehicle
{
public:
  void Print() const override;

  const std::string& GetName() const { return m_Name; }

private:
  std::string m_Name{};

  void Setup(const int speed, std::string name);
  void DeleteVehicle() override;

  Car() = default;
  ~Car() override = default;
  friend IVehicle;
};

class Van : public IVehicle
{
public:
  void Print() const override;

  int GetCargoMax() const { return m_CargoMax; }

private:
  int m_CargoMax{};

  void Setup(const int speed, const int cargoMax);
  void DeleteVehicle() override;

  Van() = default;
  ~Van() override = default;
  friend IVehicle;
};

class Forklift : public IVehicle
{
public:
  void Print() const override;

  int GetCargoMax() const { return m_CargoMax; }
  float GetBalance() const { return m_Balance; }

private:
  int m_CargoMax{};
  float m_Balance{};

  void Setup(const int speed, const int cargoMax, const float balance);
  void DeleteVehicle() override;

  Forklift() = default;
  ~Forklift() override = default;
  friend IVehicle;
};

class Truck : public IVehicle
{
public:
  void Print() const override;

  int GetTrailerCount() const { return m_TrailerCount; }
  float GetMaxLoad() const { return m_MaxLoad; }

private:
  int m_TrailerCount{};
  float m_MaxLoad{};

  void Setup(const int speed, const int trailerCount, const float maxLoad);
  void DeleteVehicle() override;

  Truck() = default;
  ~Truck() override = default;
  friend IVehicle;
};
