cmake_minimum_required(VERSION 3.13)

set(This FusionServer)

project(${This} VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(Headers
  include/fusion_server/listener.hpp
  include/fusion_server/http_session.hpp
  include/fusion_server/websocket_session.hpp
)

set(Sources
  src/listener.cpp
  src/http_session.cpp
  src/websocket_session.cpp
)

add_subdirectory(googletest)
add_subdirectory(test)

enable_testing()

add_library(${This} STATIC ${Sources} ${Headers})
target_include_directories(${This} PUBLIC include)
