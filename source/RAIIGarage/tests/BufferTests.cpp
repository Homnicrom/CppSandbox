#include <gtest/gtest.h>
#include <RAIIGarage/Buffer.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

// Buffers report their special-member operations to a TraceLog instead of printing, which is what lets
// destruction be counted per value. A buffer given no observer prints, so stray output during a run
// means one was missed. The log must be declared before the scope holding the buffers: their
// destructors report into it.
namespace
{
  struct TraceEntry
  {
    Buffer::BufferOperation operation;
    std::optional<int> value;
  };

  class TraceLog : public Buffer::IBufferObserver
  {
  public:
    void OnBufferOperation(const Buffer::BufferOperation operation, const std::optional<int> value) override
    {
      m_Entries.push_back(TraceEntry{ operation, value });
    }

    // `value` is matched exactly, so std::nullopt asks for operations on a buffer that owned nothing.
    std::size_t CountOperations(const Buffer::BufferOperation operation, const std::optional<int> value) const
    {
      const auto matches{ std::ranges::count_if(m_Entries, [&](const TraceEntry& entry) {
        return entry.operation == operation && entry.value == value;
      }) };

      return static_cast<std::size_t>(matches);
    }

  private:
    std::vector<TraceEntry> m_Entries;
  };
}

TEST(BufferTest, ValueConstructionOwnsTheGivenValue)
{
  TraceLog trace;
  const Buffer buffer{ 42, &trace };

  EXPECT_EQ(buffer.GetValue(), 42);
}

TEST(BufferTest, CopyConstructionDeepCopiesTheValue)
{
  TraceLog trace;
  {
    const Buffer original{ 42, &trace };
    const Buffer copy{ original }; // Handed no tracer, so its operations land here only if m_Observer was copied

    EXPECT_EQ(copy.GetValue(), 42);
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::CopyConstruct, 42), 1u)
    << "the copy must report through the source's tracer instead of falling back to printing";
  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 42), 2u)
    << "two independent allocations must be destroyed, not one shared one";
}

TEST(BufferTest, MoveConstructionEmptiesTheSource)
{
  TraceLog trace;
  {
    Buffer source{ 42, &trace };
    const Buffer moved{ std::move(source) }; // The tracer is copied, not transferred: both stay observed

    EXPECT_EQ(moved.GetValue(), 42);
    EXPECT_EQ(source.GetValue(), std::nullopt) << "the moved-from object must have been nulled";
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::MoveConstruct, 42), 1u)
    << "the moved-to object must report through the source's tracer";
  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 42), 1u)
    << "the value must be freed exactly once";
  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, std::nullopt), 1u)
    << "the moved-from source must keep observing its own destruction";
}

TEST(BufferTest, CopyingAMovedFromBufferProducesAnEmptyBuffer)
{
  // Exercises the copy constructor's null guard: the source genuinely holds nullptr here.
  TraceLog trace;
  {
    Buffer source{ 7, &trace };
    const Buffer moved{ std::move(source) };
    const Buffer copyOfEmpty{ source };

    EXPECT_EQ(copyOfEmpty.GetValue(), std::nullopt);
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 7), 1u);
  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, std::nullopt), 2u)
    << "the emptied source and the empty copy must both be destroyed owning nothing";
}

TEST(BufferTest, CopyAssignmentDeepCopiesAndFreesTheOldValue)
{
  TraceLog trace;
  {
    Buffer target{ 1, &trace };
    const Buffer source{ 2, &trace };

    target = source;

    EXPECT_EQ(target.GetValue(), 2);
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 2), 2u)
    << "both buffers must own their own copy of the value";
  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 1), 0u)
    << "the target's old value is freed inside the assignment, not by a destructor";
}

TEST(BufferTest, MoveAssignmentFreesTheTargetsOldValueAndEmptiesTheSource)
{
  TraceLog trace;
  {
    Buffer target{ 1, &trace };
    Buffer source{ 2, &trace };

    target = std::move(source);

    EXPECT_EQ(target.GetValue(), 2);
    EXPECT_EQ(source.GetValue(), std::nullopt) << "the moved-from source must have been emptied";
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 1), 0u)
    << "the target's old value is freed inside the assignment, not by a destructor";
  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 2), 1u)
    << "the moved value must be freed exactly once";
}

TEST(BufferTest, AssignmentDoesNotAdoptTheSourcesTracer)
{
  // Construction propagates the observer, assignment does not. Two logs make the difference visible.
  TraceLog targetTrace;
  TraceLog sourceTrace;
  {
    Buffer target{ 1, &targetTrace };
    const Buffer source{ 2, &sourceTrace };

    target = source;
  }

  EXPECT_EQ(targetTrace.CountOperations(Buffer::BufferOperation::CopyAssign, 2), 1u)
    << "the assignment itself must still be reported by the target's own observer";
  EXPECT_EQ(targetTrace.CountOperations(Buffer::BufferOperation::Destroy, 2), 1u)
    << "the target must still be observed after being assigned to";
  EXPECT_EQ(sourceTrace.CountOperations(Buffer::BufferOperation::Destroy, 2), 1u)
    << "and the source must report only its own destruction";
}

TEST(BufferTest, SelfCopyAssignmentKeepsTheValue)
{
  TraceLog trace;
  {
    Buffer buffer{ 55, &trace };

    const Buffer& alias{ buffer }; // Routed through a reference so the compiler does not diagnose it
    buffer = alias;

    EXPECT_EQ(buffer.GetValue(), 55);
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 55), 1u)
    << "self-assignment must not free the value it is copying from";
}

TEST(BufferTest, SelfMoveAssignmentKeepsTheValue)
{
  TraceLog trace;
  {
    Buffer buffer{ 55, &trace };

    Buffer& alias{ buffer };
    buffer = std::move(alias);

    EXPECT_EQ(buffer.GetValue(), 55);
  }

  EXPECT_EQ(trace.CountOperations(Buffer::BufferOperation::Destroy, 55), 1u)
    << "self-move must leave the object holding its value";
}
