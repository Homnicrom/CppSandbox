#include <gtest/gtest.h>
#include <RAIIGarage/Vehicles.h>
#include <RAIIGarage/tests/CapturedOutput.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// Helper's default constructor is private, so IVehicle::Helper{}(vehicle) cannot be written at a
// call site to delete a vehicle a smart pointer already owns. Asserted with the trait rather than
// `!requires { IVehicle::Helper{}; }`: MSVC turns the access violation into a hard error C2248
// inside a requires-expression, while is_constructible does the access check properly.
static_assert(!std::is_default_constructible_v<IVehicle::Helper>);

// Vehicles are not copyable: an implicit copy assignment slices across types and overwrites the
// vehicle's ScopedTimer with another object's start time.
static_assert(!std::is_copy_constructible_v<IVehicle>);
static_assert(!std::is_copy_assignable_v<IVehicle>);
static_assert(!std::is_copy_assignable_v<Car>);
static_assert(!std::is_copy_assignable_v<ScopedTimer>);

using TestSupport::Contains;

namespace
{
  // Counts its own destruction. Helper constructs with a bare `new T` and forwards arguments only to
  // Setup, so the counter is injected there and stays a local of each test.
  // `friend IVehicle;` is what lets the nested Helper reach these private members, as the real
  // vehicles in Vehicles.h do.
  class SpyVehicle : public IVehicle
  {
  public:
    void Print() const override {}

  private:
    int* m_DeleteCount{ nullptr };

    void Setup(int* deleteCount)
    {
      m_DeleteCount = deleteCount;
    }

    void DeleteVehicle() override
    {
      if (m_DeleteCount)
      {
        ++*m_DeleteCount;
      }

      delete this;
    }

    SpyVehicle() = default;
    ~SpyVehicle() override = default;
    friend IVehicle;
  };
}

TEST(VehiclesHelperTest, DestructionDispatchesThroughTheVirtualDeleteVehicle)
{
  // Helper::operator() calls the private virtual rather than deleting the pointer, so the counter
  // only moves if that dispatch reached the most-derived override.
  int deleteCount{ 0 };
  {
    const auto vehicle{ IVehicle::Helper::CreateVehicle<SpyVehicle>(&deleteCount) };
    EXPECT_EQ(deleteCount, 0) << "the vehicle must not be deleted while its pointer is alive";
  }

  EXPECT_EQ(deleteCount, 1);
}

TEST(VehiclesHelperTest, SharedVehicleAlsoDestroysThroughDeleteVehicle)
{
  int deleteCount{ 0 };
  {
    const auto vehicle{ IVehicle::Helper::CreateVehicleShared<SpyVehicle>(&deleteCount) };
    EXPECT_EQ(deleteCount, 0);
  }

  EXPECT_EQ(deleteCount, 1);
}

TEST(VehiclesHelperTest, ConvertingMoveToBaseStillDestroysTheDerivedType)
{
  // Destruction now goes through an IVehicle*, so only the virtual dispatch inside
  // Helper::operator() can still reach SpyVehicle::DeleteVehicle.
  int deleteCount{ 0 };
  {
    UniquePtr<IVehicle, IVehicle::Helper> vehicle{
      IVehicle::Helper::CreateVehicleCustom<SpyVehicle>(&deleteCount)
    };
    EXPECT_EQ(deleteCount, 0);
  }

  EXPECT_EQ(deleteCount, 1);
}

TEST(VehiclesHelperTest, ConcreteVehiclesReallyOverrideDeleteVehicle)
{
  // The only case that runs a shipped DeleteVehicle: the ones above all dispatch to SpyVehicle.
  // Catches the override going missing, not a `delete this` dropped from Vehicles.cpp. Nothing here
  // observes that anything was freed, and RAIIGarage is not ASan-instrumented.
  testing::internal::CaptureStdout();
  {
    const auto forklift{ IVehicle::Helper::CreateVehicle<Forklift>(10, 500, 1.5f) };
  }
  const std::string output{ testing::internal::GetCapturedStdout() };

  EXPECT_TRUE(Contains(output, "This Forklift is deleted!")) << "captured output:\n" << output;
}

// Each factory writes out its own `vehicle->Setup(...)` call, so all three are covered separately.
TEST(VehiclesHelperTest, CreateVehicleForwardsArgumentsToSetup)
{
  const auto forklift{ IVehicle::Helper::CreateVehicle<Forklift>(42, 1234, 7.5f) };

  EXPECT_EQ(forklift->GetSpeed(), 42);
  EXPECT_EQ(forklift->GetCargoMax(), 1234);
  EXPECT_FLOAT_EQ(forklift->GetBalance(), 7.5f);
}

TEST(VehiclesHelperTest, CreateVehicleCustomForwardsArgumentsToSetup)
{
  const auto forklift{ IVehicle::Helper::CreateVehicleCustom<Forklift>(42, 1234, 7.5f) };

  ASSERT_TRUE(forklift); // The reads below would be undefined behaviour on an empty pointer

  EXPECT_EQ(forklift->GetSpeed(), 42);
  EXPECT_EQ(forklift->GetCargoMax(), 1234);
  EXPECT_FLOAT_EQ(forklift->GetBalance(), 7.5f);
}

TEST(VehiclesHelperTest, CreateVehicleSharedForwardsArgumentsToSetup)
{
  const auto forklift{ IVehicle::Helper::CreateVehicleShared<Forklift>(42, 1234, 7.5f) };

  ASSERT_TRUE(forklift);

  EXPECT_EQ(forklift->GetSpeed(), 42);
  EXPECT_EQ(forklift->GetCargoMax(), 1234);
  EXPECT_FLOAT_EQ(forklift->GetBalance(), 7.5f);
}
