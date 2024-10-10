cmake_minimum_required(VERSION 3.30)
project(calculus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(calculus main.cpp)
target_link_libraries(calculus PRIVATE token differentiate)

add_library(token token.cpp)
target_sources(token PUBLIC token.h)
target_link_libraries(token simplify)

add_library(simplify simplify.cpp)
target_sources(simplify PUBLIC simplify.h)

add_library(differentiate differentiate.cpp)
target_sources(differentiate PUBLIC differentiate.h)
target_link_libraries(differentiate PRIVATE token simplify)