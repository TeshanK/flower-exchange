cmake_minimum_required(VERSION 3.22)

project(flower_exchange LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(sep_protocol INTERFACE)
target_include_directories(sep_protocol
        INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

add_library(common INTERFACE)
target_include_directories(common
        INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

add_library(network INTERFACE)
target_include_directories(network
        INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

add_subdirectory(client)
add_subdirectory(server)

enable_testing()
add_subdirectory(tests)