cmake_minimum_required(VERSION 3.10)

project(WebServer LANGUAGES CXX)

# ================= 编译设置 =================
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# ================= MySQL 手动配置 =================
# Ubuntu / Debian 默认路径
set(MYSQL_INCLUDE_DIR /usr/include/mysql)
set(MYSQL_LIBRARY mysqlclient)

# ================= buffer =================
add_library(buffer
    code/buffer/buffer.cpp
)

target_include_directories(buffer
    PUBLIC ${PROJECT_SOURCE_DIR}/code/buffer
)

# ================= sqlPool（含 RAII） =================
add_library(sqlPool
    code/pool/sql_connect_pool.cpp
)

target_include_directories(sqlPool
    PUBLIC
        ${PROJECT_SOURCE_DIR}/code/pool
        ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(sqlPool
    PUBLIC ${MYSQL_LIBRARY}
)

# ================= http =================
add_library(http
    code/http/http_request.cpp
    code/http/http_response.cpp
    code/http/http_connect.cpp
)

target_include_directories(http
    PUBLIC ${PROJECT_SOURCE_DIR}/code/http
)

target_link_libraries(http
    PUBLIC buffer
    PUBLIC sqlPool
)

# ================= threadpool（接口库） =================
add_library(threadpool INTERFACE)

target_include_directories(threadpool
    INTERFACE ${PROJECT_SOURCE_DIR}/code/pool
)

# ================= timer =================
add_library(timer
    code/timer/heap_timer.cpp
)

target_include_directories(timer
    PUBLIC ${PROJECT_SOURCE_DIR}/code/timer
)

# ================= server =================
add_library(server
    code/server/WebServer.cpp
    code/server/epoll.cpp
)

target_include_directories(server
    PUBLIC ${PROJECT_SOURCE_DIR}/code/server
)

target_link_libraries(server
    PUBLIC http
    PUBLIC timer
    PUBLIC threadpool
)

# ================= 可执行文件 =================
add_executable(WebServer
    code/main.cpp
)

target_link_libraries(WebServer
    PRIVATE server
)
