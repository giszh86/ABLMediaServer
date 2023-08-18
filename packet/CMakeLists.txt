cmake_minimum_required(VERSION 3.12)

project(packet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项，包括 -fPIC
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 添加子目录，构建四个库
file(GLOB PS_DEMUX_SOURCES ps_demux/*.cpp)
add_library(ps_demux STATIC ${PS_DEMUX_SOURCES})

file(GLOB PS_MUX_SOURCES ps_mux/*.cpp)
add_library(ps_mux STATIC ${PS_MUX_SOURCES})

file(GLOB RTPDEPACKET_SOURCES rtpdepacket/*.cpp)
add_library(rtpdepacket STATIC ${RTPDEPACKET_SOURCES})

file(GLOB RTPPACKET_SOURCES rtppacket/*.cpp)
add_library(rtppacket STATIC ${RTPPACKET_SOURCES})
