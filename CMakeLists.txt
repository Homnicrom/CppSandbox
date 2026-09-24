cmake_minimum_required(VERSION 3.21)
project(CppSandbox LANGUAGES CXX)

# libstdc++ ships <print> only from GCC 14, but GCC 13 accepts -std=c++23, so
# target_compile_features can't catch it and the build fails later on a missing header.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
    message(FATAL_ERROR
        "GCC ${CMAKE_CXX_COMPILER_VERSION} has no <print> header; GCC 14 or newer is required. "
        "Install g++-14 and configure with `cmake --preset linux`, which selects it.")
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
  URL_HASH SHA256=63b9c77751a5b8f492486005f67533fdc58682b67476fcb91650be5958d5195a
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Windows: keep gtest from overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Every executable lands under build/bin/<TargetName>/<Config>/, with no per-target properties
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<TARGET_PROPERTY:NAME>/$<CONFIG>")

# Must be at the source root for CTest to chain down into the subdirectories added below.
enable_testing()

add_subdirectory(source projects)