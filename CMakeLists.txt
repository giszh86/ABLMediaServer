cmake_minimum_required(VERSION 3.12)

project(ABLMediaServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项，包括 -fPIC
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")


# 设置输出目录
message("当前系统架构: ${CMAKE_SYSTEM_PROCESSOR}")
set(SYSTEM_NAME_LOWER "arm64")   # 自定义系统名称
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")   # 自定义构建类型
endif()

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${SYSTEM_NAME_LOWER}/${CMAKE_BUILD_TYPE})
# 设置 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${SYSTEM_NAME_LOWER}/${CMAKE_BUILD_TYPE})

message("OUTPUT_DIR : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# 设置 FFmpeg 版本
set(FFMPEG_VERSION "v6.0")
set(FFMPEG_PATH           /home/admin-nx/ffmpeg_build)
set(FAAC_PATH             ${CMAKE_CURRENT_SOURCE_DIR}/3rd/faac-1.29.9.2/build)
set(MEDIA_SERVER_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/arm64/Release)

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
	${FFMPEG_PATH}/lib
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
    PUBLIC
	pthread
    XHNetSDK
    dl
	rtppacket
	rtpdepacket
	ps_mux
	ps_demux
    avcodec
    avutil
    avformat
    swscale
    avfilter
    postproc
    swresample
	nvmpi
    ${MEDIA_SERVER_PATH}/librtmp.a
	${MEDIA_SERVER_PATH}/libflv.a
	${MEDIA_SERVER_PATH}/libhls.a
	${MEDIA_SERVER_PATH}/libmov.a
	${MEDIA_SERVER_PATH}/libmpeg.a	
	${MEDIA_SERVER_PATH}/librtp.a	
    ${FAAC_PATH}/lib/libfaac.a
)
