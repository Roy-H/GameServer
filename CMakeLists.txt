cmake_minimum_required(VERSION 3.0)
project(hello_test)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
#include (cmake/utils.cmake)

#ENABLE_LANGUAGE(ASM)

include_directories(.)
include_directories(./include)
include_directories(./src)

set(src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    )

add_executable(hello_test ${src})



#set_target_properties(bin_hello_test PROPERTIES OUTPUT_NAME "hello_test")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

