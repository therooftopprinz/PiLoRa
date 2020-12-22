cmake_minimum_required(VERSION 3.10)

project(pilora)

set(CMAKE_VERBOSE_MAKEFILE on)

include(ExternalProject)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

FetchContent_Declare(
    pigpiohwapi
    GIT_REPOSITORY https://github.com/therooftopprinz/PiGpioHwApi.git
    GIT_TAG master)
FetchContent_MakeAvailable(pigpiohwapi)

FetchContent_Declare(
    logless
    GIT_REPOSITORY https://github.com/therooftopprinz/Logless.git
    GIT_TAG master)
FetchContent_MakeAvailable(logless)

FetchContent_Declare(
    bfc
    GIT_REPOSITORY https://github.com/therooftopprinz/BFC.git
    GIT_TAG master)
FetchContent_MakeAvailable(bfc)

file(GLOB_RECURSE pilora_src src/*.cpp)

add_library(pigpiohwapistubbed STATIC hwapistub/HwApiStub.cpp)
target_link_libraries(pigpiohwapistubbed PRIVATE bfc hwapi logless)
target_include_directories(pigpiohwapistubbed PRIVATE src)

add_executable(pilorastubbed ${pilora_src})
target_include_directories(pilorastubbed PRIVATE src)
target_link_libraries(pilorastubbed PRIVATE hwapi pigpiohwapistubbed logless bfc pthread)
