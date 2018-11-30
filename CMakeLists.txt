cmake_minimum_required(VERSION 3.10)
project(rdt_with_udp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_executable(rdt_with_udp messages/include/messages.hpp server/include/server.hpp server/include/sr_worker.hpp server/include/utils.hpp server/src/server.cpp server/src/sr_worker.cpp server/src/utils.cpp)