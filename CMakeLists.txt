cmake_minimum_required(VERSION 3.12)
project(ABLMediaServer)

set(CMAKE_CXX_STANDARD 11)

# 设置输出目录, 包括 bin, lib 以及其他文件
# Windows 也不再区分 32/64
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release/${SYSTEM_NAME_LOWER}/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH    ${OUTPUT_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})


# Include directories
include_directories(
    ./Linux/boost/include
    ./Linux/ffmpeg-4.3.4/include
    ./rapidjson-master/include
    ./media-server-master/libflv/include
    ./media-server-master/librtmp/include
    ./media-server-master/libmpeg/include
    ./media-server-master/libhls/include
    ./media-server-master/librtp/include
    ./media-server-master/libmov/include
)

# Library directories
link_directories(
    ./bin
    ./Linux/ffmpeg-4.3.4/lib
    ./Linux/faac-1.29.9.2/lib/lib
    ./Linux/boost/lib
)
# 添加源文件夹路径
aux_source_directory(./ABLMediaServer SOURCES)

# 添加具体文件
#list(APPEND SOURCES
#    ./ABLMediaServer-linux/main.cpp
#)

# Add executable
add_executable(ABLMediaServer ${SOURCES})

# Link libraries
target_link_libraries(ABLMediaServer
    pthread
    XHNetSDK
    PsDemux
    PsMux
    RtpDepacket
    RtpPacket
    dl
    boost_system
    boost_thread
    boost_log
    boost_regex
    boost_filesystem
    boost_date_time
    boost_chrono
    boost_atomic
    rtmp
    flv
    mpeg
    hls
    mov
    rtp
    faac
    avcodec
    avutil
    avformat
    swscale
    avfilter
    postproc
    swresample
)
