cmake_minimum_required(VERSION 3.10.0)
project(renderer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_DISABLE_PRECOMPILED_HEADERS ON)

add_executable(renderer src/main.cpp src/screenRender.cpp src/matrices.cpp src/readObj.cpp)

set_property(TARGET renderer PROPERTY CXX_STANDARD 20)

target_include_directories(renderer PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(renderer PRIVATE gdiplus)