---
name: googletest
description: Write, structure, run, and prune C++ unit tests with GoogleTest (and GoogleMock). Use whenever the user asks to add tests, test a C++ class or function, write fixtures, parameterized or typed tests, mocks, or debug failing/flaky GoogleTest cases, even if they only say "add tests" for C++ code. Use it also to review, audit, or trim an existing test suite: judging whether cases are redundant, duplicated, unfalsifiable, or misnamed.
---

# GoogleTest

Guidance for writing effective GoogleTest unit tests in modern C++ (C++17/20).

## Core structure

```cpp
#include <gtest/gtest.h>

// TEST(TestSuiteName, TestName): suite groups related tests.
TEST(CalculatorTest, AddsTwoPositives) {
  EXPECT_EQ(Add(2, 3), 5);
}
```

Name suites after the unit under test and tests after the behavior asserted (`AddsTwoPositives`, not `Test1`). One logical behavior per test. Names must be valid identifiers with no underscores.

Don't write a `main()`; link the `gtest_main` (and `gmock_main` if mocking) library, which provides one that calls `RUN_ALL_TESTS()`. Only write your own `main` for custom pre-test setup, and have it return `RUN_ALL_TESTS()`.

## File organization

- One test file per unit under test (`test_calculator.cpp` for `Calculator`), suite name aligned with the class.
- Include only the header of the unit under test plus `<gtest/gtest.h>`; let the linker pull in the implementation.
- When an assertion lives inside a helper function, wrap the call site with `SCOPED_TRACE("context")` so failures point back to the caller rather than the helper.

## Assertions

- `EXPECT_*` continues on failure; `ASSERT_*` aborts the current test. Use `ASSERT_*` for preconditions where continuing would crash or be meaningless (e.g. a null pointer you're about to dereference), `EXPECT_*` otherwise.
- Comparisons: `EXPECT_EQ/NE/LT/LE/GT/GE`. They print both operands on failure, so prefer them over `EXPECT_TRUE(a == b)`.
- Floats: `EXPECT_FLOAT_EQ` / `EXPECT_DOUBLE_EQ`, or `EXPECT_NEAR(val, expected, tol)`. Never `EXPECT_EQ` on floating point.
- Strings: `EXPECT_EQ` works for `std::string`; use `EXPECT_STREQ` only for `const char*`.
- Add context with `<<`: `EXPECT_EQ(list.Size(), 3) << "after inserting three items";`
- Rich assertions with matchers via `EXPECT_THAT(value, matcher)`, often clearer than a chain of `EXPECT_EQ`s and with better failure messages. Common matchers: `ElementsAre(1, 2, 3)`, `UnorderedElementsAre(...)`, `Contains(x)`, `IsEmpty()`, `SizeIs(n)`, `HasSubstr("...")`, `Optional(m)`, `Field(&T::member, m)`. E.g. `EXPECT_THAT(names, ElementsAre("a", "b"));`

## Fixtures (shared setup)

Use a fixture when multiple tests share the same setup. `TEST_F` builds a fresh fixture instance per test, so state never leaks between tests.

```cpp
class StackTest : public ::testing::Test {
protected:
  Stack<int> s;            // rebuilt for every test
  void SetUp() override {  // optional; runs before each test
    s.Push(1);
    s.Push(2);
  }
};

TEST_F(StackTest, PopReturnsLastPushed) {
  EXPECT_EQ(s.Pop(), 2);
}

TEST_F(StackTest, SizeReflectsPushes) {
  EXPECT_EQ(s.Size(), 2);  // fresh state, unaffected by the test above
}
```

Members declared in the fixture are accessible in every `TEST_F`. For a resource that is expensive to build and safe to share read-only across the whole suite, use the static `SetUpTestSuite()` / `TearDownTestSuite()` instead. Reset any global or singleton state in `SetUp()` so tests stay order-agnostic.

## Exceptions

```cpp
EXPECT_THROW(fn(), std::invalid_argument);  // throws that exact type
EXPECT_NO_THROW(fn());
EXPECT_ANY_THROW(fn());
```

To assert that code aborts/terminates the process (not throws), use death tests (`EXPECT_DEATH(stmt, "regex")`) and name the suite `*DeathTest` so it runs before other tests.

## Skipping and disabling

- `GTEST_SKIP() << "reason";` skips a test (or all tests, if placed in `SetUp()`) at runtime when a precondition isn't met; reported as skipped, not failed.
- Prefix a test or fixture name with `DISABLED_` to exclude it from runs while keeping it compiled (so it doesn't rot). Run them anyway with `--gtest_also_run_disabled_tests`.

## Parameterized tests (same logic, many inputs)

```cpp
class IsEvenTest : public ::testing::TestWithParam<int> {};

TEST_P(IsEvenTest, ReturnsTrueForEven) {
  EXPECT_TRUE(IsEven(GetParam()));
}
INSTANTIATE_TEST_SUITE_P(Evens, IsEvenTest, ::testing::Values(0, 2, 4, 100));
```

Prefer this over copy-pasted near-identical tests. `::testing::ValuesIn(container)` and `::testing::Combine(...)` cover larger input sets.

## GoogleMock (mocking interfaces)

Mock a dependency to isolate the unit under test: supply canned return values and/or assert how the dependency is called.

```cpp
#include <gmock/gmock.h>

class MockDatabase : public IDatabase {
public:
  MOCK_METHOD(int, GetUserCount, (), (const, override));
  MOCK_METHOD(bool, Save, (const User& u), (override));
};

using ::testing::Return;
using ::testing::_;

TEST(UserServiceTest, RejectsSaveWhenAtCapacity) {
  MockDatabase db;
  EXPECT_CALL(db, GetUserCount()).WillOnce(Return(100));  // canned value
  EXPECT_CALL(db, Save(_)).Times(0);                      // must NOT be called

  UserService svc(&db);
  EXPECT_FALSE(svc.AddUser(User{"alice"}));
}
```

`MOCK_METHOD(ReturnType, Name, (args), (qualifiers))`: qualifiers must include `override` and match `const`/`noexcept`; wrap comma-containing types in extra parens. Set every `EXPECT_CALL` *before* exercising the code; they are verified when the mock is destroyed. Common actions: `Return(v)`, `WillOnce(...)`, `WillRepeatedly(...)`; matchers: `_`, `Eq(x)`, `Gt(x)`, `HasSubstr(s)`. Use `NiceMock<T>` to silence warnings on uninteresting calls, `StrictMock<T>` to make them failures.

## Best practices

- **Independent tests**: any test can run alone or in any order; reset any shared global state in `SetUp()`.
- **Test behavior, not implementation**: assert on observable outputs, not private internals.
- **One reason to fail per test**: keeps failures diagnosable.
- **Arrange-Act-Assert**: structure the body in that order.
- **Deterministic**: no reliance on timing, addresses, or unseeded randomness, the main cause of most flaky tests.
- **Fast**: unit tests avoid I/O, sleeps, and network.
- **Every test must be able to fail**: see the next section.

## Redundant, repeated, and unfalsifiable tests

A test that cannot fail, or that fails only when another test already fails, costs maintenance and buys nothing. It also inflates the count in a way that hides what is genuinely untested.

**Do this before reporting any test work as finished**: re-read every case you just wrote or touched, run each one through the three checks below, drop the ones that fail them, and say in your summary which you dropped and why. Writing the cases is not the last step; this pass is. The same three checks are what to apply when reviewing an existing suite.

### 1. Can this test ever fail?

Name the implementation change that would make the assertion fail. If you cannot name one, the test is decoration.

- Asserting a postcondition the language already guarantees, such as non-null on a pointer from an allocation that throws rather than returning null.
- An assertion whose failure case is undefined behavior. If the bad state crashes or corrupts before the assertion is reached, the test reports nothing. Assert on state you can legally read back instead.
- Pinning a result that is identical under every plausible implementation, so no realistic edit can change it.
- `EXPECT_NO_THROW` around code that contains no `throw`. If the real risk is UB, a crash, or silent corruption, no-throw catches none of them. Assert on the resulting state, or use a death test.
- Restating an invariant every other test in the file already depends on, so it can never be the only failure.

### 2. Is it already covered?

- A runtime `TEST` restating a `static_assert`. The compile-time form is strictly stronger; delete the runtime copy.
- Two tests differing only in data, not in the path through the code. Same branches, no new coverage. If the inputs genuinely differ in meaning, use `TEST_P` rather than copy-paste.
- A loose assertion implied by a stricter one elsewhere, for example a "greater than zero" check where another test pins the exact value.
- A case subsumed by a strictly larger one that runs the same path plus something extra.
- Behavior owned by the language or the standard library rather than by your code. Test what your type adds on top, not what it inherits for free.

### 3. Does it test what its name says?

A name and body that drift apart are how a suite ends up with an untested path everyone believes is covered. If a test named after one function asserts only on a second one that happens to mirror its logic, either call the function in the name or rename the test. Likewise for a name describing a multi-object or multi-step scenario that the body never actually sets up.

### Remove or rewrite?

Default to removing. Rewrite only when the *subject* is worth covering and the body is what is wrong, and make the rewrite target the failure the original could not catch:

- A no-throw check on an operation applied to an empty container becomes: populate it, apply the operation to an entry that is not there, assert the existing entries were left alone. Same subject, now catches the real bug.
- A vacuous non-null check on a factory result becomes an assertion that the constructor arguments were forwarded, which proves the object is usable and covers the forwarding at once.

If deleting a duplicate would lose an assertion with real diagnostic value (one that says *which* half broke), fold that assertion into the test that survives rather than keeping both.

After trimming, leave a one-line comment saying why the remaining coverage is sufficient. Without it, the deleted cases get re-added by the next person who notices the apparent gap.

## Running / filtering

```bash
./unit_tests                         # all tests
./unit_tests --gtest_filter=StackTest.*
./unit_tests --gtest_repeat=100 --gtest_shuffle   # flush out flakiness / order deps
ctest --output-on-failure
```
