cmake_minimum_required(VERSION 3.10.2)
project(myWebServer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_STANDARD 14)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(myWebServer code/main.cpp code/buffer/buffer.h code/buffer/buffer.cpp code/http/http_request.h code/http/http_request.cpp code/http/http_response.h code/http/http_response.cpp code/http/http_request.cpp code/http/http_request.cpp code/http/http_connect.h code/http/http_connect.cpp code/server/WebServer.h code/server/WebServer.cpp code/server/epoll.h code/server/epoll.cpp)