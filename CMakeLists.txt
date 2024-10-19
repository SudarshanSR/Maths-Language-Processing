cmake_minimum_required(VERSION 3.30)
project(mlp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(mlp main.cpp)
target_link_libraries(mlp PRIVATE token)

add_library(token token.cpp dependent.cpp evaluate.cpp simplify.cpp derivative.cpp integral.cpp)
target_sources(token PUBLIC token.h)