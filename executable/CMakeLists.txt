cmake_minimum_required(VERSION 3.13)

set(This FusionServerExecutable)

project(${This} VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# To remove the propagated `Headers` from parent directory.
unset(Headers)

set(Sources
  src/main.cpp
)

add_executable(${This} ${Sources} ${Headers})

target_link_libraries(${This}
  PRIVATE FusionServerLib
)