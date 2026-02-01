cmake_minimum_required(VERSION 3.20)
project(Zugzwang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE PROJECT_SOURCES
     CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/src/*.cpp"
     "${PROJECT_SOURCE_DIR}/src/*.hpp"
     "${PROJECT_SOURCE_DIR}/src/*.h"
)

add_executable(Zugzwang ${PROJECT_SOURCES})

target_include_directories(Zugzwang PRIVATE "${PROJECT_SOURCE_DIR}/src")
