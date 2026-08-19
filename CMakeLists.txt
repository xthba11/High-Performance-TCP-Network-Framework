cmake_minimum_required(VERSION 3.16)
project(HighPerformanceTCPNetworkFramework VERSION 0.1.0 LANGUAGES CXX)

include(cmake/options.cmake)
include(cmake/sanitizers.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(hello src/hello.cc)
target_compile_options(hello PRIVATE
  $<$<CONFIG:Debug>:-g -O0>
  $<$<CONFIG:Release>:-O2 -DNDEBUG>
)
hp_tcp_enable_sanitizers(hello)

if(BUILD_TESTS)
  include(CTest)
  add_test(NAME hello_runs COMMAND hello)
endif()
