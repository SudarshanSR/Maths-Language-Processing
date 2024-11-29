cmake_minimum_required(VERSION 3.30)
project(mlp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(mlp main.cpp)
target_link_libraries(mlp PRIVATE token)

add_library(token lib/token.cpp)
target_sources(token PUBLIC include/token.h)
target_link_libraries(token PRIVATE constant variable function term terms expression)

add_library(constant lib/constant.cpp)
target_sources(constant PUBLIC include/constant.h)
target_link_libraries(constant PRIVATE token)

add_library(variable lib/variable.cpp)
target_sources(variable PUBLIC include/variable.h)
target_link_libraries(variable PRIVATE token)

add_library(term lib/term.cpp)
target_sources(term PUBLIC include/term.h)
target_link_libraries(term PRIVATE token)

add_library(function lib/function.cpp)
target_sources(function PUBLIC include/function.h)
target_link_libraries(function PRIVATE token)

add_library(terms lib/terms.cpp)
target_sources(terms PUBLIC include/terms.h)
target_link_libraries(terms PRIVATE token)

add_library(expression lib/expression.cpp)
target_sources(expression PUBLIC include/expression.h)
target_link_libraries(expression PRIVATE token)