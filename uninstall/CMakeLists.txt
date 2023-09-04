find_package(Qt5 COMPONENTS Widgets OpenGL Network REQUIRED)

message(STATUS "Found Qt5 version ${Qt5_VERSION}: ${Qt5_DIR}")

file(GLOB_RECURSE source CONFIGURE_DEPENDS *.h *.cpp *.ui)
file(GLOB_RECURSE resource CONFIGURE_DEPENDS *.qrc)
qt5_add_resources(source ${resource})

if (WIN32)
    enable_language("RC")
    set (WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uninstall.rc)
endif()

if(MSVC)
    add_compile_options(
        $<$<CONFIG:>:/MT> #---------|
        $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
        $<$<CONFIG:Release>:/MT> #--|
    )
endif()

add_executable(uninstall ${source} ${WIN32_RESOURCES}) 

set_property(TARGET uninstall PROPERTY AUTOUIC ON)
set_property(TARGET uninstall PROPERTY AUTOMOC ON)
set_property(TARGET uninstall PROPERTY AUTORCC ON)
set_property(TARGET uninstall PROPERTY WIN32_EXECUTABLE ON)

if (WIN32)
    SET_TARGET_PROPERTIES(uninstall PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS")
endif()

target_link_libraries(uninstall PRIVATE Qt5::Widgets Qt5::OpenGL Qt5::Network)
target_link_libraries(uninstall PRIVATE winsetup)

add_definitions(-D_UNICODE)