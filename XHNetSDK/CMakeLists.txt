cmake_minimum_required(VERSION 3.0)

project(XHNetSDK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 含 当前 文件夹下的头文件目录
include_directories(${CMAKE_SOURCE_DIR})
# 包含 asio 文件夹下的头文件目录
include_directories(${CMAKE_SOURCE_DIR}/asio/include)

# 包含 thread_pool 文件夹下的头文件目录
include_directories(${CMAKE_SOURCE_DIR}/thread_pool)

# 添加当前目录下的所有 .cpp 文件到 SOURCES 列表中
aux_source_directory(${CMAKE_SOURCE_DIR} SOURCES)

# 添加 thread_pool 目录下的所有 .cpp 文件到 SOURCES 列表中
aux_source_directory(${CMAKE_SOURCE_DIR}/thread_pool THREAD_POOL_SOURCES)
list(APPEND SOURCES ${THREAD_POOL_SOURCES})

# 添加 asio/src 目录下的所有 .cpp 文件到 SOURCES 列表中
#aux_source_directory(${CMAKE_SOURCE_DIR}/asio/src ASIO_SOURCES)
#list(APPEND SOURCES ${ASIO_SOURCES})



add_library(XHNetSDK SHARED ${SOURCES})

# 链接当前文件夹下的 asio 头文件目录
target_include_directories(XHNetSDK PRIVATE asio/include)

target_link_libraries(XHNetSDK pthread)

# 设置生成的库文件路径为上一级目录下的 Bin 文件夹
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../Bin)
