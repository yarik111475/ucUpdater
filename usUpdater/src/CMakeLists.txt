cmake_minimum_required(VERSION 3.14)

project(usUpdater LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost COMPONENTS
    thread
    date_time
    filesystem
    program_options
)

set(SOURCES
    main.cpp
)

set(HEADERS
)

add_executable(usUpdater
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(usUpdater
    PRIVATE
    ${Boost_INCLUDE_DIRS}
    #${CMAKE_CURRENT_SOURCE_DIR}/../../spdlog-1.9.2/include
)

target_link_libraries(usUpdater
    spdlog
    ${Boost_LIBRARIES}
    ${LINUX_STATIC_LINK_OPTION}
)

install(TARGETS usUpdater
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
