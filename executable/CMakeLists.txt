set(This FusionServerExe)
project(${This} LANGUAGES CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

unset(Headers)

set(SourcesBase "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(Sources
  ${SourcesBase}/main.cpp
)

add_executable(${This} ${Sources} ${Headers})

set_target_properties(${This} PROPERTIES
  FOLDER bin
)

target_link_libraries(${This}
  PRIVATE FusionServer
)