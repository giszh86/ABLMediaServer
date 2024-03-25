cmake_minimum_required(VERSION 3.12)

project(ABLMediaServer)



# 设置编译选项，包括 -fPIC
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -fPIC")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fPIC")


# 设置输出目录
message("当前系统架构: ${CMAKE_SYSTEM_PROCESSOR}")
set(SYSTEM_NAME_LOWER "amd64")   # 自定义系统名称
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")   # 自定义构建类型
endif()

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${SYSTEM_NAME_LOWER}/${CMAKE_BUILD_TYPE})

# 根据架构设置不同的链接目录
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/linux)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/linux)
set(LINK_DIRECTORIES
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
        ./3rd/ffmpeg6/lib/linux86
    )
    
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
  set(LINK_DIRECTORIES
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
        ./3rd/ffmpeg6/lib/arm64
    ) 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/arm64)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/arm64)

endif()






message("OUTPUT_DIR : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

 message("链接目录: ${LINK_DIRECTORIES}")
 
# 设置 FFmpeg 版本
set(FFMPEG_VERSION "v6.0")
set(FAAC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd/faac-1.29.9.2/build)
set(MEDIA_SERVER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/media-server-master/${SYSTEM_NAME_LOWER})


# 根据 FFmpeg 版本进行不同操作
if(FFMPEG_VERSION STREQUAL "v4.0")
    message("Using FFmpeg v4.0")
    include_directories(./3rd/ffmpeg4/include)
elseif(FFMPEG_VERSION STREQUAL "v6.0")
    message("Using FFmpeg v6.0")
    add_definitions(-DFFMPEG6)
    include_directories(./3rd/ffmpeg6/include)
else()
    message(FATAL_ERROR "Unsupported FFmpeg version: ${FFMPEG_VERSION}")
endif()

add_definitions(-DUSE_WVP)


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
     ./ABLMediaServer/
 
)

   

link_directories(${LINK_DIRECTORIES}  )


# 添加子目录，这样子目录中的CMakeLists.txt才会被调用
add_subdirectory(webrtc-streamer)

# 添加子目录，
add_subdirectory(XHNetSDK)

# Add executable
add_executable(${PROJECT_NAME})

file(GLOB ABL_SRC_LIST CONFIGURE_DEPENDS 
   ./ABLMediaServer/*.cpp
    ./ABLMediaServer/packet/ps_demux/*.cpp
    ./ABLMediaServer/packet/ps_mux/*.cpp
    ./ABLMediaServer/packet/rtpdepacket/*.cpp
    ./ABLMediaServer/packet/rtppacket/*.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${ABL_SRC_LIST})

# 链接需要的库文件
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    pthread
    XHNetSDK
    dl
    PsDemux
    PsMux
    RtpDepacket
    RtpPacket
    avcodec
    avutil
    avformat
    swscale
    avfilter
    postproc
    swresample
    webrtc-streamer
    ${MEDIA_SERVER_PATH}/librtmp.a
    ${MEDIA_SERVER_PATH}/libflv.a
	${MEDIA_SERVER_PATH}/libhls.a
	${MEDIA_SERVER_PATH}/libmov.a
	${MEDIA_SERVER_PATH}/libmpeg.a	
	${MEDIA_SERVER_PATH}/librtp.a	
       ${CMAKE_CURRENT_SOURCE_DIR}/3rd/faac-1.29.9.2/bulid/lib/libfaac.so
      
)


