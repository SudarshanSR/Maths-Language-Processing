cmake_minimum_required(VERSION 3.30)
project(calculus)

set(CMAKE_CXX_STANDARD 23)

add_executable(calculus main.cpp)
target_link_libraries(calculus PRIVATE token differentiate)

add_library(token tokenise.cpp)
target_sources(token PUBLIC tokenise.h)

add_library(differentiate differentiate.cpp)
target_sources(differentiate PUBLIC differentiate.h)
target_link_libraries(differentiate PRIVATE token)