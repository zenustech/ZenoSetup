find_package(Qt5 COMPONENTS Widgets OpenGL Network REQUIRED)

message(STATUS "Found Qt5 version ${Qt5_VERSION}: ${Qt5_DIR}")

file(GLOB_RECURSE source CONFIGURE_DEPENDS *.h *.cpp *.ui)
file(GLOB_RECURSE resource CONFIGURE_DEPENDS *.qrc)
qt5_add_resources(source ${resource})

if (WIN32)
    enable_language("RC")
    set (WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/zenoinstall.rc)
endif()

if(MSVC)
    add_compile_options(
        $<$<CONFIG:>:/MT> #---------|
        $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
        $<$<CONFIG:Release>:/MT> #--|
    )
endif()

add_executable(zenoinstall ${source} ${WIN32_RESOURCES})

set_property(TARGET zenoinstall PROPERTY AUTOUIC ON)
set_property(TARGET zenoinstall PROPERTY AUTOMOC ON)
set_property(TARGET zenoinstall PROPERTY AUTORCC ON)
set_property(TARGET zenoinstall PROPERTY WIN32_EXECUTABLE ON)

if (WIN32)
    SET_TARGET_PROPERTIES(zenoinstall PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS")
endif()

target_link_libraries(zenoinstall PRIVATE Qt5::Widgets Qt5::OpenGL Qt5::Network)
target_link_libraries(zenoinstall PRIVATE winsetup ws2_32.lib winmm.lib wldap32.lib Advapi32.lib Crypt32.lib DXGI.lib)

message(${CMAKE_BUILD_TYPE})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(zenoinstall PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/../thr/libs/x64/static/debug/libcrypto.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../thr/libs/x64/static/debug/libcurl.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../thr/libs/x64/static/debug/libssl.lib)
else()
    target_link_libraries(zenoinstall PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/../thr/libs/x64/static/release/libcrypto.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../thr/libs/x64/static/release/libcurl.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../thr/libs/x64/static/release/libssl.lib)
endif()

target_include_directories(zenoinstall PRIVATE . .. ../thr/include)

add_definitions(-D_UNICODE)