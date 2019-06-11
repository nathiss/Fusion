cmake_minimum_required(VERSION 3.13)

set(This FusionServer)

project(${This} VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(Headers

)

set(Sources

)

add_subdirectory(googletest)
add_subdirectory(test)

enable_testing()

#target_include_directories(${This} PUBLIC include)
add_library(${This} STATIC ${Sources} ${Headers})
