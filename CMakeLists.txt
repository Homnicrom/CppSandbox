cmake_minimum_required(VERSION 3.20)
project(CppSandbox LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
  URL_HASH SHA256=63b9c77751a5b8f492486005f67533fdc58682b67476fcb91650be5958d5195a
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Make all target's executable land under build/bin/<TargetName>/<Config>/, without separate properties
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<TARGET_PROPERTY:NAME>/$<CONFIG>")

# Must be called here, in the source root, so CTest chains down into the subdirectories added below.
enable_testing()

add_subdirectory(source projects)