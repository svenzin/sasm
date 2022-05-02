cmake_minimum_required(VERSION 3.14)

project(sasm)

include_directories(include)
add_subdirectory(src)

include_directories(extern/googletest/googletest/include)
add_subdirectory(extern/googletest)
add_subdirectory(test)
