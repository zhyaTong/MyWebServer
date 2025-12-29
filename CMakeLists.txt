cmake_minimum_required(VERSION 3.10)
project(myWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# buffer 模块
add_library(buffer
    code/buffer/buffer.cpp
)

# http 模块
add_library(http
    code/http/http_request.cpp
    code/http/http_response.cpp
    code/http/http_connect.cpp
)
target_link_libraries(http PUBLIC buffer)   #显式依赖

# 线程池模块
add_library(threadpool INTERFACE)
target_include_directories(threadpool
    INTERFACE ${PROJECT_SOURCE_DIR}/code/pool
)

# 定时器模块
add_library(timer
    code/timer/heap_timer.cpp
)

# server 模块
add_library(server
    code/server/WebServer.cpp
    code/server/epoll.cpp
)
target_link_libraries(server 
    PUBLIC http 
    PUBLIC timer
    PUBLIC threadpool)

# 可执行文件
add_executable(myWebServer
    code/main.cpp
)
target_link_libraries(myWebServer PUBLIC server)