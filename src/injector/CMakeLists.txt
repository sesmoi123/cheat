add_executable(RetracOGFN-Launcher
    main.cpp
    Injector.hpp
    Injector.cpp)

target_link_libraries(RetracOGFN-Launcher PRIVATE common)

set_target_properties(RetracOGFN-Launcher PROPERTIES
    OUTPUT_NAME "RetracOGFN"
    VS_DPI_AWARE "PerMonitor")

add_dependencies(RetracOGFN-Launcher RetracCheat)

add_custom_command(TARGET RetracOGFN-Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:RetracCheat>"
            "$<TARGET_FILE_DIR:RetracOGFN-Launcher>")
