cmake_minimum_required(VERSION 3.22)

project(ZenoSetup
    LANGUAGES CXX C
    DESCRIPTION "Open-source node system framework for simulation and others"
    HOMEPAGE_URL https://github.com/zenustech/zeno-setup
    )

add_subdirectory(winsetup)
add_subdirectory(zenoinstall)
add_subdirectory(uninstall)