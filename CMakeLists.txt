cmake_minimum_required(VERSION 3.0)

set(This FusionServer)
project(${This} LANGUAGES CXX)

find_package(Boost 1.67 COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)

option(FUSION_DOCS "Generate the docs target" ON)
option(FUSION_TEST "Generate the test target" ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(FusionServerRootDir ${PROJECT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${FusionServerRootDir}/cmake" ${CMAKE_MODULE_PATH})

set(HeadersBase "${FusionServerRootDir}/include/fusion_server")
set(Headers
  ${HeadersBase}/game.hpp
  ${HeadersBase}/http_session.hpp
  ${HeadersBase}/listener.hpp
  ${HeadersBase}/server.hpp
  ${HeadersBase}/websocket_session.hpp
  ${HeadersBase}/json.hpp
  ${HeadersBase}/logger_manager.hpp
  ${HeadersBase}/ui/player.hpp
  ${HeadersBase}/ui/player_factory.hpp
  ${HeadersBase}/ui/map.hpp
  ${HeadersBase}/ui/abstract.hpp
  ${HeadersBase}/system/package.hpp
)

set(SourcesBase "${FusionServerRootDir}/src")

set(Sources
  ${SourcesBase}/game.cpp
  ${SourcesBase}/http_session.cpp
  ${SourcesBase}/listener.cpp
  ${SourcesBase}/server.cpp
  ${SourcesBase}/websocket_session.cpp
  ${SourcesBase}/json.cpp
  ${SourcesBase}/logger_manager.cpp
  ${SourcesBase}/player_factory.cpp
)

add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/nlohmann_json)

if (FUSION_TEST)
  add_subdirectory(third_party/googletest)
  add_subdirectory(test)
  enable_testing()
endif()

if (FUSION_DOCS)
  add_subdirectory(docs)
endif()

add_subdirectory(executable)

add_library(${This} STATIC ${Sources} ${Headers})

target_link_libraries(${This}
  PUBLIC nlohmann_json::nlohmann_json
  PUBLIC spdlog::spdlog
  PRIVATE ${Boost_LIBRARIES}
  PRIVATE Threads::Threads
)
target_include_directories(${This}
  PUBLIC include
  PRIVATE ${Boost_INCLUDE_DIR}
)
