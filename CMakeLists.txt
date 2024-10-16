cmake_minimum_required(VERSION 3.30)
project(calculus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(calculus main.cpp)
target_link_libraries(calculus PRIVATE token derivative integral)

add_library(token token.cpp)
target_sources(token PUBLIC token.h)
target_link_libraries(token PRIVATE simplify)

add_library(dependent dependent.cpp)
target_sources(dependent PUBLIC dependent.h)
target_link_libraries(dependent PRIVATE token)

add_library(evaluate evaluate.cpp)
target_sources(evaluate PUBLIC evaluate.h)
target_link_libraries(evaluate PRIVATE token)

add_library(simplify simplify.cpp)
target_sources(simplify PUBLIC simplify.h)
target_link_libraries(simplify PRIVATE token)

add_library(derivative derivative.cpp)
target_sources(derivative PUBLIC derivative.h)
target_link_libraries(derivative PRIVATE token simplify evaluate dependent)

add_library(integral integral.cpp)
target_sources(integral PUBLIC integral.h)
target_link_libraries(integral PRIVATE token simplify evaluate dependent)