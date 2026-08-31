add_executable(MemorySafetyDrills main.cpp)
target_compile_features(MemorySafetyDrills PRIVATE cxx_std_23)
set_target_properties(MemorySafetyDrills PROPERTIES CXX_STANDARD_REQUIRED ON)

option(MEMSAFETYDRILLS_ENABLE_ASAN "Build MemorySafetyDrills with AddressSanitizer" ON)

if (MEMSAFETYDRILLS_ENABLE_ASAN)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(MemorySafetyDrills PRIVATE -fsanitize=address -g)
        target_link_options(MemorySafetyDrills PRIVATE -fsanitize=address)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(MemorySafetyDrills PRIVATE /fsanitize=address)

        # link.exe auto-links the ASan import lib via /InferAsanLibs, but nothing
        # copies the matching runtime DLL next to the .exe (that normally happens via
        # the IDE's EnableASan project property, which CMake doesn't set); do it here.
        get_filename_component(_msvc_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)

        # The compiler directory already follows the target architecture, the DLL name does not:
        # the x64 tools ship clang_rt.asan*dynamic-x86_64.dll, the x86 ones ...-i386.dll.
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_asan_arch "x86_64")
        else()
            set(_asan_arch "i386")
        endif()

        file(GLOB _asan_dlls "${_msvc_bin_dir}/clang_rt.asan*dynamic-${_asan_arch}.dll")

        # An empty glob would collapse the command below into a one-argument copy, failing every
        # build of this target with an arity error that says nothing about ASan.
        if (_asan_dlls)
            add_custom_command(TARGET MemorySafetyDrills POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_asan_dlls} "$<TARGET_FILE_DIR:MemorySafetyDrills>"
                COMMENT "Copying ASan runtime DLL next to MemorySafetyDrills.exe"
            )
        else()
            message(WARNING "No ASan runtime DLL matching clang_rt.asan*dynamic-${_asan_arch}.dll in ${_msvc_bin_dir}. MemorySafetyDrills.exe will not start. Configure for a supported architecture or pass -DMEMSAFETYDRILLS_ENABLE_ASAN=OFF.")
        endif()
    endif()
endif()
