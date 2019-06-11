cmake_minimum_required(VERSION 3.13)

set(This FusionServerTests)

set(Headers

)

set(Sources
  src/connectionTests.cpp
)

add_executable(${This} ${Sources} ${Headers})

target_link_libraries(${This} PUBLIC
  gtest_main
  FusionServer
)

add_test(
  NAME ${This}
  COMMAND ${This}
)