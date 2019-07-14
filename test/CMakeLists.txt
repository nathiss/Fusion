cmake_minimum_required(VERSION 3.13)

set(This FusionServerTests)

unset(Headers)

set(Sources
  src/http_session_test.cpp
  src/listener_test.cpp
)

add_executable(${This} ${Sources} ${Headers})

target_link_libraries(${This} PUBLIC
  PRIVATE gtest_main
  PRIVATE FusionServerLib
)

add_test(
  NAME ${This}
  COMMAND ${This}
)
