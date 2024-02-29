cmake_minimum_required(VERSION 2.8.4)
project(md5utils)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-std=c++11  -ggdb")

include_directories(.)
#add_executable(md5utils  sqlite3.c md5.c  main.cpp)
add_executable(md5utils  sqlite3.c md5.c http_parser.c file_utils.cpp json11.cpp server.cpp cicd_cmd.cpp db_utils.cpp main.cpp)

target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} dl)