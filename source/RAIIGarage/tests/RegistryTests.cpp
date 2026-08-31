#include <gtest/gtest.h>
#include <RAIIGarage/Registry.h>
#include <RAIIGarage/Vehicles.h>
#include <RAIIGarage/tests/CapturedOutput.h>

#include <memory>
#include <string>

// Registry is a process-wide singleton, so its state outlives any single test. The fixture's SetUp
// calls Reset(), which is what keeps these cases independent of each other, of any other suite that
// registers a vehicle, and of the order they run in. Any future suite observing the Registry should
// derive from a fixture that does the same.
// CountLiveVehicles<T>() covers registered state; output is captured only where CallVehicles<T>()'s
// own printing is the subject.
using TestSupport::Contains;

namespace
{
  std::string CaptureCallVehiclesOfTrucks()
  {
    testing::internal::CaptureStdout();
    Registry::GetInstance().CallVehicles<Truck>();
    return testing::internal::GetCapturedStdout();
  }
}

class RegistryTest : public testing::Test
{
protected:
  void SetUp() override
  {
    Registry::GetInstance().Reset();
  }

  Registry& registry{ Registry::GetInstance() };
};

TEST_F(RegistryTest, ResetForgetsEveryRegistrationWithoutDestroyingTheVehicle)
{
  const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
  registry.RegisterVehicle(truck);
  ASSERT_EQ(registry.CountLiveVehicles<Truck>(), 1u);

  registry.Reset();

  EXPECT_EQ(registry.CountLiveVehicles<Truck>(), 0u);
  EXPECT_EQ(truck.use_count(), 1) << "Reset only drops weak observations, it must not free anything";
}

TEST_F(RegistryTest, TypedCallPrintsOnlyExactTypeMatches)
{
  const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
  const auto car{ IVehicle::Helper::CreateVehicleShared<Car>(120, "Test Car") };
  registry.RegisterVehicle(truck);
  registry.RegisterVehicle(car);

  const std::string output{ CaptureCallVehiclesOfTrucks() };

  EXPECT_TRUE(Contains(output, "This is a Truck!")) << "captured output:\n" << output;
  EXPECT_FALSE(Contains(output, "This is a Car!")) << "captured output:\n" << output;
}

TEST_F(RegistryTest, UntypedCallPrintsEveryLiveVehicle)
{
  const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
  const auto car{ IVehicle::Helper::CreateVehicleShared<Car>(120, "Test Car") };
  registry.RegisterVehicle(truck);
  registry.RegisterVehicle(car);

  testing::internal::CaptureStdout();
  registry.CallVehicles();
  const std::string output{ testing::internal::GetCapturedStdout() };

  EXPECT_TRUE(Contains(output, "This is a Truck!")) << "captured output:\n" << output;
  EXPECT_TRUE(Contains(output, "This is a Car!")) << "captured output:\n" << output;
}

TEST_F(RegistryTest, TypedCallIgnoresTheAbstractBaseType)
{
  // typeid comparison is exact, so no concrete vehicle ever matches IVehicle itself.
  const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
  registry.RegisterVehicle(truck);

  testing::internal::CaptureStdout();
  registry.CallVehicles<IVehicle>();
  const std::string output{ testing::internal::GetCapturedStdout() };

  // The header proves the capture ran, so the negative below cannot pass on an empty string.
  EXPECT_TRUE(Contains(output, "Calling Vehicles of type")) << "captured output:\n" << output;
  EXPECT_FALSE(Contains(output, "This is a Truck!")) << "captured output:\n" << output;
}

TEST_F(RegistryTest, SkipsExpiredEntriesWithoutCrashing)
{
  {
    const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
    registry.RegisterVehicle(truck);
  }

  // The vehicle is gone but its entry is still in the vector, since only RemoveVehicle erases. The
  // dead entry is walked here and the lock() guard is what has to skip it.
  const std::string output{ CaptureCallVehiclesOfTrucks() };

  // The header proves the loop ran, so the negative below cannot pass on an empty string.
  EXPECT_TRUE(Contains(output, "Calling Vehicles of type")) << "captured output:\n" << output;
  EXPECT_FALSE(Contains(output, "This is a Truck!")) << "captured output:\n" << output;
  EXPECT_EQ(registry.CountLiveVehicles<Truck>(), 0u) << "the expired entry must not be counted either";
}

TEST_F(RegistryTest, RegisteringTheSameVehicleTwiceRequiresTwoRemovals)
{
  const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };

  registry.RegisterVehicle(truck);
  registry.RegisterVehicle(truck);

  registry.RemoveVehicle(truck);
  const std::size_t countAfterFirstRemoval{ registry.CountLiveVehicles<Truck>() };

  registry.RemoveVehicle(truck);
  const std::size_t countAfterSecondRemoval{ registry.CountLiveVehicles<Truck>() };

  // The vehicle stays alive throughout, so only the entry's refCount decides this.
  EXPECT_EQ(countAfterFirstRemoval, 1u) << "one removal should not undo two registrations";
  EXPECT_EQ(countAfterSecondRemoval, 0u);
}

TEST_F(RegistryTest, RegisteringANullVehicleAddsNoEntry)
{
  // Asserted on EntryCount because a null slot is invisible to CountLiveVehicles<T>(), which only reports
  // entries that still lock. Matching on lock().get() would make null compare equal to every
  // expired entry, and finding none here, claim a slot of its own.
  const auto truck{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
  registry.RegisterVehicle(truck);

  registry.RegisterVehicle(std::shared_ptr<IVehicle>{});

  EXPECT_EQ(registry.EntryCount(), 1u) << "null must not occupy a slot of its own";
  EXPECT_EQ(registry.CountLiveVehicles<Truck>(), 1u) << "and must not disturb the real entry";
}

TEST_F(RegistryTest, RemovingAnUnregisteredVehicleLeavesOtherEntriesAlone)
{
  // The interesting failure is not a throw, it is decrementing some other vehicle's entry when the
  // scan finds no match.
  const auto registered{ IVehicle::Helper::CreateVehicleShared<Truck>(80, 2, 12000.0f) };
  const auto neverRegistered{ IVehicle::Helper::CreateVehicleShared<Truck>(60, 1, 5000.0f) };
  registry.RegisterVehicle(registered);

  registry.RemoveVehicle(neverRegistered);

  EXPECT_EQ(registry.CountLiveVehicles<Truck>(), 1u) << "an unmatched removal must not drop another entry";

  registry.RemoveVehicle(registered);

  EXPECT_EQ(registry.CountLiveVehicles<Truck>(), 0u) << "the registered vehicle must still be removable afterwards";
}
