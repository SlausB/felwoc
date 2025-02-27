cmake_minimum_required(VERSION 3.16)
project(felwoc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-Ofast -mfpmath=both -march=native -m64 -mavx2 -ffast-math)
else()
    add_compile_options(-g)
endif()

add_compile_options(
    -std=gnu++20
    -fconcepts
    -fconcepts-diagnostics-depth=20
    -fdiagnostics-color=always
    -fmax-errors=2
)

# Find dependencies
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS program_options filesystem)
find_package(ZLIB REQUIRED)
find_library(MINIZIP_LIBRARY NAMES minizip REQUIRED)

# Recursively gather all .cpp and .h files
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/*.h)
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/excel_format/Examples.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/output/test/main.cpp)
list(FILTER SOURCES EXCLUDE REGEX "CMakeCXXCompilerId\\.cpp$")

# Define the executable
add_executable(felwoc ${SOURCES})

# Include directories
target_include_directories(felwoc PUBLIC ${CMAKE_SOURCE_DIR})

# Link required libraries
target_link_libraries(felwoc PRIVATE ${Boost_LIBRARIES} ZLIB::ZLIB ${MINIZIP_LIBRARY})
