cmake_minimum_required(VERSION 3.12)

project(ABLMediaServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项，包括 -fPIC
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")


# 设置输出目录
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release/${SYSTEM_NAME_LOWER}/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH    ${OUTPUT_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})

# 设置 FFmpeg 版本
set(FFMPEG_VERSION "v6.0")
set(FFMPEG_PATH    /home/admin-nx/ffmpeg_build)
set(FAAC_PATH     ./3rd/faac-1.29.9.2/build)

# 根据 FFmpeg 版本进行不同操作
if(FFMPEG_VERSION STREQUAL "v4.0")
    message("Using FFmpeg v4.0")
elseif(FFMPEG_VERSION STREQUAL "v6.0")
    message("Using FFmpeg v6.0")
    add_definitions(-DFFMPEG6)

else()
    message(FATAL_ERROR "Unsupported FFmpeg version: ${FFMPEG_VERSION}")
endif()


include_directories(${FFMPEG_PATH}/include)

# Include directories
include_directories(
    ./rapidjson-master/include
    ./media-server-master/libflv/include
    ./media-server-master/librtmp/include
    ./media-server-master/libmpeg/include
    ./media-server-master/libhls/include
    ./media-server-master/librtp/include
    ./media-server-master/libmov/include
    ./ABLMediaServer/packet/ps_demux
    ./ABLMediaServer/packet/ps_mux
    ./ABLMediaServer/packet/rtpdepacket
    ./ABLMediaServer/packet/rtppacket
)

# Library directories
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Bin
    ./bin
	${FFMPEG_PATH}/lib
	${FAAC_PATH}/lib
)


# Add executable
add_executable(${PROJECT_NAME})

file(GLOB sources CONFIGURE_DEPENDS 
   ./ABLMediaServer/*.cpp
   ./ABLMediaServer/packet/ps_demux/*.cpp
   ./ABLMediaServer/packet/ps_mux/*.cpp
   ./ABLMediaServer/packet/rtpdepacket/*.cpp
   ./ABLMediaServer/packet/rtppacket/*.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${sources})

# 链接需要的库文件
target_link_libraries(${PROJECT_NAME}
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
