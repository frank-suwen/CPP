cmake_minimum_required(VERSION 3.20)
project(udp_epoll_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(udp_echo_server apps/udp_echo_server.cpp)
add_executable(udp_client      apps/udp_client.cpp)

target_compile_options(udp_echo_server PRIVATE -O3 -Wall -Wextra -Wpedantic)
target_compile_options(udp_client      PRIVATE -O3 -Wall -Wextra -Wpedantic)