include(GoogleTest)

# The project's own code, compiled once and shared by the demo executable and the tests.
# main.cpp is deliberately not part of it: it holds only the demo script and its main(),
# which would clash with the one gtest_main provides.
#
# The compile features and include directory are PUBLIC, so they are part of this library's
# interface and both consumers inherit them from here. That is the point of the target: it
# makes it impossible for the tests to end up compiling these sources under different
# settings than RAIIGarage.exe does.
add_library(RAIIGarageLib OBJECT
    Dealer.cpp
    Registry.cpp
    Vehicles.cpp
    Buffer.cpp
    Dealer.h
    Registry.h
    Vehicles.h
    UniquePtr.h
    Buffer.h
    Utilities.h
    ScopedTimer.h
)

target_compile_features(RAIIGarageLib PUBLIC cxx_std_23)
target_include_directories(RAIIGarageLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(RAIIGarageTests
    tests/CapturedOutput.h
    tests/UniquePtrTests.cpp
    tests/VehiclesHelperTests.cpp
    tests/RegistryTests.cpp
    tests/DealerTests.cpp
    tests/BufferTests.cpp
    tests/UtilitiesMaxTests.cpp
)
target_link_libraries(RAIIGarageTests PRIVATE RAIIGarageLib GTest::gtest_main)
gtest_discover_tests(RAIIGarageTests)

# Run tests after build, forced with custom target
add_custom_target(RunRAIIGarageTests
    COMMAND RAIIGarageTests
    COMMENT "Running RAIIGarage tests"
)

add_executable(RAIIGarage main.cpp)
target_link_libraries(RAIIGarage PRIVATE RAIIGarageLib)
# Don't build unless the tests pass
add_dependencies(RAIIGarage RunRAIIGarageTests)
