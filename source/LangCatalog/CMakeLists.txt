add_executable(LangCatalog main.cpp)
target_compile_features(LangCatalog PRIVATE cxx_std_17)

add_custom_command(TARGET LangCatalog POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/languages.txt
            $<TARGET_FILE_DIR:LangCatalog>/languages.txt
)
