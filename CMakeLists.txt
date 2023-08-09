cmake_minimum_required(VERSION 3.12)

project(ABLMediaServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录, 包括 bin, lib 以及其他文件
# Windows 也不再区分 32/64
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release/${SYSTEM_NAME_LOWER}/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH    ${OUTPUT_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})


# Include directories
include_directories(
    ./3rd/ffmpeg6/include
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
    ./3rd/ffmpeg6/lib
    ./3rd/faac-1.29.9.2/lib/lib
)
# 添加源文件夹路径
aux_source_directory(./ABLMediaServer SOURCES)

# 添加具体文件
#list(APPEND SOURCES
#    ./ABLMediaServer-linux/main.cpp
#)

# 添加 cuda_Decocer_Encoder 子目录
#add_subdirectory(cuda_Decocer_Encoder)

# 添加 XHNetSDK 子目录
#add_subdirectory(XHNetSDK)

# Add executable
add_executable(ABLMediaServer ${SOURCES})

# 设置头文件搜索路径
target_include_directories(ABLMediaServer PRIVATE ${INCLUDES})

# 设置链接的库文件搜索路径
target_link_directories(ABLMediaServer PRIVATE ${LIBS})

# 链接需要的库文件
target_link_libraries(ABLMediaServer PRIVATE ${LIBRARIES})

# Link libraries
target_link_libraries(ABLMediaServer
    pthread
    XHNetSDK
    dl
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
