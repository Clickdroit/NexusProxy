cmake_minimum_required(VERSION 3.20)
project(NexusProxy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/socket_utils.cpp
    src/http_parser.cpp
    src/load_balancer.cpp
    src/health_checker.cpp
    src/proxy_server.cpp
)

add_executable(NexusProxy ${SOURCES})

if(WIN32)
    target_link_libraries(NexusProxy ws2_32)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(NexusProxy PRIVATE /W4)
else()
    target_compile_options(NexusProxy PRIVATE -Wall -Wextra -Wpedantic)
endif()
