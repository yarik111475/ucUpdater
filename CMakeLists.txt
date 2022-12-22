cmake_minimum_required(VERSION 3.14)

project(ucUpdater LANGUAGES CXX)

#static link for linux
if(LINUX)
    set (LINUX_STATIC_LINK_OPTION
        -pthread
        -static-libgcc
        -static-libstdc++
    )
endif()

#spdlog
set(SPDLOG_MASTER_PROJECT OFF)
set(SPDLOG_INSTALL OFF)
set(SPDLOG_BUILD_SHARED OFF)

#boost
if(WIN32)
    set(BOOST_ROOT "C:/boost_1_76_0")
    set(BOOST_INCLUDEDIR "C:/boost_1_76_0/boost")
    set(BOOST_LIBRARYDIR "C:/boost_1_76_0/stage/lib")
endif()

if(LINUX)
    set(BOOST_ROOT "/usr")
    set(BOOST_INCLUDEDIR "/usr/include")
    set(BOOST_LIBRARYDIR "/usr/lib")
endif()

set(Boost_DEBUG 1)
set(Boost_USE_STATIC_LIBS ON)
#set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not search system for Boost")

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions( -DHAS_BOOST )
#add_definitions(-DBOOST_USE_WINDOWS_H)
add_definitions(-D_WIN32_WINNT=0x0601)

add_subdirectory(usUpdater/src)
add_subdirectory(spdlog-1.9.2)

#rpath: use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

#rpath: when building, don't use the install RPATH already, (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};../lib")

#rpath: add the automatically determined parts of the RPATH
#which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(MSVC)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${PROJECT_SOURCE_DIR}/utils/decode.bat")
endif()

