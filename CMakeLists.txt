cmake_minimum_required(VERSION 3.13)

set(This FusionServerLib)

project(${This} VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(HeadersBase "include/fusion_server")

set(Headers
  ${HeadersBase}/game.hpp
  ${HeadersBase}/http_session.hpp
  ${HeadersBase}/listener.hpp
  ${HeadersBase}/logger_types.hpp
  ${HeadersBase}/player.hpp
  ${HeadersBase}/server.hpp
  ${HeadersBase}/system_abstractions.hpp
  ${HeadersBase}/websocket_session.hpp
  ${HeadersBase}/json.hpp
)

set(SourcesBase "src")

set(Sources
  ${SourcesBase}/game.cpp
  ${SourcesBase}/http_session.cpp
  ${SourcesBase}/listener.cpp
  ${SourcesBase}/server.cpp
  ${SourcesBase}/websocket_session.cpp
  ${SourcesBase}/json.cpp
)

add_subdirectory(third_party/spdlog)

set(JSON_Install OFF CACHE INTERNAL "")  # do not install nlohmann::json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(third_party/nlohmann_json)

add_subdirectory(third_party/googletest)
add_subdirectory(executable)
add_subdirectory(test)

enable_testing()

find_package(Boost 1.67 COMPONENTS REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (Analyse)
  set(CMAKE_LINK_WHAT_YOU_USE TRUE)
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*,-fuchsia-default-arguments,-bugprone-exception-escape")
  set(CMAKE_CXX_CPPLINT "cpplint;--linelength=120;--quiet;--filter=-readability/nolint")
  set(CMAKE_CXX_CPPCHECK "cppcheck;\
    --enable=warning,performance,portability,information,missingInclude;\
    --std=c++11;\
    --verbose;\
    --quiet;\
    --template=\"[{severity}][{id}] {message} {callstack} (On {file}:{line})\"")
endif()

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
