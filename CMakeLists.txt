cmake_minimum_required(VERSION 3.5)

## Project name: ##
project(cynth CXX)

## C++ Standard: ##
set(CMAKE_CXX_STANDARD 17)

## Warnings: ##
set(CMAKE_CXX_FLAGS "-Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/ext/ASIOSDK2.3.2/common)
include_directories(${PROJECT_SOURCE_DIR}/ext/ASIOSDK2.3.2/host)
include_directories(${PROJECT_SOURCE_DIR}/ext/ASIOSDK2.3.2/host/pc)
include_directories(${PROJECT_SOURCE_DIR}/ext/gcem/include)

set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/entry.cpp
    ${PROJECT_SOURCE_DIR}/ext/ASIOSDK2.3.2/common/asio.cpp
    ${PROJECT_SOURCE_DIR}/ext/ASIOSDK2.3.2/host/asiodrivers.cpp
    ${PROJECT_SOURCE_DIR}/ext/ASIOSDK2.3.2/host/pc/asiolist.cpp)

ADD_LIBRARY(LIBS ${SOURCES})

target_link_libraries(LIBS -luuid)
target_link_libraries(LIBS -lksuser)
target_link_libraries(LIBS -lole32)
target_link_libraries(LIBS -lwinmm)

add_executable(cynth ${SOURCES})

target_link_libraries(cynth LIBS)