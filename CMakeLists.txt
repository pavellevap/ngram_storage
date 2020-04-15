cmake_minimum_required(VERSION 3.7)
project(NGramStorage)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wsign-compare -O2")
set(CMAKE_CXX_LINKER_FLAGS "-Wall -Wextra -Wsign-compare -O2")

include_directories(src)

add_subdirectory(src)
add_subdirectory(test)

