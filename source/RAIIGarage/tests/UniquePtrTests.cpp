#include <gtest/gtest.h>
#include <RAIIGarage/UniquePtr.h>

#include <utility>

// Test-only types. The vehicles cannot be constructed outside IVehicle::Helper, and counting
// deleter calls needs a type whose destruction this suite controls.
namespace
{
  // Aggregate so CountingDeleter{&count} works, default-constructible for UniquePtr's m_Deleter.
  struct CountingDeleter
  {
    int* count{ nullptr };

    void operator()(int* ptr) const
    {
      if (count)
      {
        ++*count;
      }
      delete ptr;
    }
  };

  struct Probe
  {
    int value{ 7 };
  };

  struct Base
  {
    virtual ~Base() = default;
    virtual int Value() const { return 1; }
  };

  struct Derived : Base
  {
    int Value() const override { return 2; }
  };

  // Takes Base*, so the same deleter type fits both sides of the converting move.
  struct BaseCountingDeleter
  {
    int* count{ nullptr };

    void operator()(Base* ptr) const
    {
      if (count)
      {
        ++*count;
      }
      delete ptr;
    }
  };
}

TEST(UniquePtrTest, DefaultConstructedOwnsNothing)
{
  UniquePtr<int> ptr{};

  EXPECT_FALSE(ptr);
  EXPECT_EQ(ptr.Get(), nullptr);
}

TEST(UniquePtrTest, DereferencesTheOwnedObject)
{
  UniquePtr<int> ptr{ new int{ 42 } };

  EXPECT_EQ(*ptr, 42);
}

TEST(UniquePtrTest, ArrowOperatorReachesMembers)
{
  UniquePtr<Probe> ptr{ new Probe{} };

  EXPECT_EQ(ptr->value, 7);
}

TEST(UniquePtrTest, MoveConstructorTransfersOwnershipAndNullsSource)
{
  int count{ 0 };
  {
    UniquePtr<int, CountingDeleter> source{ new int{ 13 }, CountingDeleter{ &count } };
    const int* const originalAddress{ source.Get() };

    UniquePtr<int, CountingDeleter> moved{ std::move(source) };

    EXPECT_EQ(moved.Get(), originalAddress) << "moved-to must own the original object";
    EXPECT_EQ(source.Get(), nullptr) << "moved-from must have been nulled";
  }

  EXPECT_EQ(count, 1) << "both objects were destroyed but only one owned the resource";
}

TEST(UniquePtrTest, DestructorInvokesDeleterExactlyOnce)
{
  int count{ 0 };
  {
    UniquePtr<int, CountingDeleter> ptr{ new int{ 1 }, CountingDeleter{ &count } };
    EXPECT_EQ(count, 0) << "deleter must not run while the pointer is still alive";
  }

  EXPECT_EQ(count, 1);
}

TEST(UniquePtrTest, ReleaseDoesNotInvokeDeleter)
{
  int count{ 0 };
  int* raw{ nullptr };
  {
    UniquePtr<int, CountingDeleter> ptr{ new int{ 5 }, CountingDeleter{ &count } };
    raw = ptr.Release();
    EXPECT_EQ(count, 0);
  }

  EXPECT_EQ(count, 0) << "Release hands ownership to the caller, so the deleter must not run";
  ASSERT_NE(raw, nullptr);
  EXPECT_EQ(*raw, 5);

  delete raw; // the test owns it now.
}

TEST(UniquePtrTest, ResetDeletesPreviousResourceAndAdoptsTheNewOne)
{
  int count{ 0 };
  {
    UniquePtr<int, CountingDeleter> ptr{ new int{ 1 }, CountingDeleter{ &count } };

    ptr.Reset(new int{ 2 });

    EXPECT_EQ(count, 1) << "the previously owned object must be deleted by Reset";
    EXPECT_EQ(*ptr, 2);
  }

  EXPECT_EQ(count, 2) << "the adopted object must be deleted at scope exit";
}

TEST(UniquePtrTest, ResetWithNoArgumentReleasesOwnership)
{
  int count{ 0 };
  {
    UniquePtr<int, CountingDeleter> ptr{ new int{ 1 }, CountingDeleter{ &count } };

    ptr.Reset();

    EXPECT_EQ(count, 1);
  }

  EXPECT_EQ(count, 1) << "destructor must not delete again after Reset emptied the pointer";
}

TEST(UniquePtrTest, MoveAssignmentDeletesTheTargetsPreviousResource)
{
  int count{ 0 };
  {
    UniquePtr<int, CountingDeleter> target{ new int{ 1 }, CountingDeleter{ &count } };
    UniquePtr<int, CountingDeleter> source{ new int{ 2 }, CountingDeleter{ &count } };

    target = std::move(source);

    EXPECT_EQ(count, 1) << "the target's original resource must be freed on assignment";
    EXPECT_EQ(*target, 2);
  }

  EXPECT_EQ(count, 2) << "only the surviving resource is freed at scope exit";
}

TEST(UniquePtrTest, SelfMoveAssignmentLeavesThePointerIntact)
{
  int count{ 0 };
  {
    UniquePtr<int, CountingDeleter> ptr{ new int{ 99 }, CountingDeleter{ &count } };

    UniquePtr<int, CountingDeleter>& alias{ ptr }; // Routed through a reference so the compiler does not diagnose it
    ptr = std::move(alias);

    EXPECT_EQ(count, 0) << "self-move must not free the owned object";
    EXPECT_EQ(*ptr, 99);
  }

  EXPECT_EQ(count, 1);
}

TEST(UniquePtrTest, ConvertingMoveTransfersFromDerivedToBase)
{
  int count{ 0 };
  {
    UniquePtr<Derived, BaseCountingDeleter> derived{ new Derived{}, BaseCountingDeleter{ &count } };

    UniquePtr<Base, BaseCountingDeleter> base{ std::move(derived) };

    EXPECT_EQ(base->Value(), 2) << "the base pointer must still dispatch to Derived";
    EXPECT_EQ(derived.Get(), nullptr) << "the source must have been released";
    EXPECT_EQ(base.GetDeleter().count, &count) << "the source's deleter must have been carried across";
    EXPECT_EQ(count, 0);
  }

  EXPECT_EQ(count, 1) << "exactly one deleter call despite the conversion";
}
