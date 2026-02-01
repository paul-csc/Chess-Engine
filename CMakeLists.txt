cmake_minimum_required(VERSION 3.10)
project(Zugzwang)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
    src/bitboard.cpp
    src/board.cpp
    src/movegen.cpp
    src/uci.cpp

    src/bitboard.h
    src/board.h
    src/movegen.h
    src/uci.h
    src/types.h
)

add_executable(Zugzwang ${SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

target_include_directories(Zugzwang PRIVATE "${PROJECT_SOURCE_DIR}/src")
