cmake_minimum_required(VERSION 3.20)
project(WebServer)
set(CMAKE_CXX_STANDARD 11)

#添加头文件搜索路径
set(SOCKET_INC ${PROJECT_SOURCE_DIR}/sockets)
set(ERROR_INC ${PROJECT_SOURCE_DIR}/error)

#源文件路径
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/sockets SOCKET_SRC)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/error ERROR_SRC)

add_library(Lib ${SOCKET_SRC} ${SOCKET_INC} ${ERROR_INC} ${ERROR_SRC})

add_executable(server ${PROJECT_SOURCE_DIR}/server.cpp)
target_include_directories(server PRIVATE ${SOCKET_INC} ${ERROR_INC})
target_link_libraries(server Lib)

add_executable(client ${PROJECT_SOURCE_DIR}/client.cpp)
target_include_directories(client PRIVATE ${ERROR_INC} ${SOCKET_INC})
target_link_libraries(client Lib)