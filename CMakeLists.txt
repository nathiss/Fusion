cmake_minimum_required(VERSION 3.13)

set(This FusionServer)

project(${This} VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(Headers
  include/nlohmann/json.hpp
  include/fusion_server/system_abstractions.hpp
  include/fusion_server/package_parser.hpp
  include/fusion_server/server.hpp
  include/fusion_server/listener.hpp
  include/fusion_server/http_session.hpp
  include/fusion_server/websocket_session.hpp
  include/fusion_server/game.hpp
  include/fusion_server/player.hpp
)

set(Sources
  src/server.cpp
  src/package_parser.cpp
  src/listener.cpp
  src/http_session.cpp
  src/websocket_session.cpp
  src/game.cpp
  src/main.cpp
)

#add_subdirectory(googletest)
#add_subdirectory(test)
#enable_testing()

add_executable(${This} ${Sources} ${Headers})

target_include_directories(${This} PUBLIC include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${This} Threads::Threads)