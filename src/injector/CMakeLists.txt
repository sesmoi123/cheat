# ---------------------------------------------------------------------------
# La DLL du cheat est embarquee dans l'exe en ressource RCDATA :
# l'utilisateur ne recoit qu'un seul fichier a lancer.
# ---------------------------------------------------------------------------
set(RETRAC_RESOURCE_RC "${CMAKE_BINARY_DIR}/generated/cheat_dll.rc")

file(GENERATE
    OUTPUT "${RETRAC_RESOURCE_RC}"
    CONTENT "RETRAC_CHEAT_DLL RCDATA \"$<TARGET_FILE:RetracCheat>\"\n")

set_source_files_properties("${RETRAC_RESOURCE_RC}" PROPERTIES GENERATED TRUE)

add_executable(RetracOGFN-Launcher WIN32
    main.cpp
    Injector.hpp
    Injector.cpp
    Resource.hpp
    Resource.cpp
    "${RETRAC_RESOURCE_RC}")

target_link_libraries(RetracOGFN-Launcher PRIVATE common shell32)

# La DLL doit exister avant la compilation de la ressource.
add_dependencies(RetracOGFN-Launcher RetracCheat)

# Recompile la ressource quand la DLL change (sinon l'exe embarquerait l'ancienne).
set_property(SOURCE "${RETRAC_RESOURCE_RC}" APPEND PROPERTY OBJECT_DEPENDS "$<TARGET_FILE:RetracCheat>")

# Manifest maison : elevation administrateur obligatoire (EasyAntiCheat refuse
# l'ouverture d'un handle Fortnite a un process non eleve).
target_link_options(RetracOGFN-Launcher PRIVATE
    "/MANIFESTUAC:NO"
    "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/RetracOGFN.manifest")

set_target_properties(RetracOGFN-Launcher PROPERTIES
    OUTPUT_NAME "RetracOGFN")

# Copie aussi la DLL a cote de l'exe (pratique en dev). L'exe la prefere si elle
# est presente, sinon il utilise celle embarquee.
add_custom_command(TARGET RetracOGFN-Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:RetracCheat>"
            "$<TARGET_FILE_DIR:RetracOGFN-Launcher>")
