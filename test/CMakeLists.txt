set(This FusionServerTest)
project(${This} LANGUAGES CXX)

unset(Headers)

set(SourcesBase "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(Sources
  ${SourcesBase}/http_session_test.cpp
  ${SourcesBase}/listener_test.cpp
  ${SourcesBase}/logger_manager_test.cpp
  ${SourcesBase}/abstract_test.cpp
  ${SourcesBase}/player_test.cpp
)

add_executable(${This} ${Sources} ${Headers})

set_target_properties(${This} PROPERTIES
  FOLDER test
)

target_link_libraries(${This} PUBLIC
  PRIVATE gtest_main
  PRIVATE FusionServer
)

add_test(
  NAME ${This}
  COMMAND ${This}
)
