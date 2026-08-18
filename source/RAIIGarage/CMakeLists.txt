add_executable(RAIIGarage
    main.cpp
    Dealer.cpp
    Registry.cpp
    Vehicles.cpp
    Dealer.h
    Registry.h
    Vehicles.h
    UniquePtr.h
    Buffer.h
    Utilities.h
    ScopedTimer.h
)

target_compile_features(RAIIGarage PRIVATE cxx_std_23)
set_target_properties(RAIIGarage PROPERTIES CXX_STANDARD_REQUIRED ON)
target_include_directories(RAIIGarage PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
