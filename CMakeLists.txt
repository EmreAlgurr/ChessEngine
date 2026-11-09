cmake_minimum_required(VERSION 3.16)
project(ChessEngine VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

# Performance & Hardware Optimization Flags (AVX2, Native Architecture)
if(MSVC)
    add_compile_options(/O2 /Oi /Ot /GL /EHsc /arch:AVX2 /W3)
else()
    add_compile_options(-O3 -march=native -mavx2 -flto -Wall -Wextra -pthread)
endif()

file(GLOB SOURCES "src/*.cpp")

add_executable(ChessEngine ${SOURCES})

# Target-specific AVX2 definition
target_compile_definitions(ChessEngine PRIVATE USE_AVX2=1)

message(STATUS "Configured ChessEngine with AVX2 NNUE Hardware Acceleration.")
