cmake_minimum_required(VERSION 3.15)
project(getaddrinfo_hw)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -D GOLIKOV -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -D_GLIBCXX_DEBUG")

add_executable(getaddrinfo_hw main.cpp epoll.h epoll.cpp file_descriptor.cpp file_descriptor.h server.cpp server.cpp server.h)