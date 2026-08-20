cmake_minimum_required(VERSION 3.16)
project(hp_tcp_framework LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HP_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(hpnet STATIC
    src/net/hp_buffer.c
    src/net/hp_event_loop.c
    src/net/hp_server.c
)
target_include_directories(hpnet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(hpnet PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_link_libraries(hpnet PUBLIC pthread)

if(HP_ENABLE_SANITIZERS)
    target_compile_options(hpnet PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(hpnet PUBLIC -fsanitize=address,undefined)
endif()

add_executable(hp_server apps/hp_server.cpp src/protocol/hp_protocol.cpp)
target_include_directories(hp_server PRIVATE include src/protocol)
target_link_libraries(hp_server PRIVATE hpnet)
target_compile_options(hp_server PRIVATE -Wall -Wextra -Werror -Wpedantic)

add_executable(hp_client apps/hp_client.cpp src/protocol/hp_protocol.cpp)
target_include_directories(hp_client PRIVATE include src/protocol)
target_link_libraries(hp_client PRIVATE hpnet)
target_compile_options(hp_client PRIVATE -Wall -Wextra -Werror -Wpedantic)

enable_testing()
add_executable(test_protocol tests/test_protocol.cpp src/protocol/hp_protocol.cpp)
target_include_directories(test_protocol PRIVATE src/protocol)
target_compile_options(test_protocol PRIVATE -Wall -Wextra -Werror -Wpedantic)
add_test(NAME protocol COMMAND test_protocol)
