# ---------------------------------------------------------------------------
# La DLL du cheat est embarquee dans l'exe en ressource RCDATA :
# l'utilisateur ne recoit qu'un seul fichier a lancer.
# ---------------------------------------------------------------------------
set(RETRAC_RESOURCE_RC "${CMAKE_BINARY_DIR}/generated/cheat_dll.rc")

# $<SHELL_PATH:...> -> slashes avant : RC interprete les backslashes comme des
# sequences d'echappement ("\b" dans "build\bin" deviendrait un backspace).
file(GENERATE
    OUTPUT "${RETRAC_RESOURCE_RC}"
    CONTENT "RETRAC_CHEAT_DLL RCDATA \"$<SHELL_PATH:$<TARGET_FILE:RetracCheat>>\"\n")

set_source_files_properties("${RETRAC_RESOURCE_RC}" PROPERTIES GENERATED TRUE)

add_executable(RetracOGFN-Launcher WIN32
    main.cpp
    Elevation.hpp
    Elevation.cpp
    Injector.hpp
    Injector.cpp
    Resource.hpp
    Resource.cpp
    "${RETRAC_RESOURCE_RC}")

# advapi32 : OpenProcessToken / AdjustTokenPrivileges / LookupPrivilegeValue.
target_link_libraries(RetracOGFN-Launcher PRIVATE common shell32 advapi32)

add_dependencies(RetracOGFN-Launcher RetracCheat)

set_property(SOURCE "${RETRAC_RESOURCE_RC}" APPEND PROPERTY OBJECT_DEPENDS "$<TARGET_FILE:RetracCheat>")

# Manifest maison : elevation administrateur obligatoire.
target_link_options(RetracOGFN-Launcher PRIVATE
    "/MANIFESTUAC:NO"
    "/MANIFESTINPUT:\"${CMAKE_CURRENT_SOURCE_DIR}/RetracOGFN.manifest\"")

set_target_properties(RetracOGFN-Launcher PROPERTIES OUTPUT_NAME "RetracOGFN")

# Copie la DLL a cote de l'exe (pratique en dev) : l'exe la prefere si presente.
add_custom_command(TARGET RetracOGFN-Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:RetracCheat>"
            "$<TARGET_FILE_DIR:RetracOGFN-Launcher>")
