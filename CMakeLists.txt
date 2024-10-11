cmake_minimum_required(VERSION 3.30)
project(calculus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(calculus main.cpp)
target_link_libraries(calculus PRIVATE token derivative)

add_library(token token.cpp)
target_sources(token PUBLIC token.h)
target_link_libraries(token simplify)

add_library(simplify simplify.cpp)
target_link_libraries(simplify PRIVATE derivative integral)

add_library(derivative derivative.cpp)
target_link_libraries(derivative PRIVATE token simplify)

add_library(integral integral.cpp)
target_link_libraries(integral PRIVATE token simplify)