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

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)

# Excel format library
add_library(excel_format
    excel_format/BasicExcel.cpp
    excel_format/ExcelFormat.cpp
)
target_include_directories(excel_format PUBLIC ${CMAKE_SOURCE_DIR}/excel_format)

# Output library
add_library(output
    output/messenger.cpp
    output/outputs/console_output.cpp
    output/outputs/file_output.cpp
    output/outputs/string_output.cpp
)
target_include_directories(output PUBLIC ${CMAKE_SOURCE_DIR}/output)

# Targets library
add_library(targets
    targets/utils.cpp
    targets/as3/as3_target.cpp
    targets/ts/ts_target.cpp
    targets/haxe/haxe_target.cpp
)
target_include_directories(targets PUBLIC ${CMAKE_SOURCE_DIR}/targets)
target_link_libraries(targets PRIVATE ${Boost_LIBRARIES} output ast)

# AST library
file(GLOB AST_SOURCES ${CMAKE_SOURCE_DIR}/ast/*.cpp)
add_library(ast ${AST_SOURCES})
target_include_directories(ast PUBLIC ${CMAKE_SOURCE_DIR}/ast)

# PugiXML library
add_library(pugixml xml/pugixml.cpp)
target_include_directories(pugixml PUBLIC ${CMAKE_SOURCE_DIR}/xml)

# Data source library
add_library(data_source
    data_source/data_source.cpp
    data_source/ods_as_xml/ods_as_xml.cpp
)
target_include_directories(data_source PUBLIC ${CMAKE_SOURCE_DIR}/data_source)
target_link_libraries(data_source PRIVATE output pugixml ZLIB::ZLIB)

# Main executable
add_executable(felwoc main.cpp parsing.cpp)
target_link_libraries(felwoc PRIVATE ${Boost_LIBRARIES} excel_format output targets data_source)
