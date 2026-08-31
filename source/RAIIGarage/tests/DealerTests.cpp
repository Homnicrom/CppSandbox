#include <gtest/gtest.h>
#include <RAIIGarage/Dealer.h>
#include <RAIIGarage/Registry.h>
#include <RAIIGarage/Vehicles.h>

#include <memory>

// Registry holds weak_ptrs, so it never disturbs the use_count assertions here. Cases that read the
// Registry back call Reset() first, so they start from an empty singleton whatever ran before them.
// Registry counts entries and not references, so a second registration of a vehicle it already
// knows is invisible until one Dealer dies and the entry survives: hence the nested scopes below.

TEST(DealerTest, ConstructionAddsASharedOwner)
{
  const auto van{ IVehicle::Helper::CreateVehicleShared<Van>(90, 800) };
  ASSERT_EQ(van.use_count(), 1);

  {
    const Dealer dealer{ van };
    EXPECT_EQ(van.use_count(), 2);
  }

  EXPECT_EQ(van.use_count(), 1) << "the Dealer must release the vehicle when it goes out of scope";
}

TEST(DealerTest, CopyConstructionAddsAnotherSharedOwner)
{
  Registry& registry{ Registry::GetInstance() };
  registry.Reset();

  const auto van{ IVehicle::Helper::CreateVehicleShared<Van>(90, 800) };

  {
    const Dealer first{ van };
    ASSERT_EQ(van.use_count(), 2);

    {
      const Dealer second{ first };
      EXPECT_EQ(van.use_count(), 3);
      EXPECT_EQ(registry.CountLiveVehicles<Van>(), 1u) << "both registrations share one entry";
    }

    EXPECT_EQ(van.use_count(), 2);
    EXPECT_EQ(registry.CountLiveVehicles<Van>(), 1u) << "the surviving Dealer must still be registered";
  }

  EXPECT_EQ(van.use_count(), 1);
  EXPECT_EQ(registry.CountLiveVehicles<Van>(), 0u);
}

// No move-construction case: Dealer's user-declared copy members and destructor suppress the
// implicit move constructor, so std::move on a Dealer runs the copy path covered above.

TEST(DealerTest, CopyAssignmentReleasesTheOldVehicleAndTakesTheNewOne)
{
  Registry& registry{ Registry::GetInstance() };
  registry.Reset();

  const auto firstVan{ IVehicle::Helper::CreateVehicleShared<Van>(90, 800) };
  const auto secondVan{ IVehicle::Helper::CreateVehicleShared<Van>(70, 400) };

  {
    Dealer holdingFirst{ firstVan };
    {
      const Dealer holdingSecond{ secondVan };
      ASSERT_EQ(firstVan.use_count(), 2);
      ASSERT_EQ(secondVan.use_count(), 2);
      ASSERT_EQ(registry.EntryCount(), 2u);

      holdingFirst = holdingSecond;

      EXPECT_EQ(firstVan.use_count(), 1) << "the reassigned Dealer must let go of its old vehicle";
      EXPECT_EQ(secondVan.use_count(), 3) << "both Dealers now hold the second vehicle";
      EXPECT_EQ(registry.EntryCount(), 1u) << "the old vehicle's only registration must be gone";
    }

    EXPECT_EQ(registry.CountLiveVehicles<Van>(), 1u) << "the reassigned Dealer must be registered on its new vehicle";
  }

  EXPECT_EQ(firstVan.use_count(), 1);
  EXPECT_EQ(secondVan.use_count(), 1);
  EXPECT_EQ(registry.CountLiveVehicles<Van>(), 0u);
}

TEST(DealerTest, SelfAssignmentKeepsTheVehicle)
{
  const auto van{ IVehicle::Helper::CreateVehicleShared<Van>(90, 800) };

  {
    Dealer dealer{ van };
    ASSERT_EQ(van.use_count(), 2);

    const Dealer& alias{ dealer }; // Routed through a reference so the compiler does not diagnose it
    dealer = alias;

    EXPECT_EQ(van.use_count(), 2) << "self-assignment must not drop or duplicate ownership";
  }

  EXPECT_EQ(van.use_count(), 1);
}

TEST(DealerTest, RegistersWithTheRegistryForItsLifetime)
{
  Registry& registry{ Registry::GetInstance() };
  registry.Reset();

  const auto van{ IVehicle::Helper::CreateVehicleShared<Van>(90, 800) };
  ASSERT_EQ(registry.CountLiveVehicles<Van>(), 0u);

  {
    const Dealer dealer{ van };

    EXPECT_EQ(registry.CountLiveVehicles<Van>(), 1u) << "construction must register the vehicle";
  }

  EXPECT_EQ(registry.CountLiveVehicles<Van>(), 0u) << "destruction must take the registration back out";
}
