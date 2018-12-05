cmake_minimum_required(VERSION 3.10)
project(rdt_with_udp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_library(messages INTERFACE)
target_include_directories(messages INTERFACE messages/include)

add_executable(server server/src/server.cpp)
target_include_directories(server PRIVATE server/include)
target_link_libraries(server messages)

add_executable(client client/src/client.cpp)
target_include_directories(client PRIVATE client/include)
target_link_libraries(client messages)
