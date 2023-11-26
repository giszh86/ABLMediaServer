# 设置CMake的最低版本要求
cmake_minimum_required(VERSION 3.0.0)

# 设置项目的名称
project(webrtc-streamer)

# 设置编译器选项
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -fPIC")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fPIC")

# 设置默认构建类型
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "未指定构建类型，设置为 '${default_build_type}'。")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "选择构建类型." FORCE)
  # 设置CMake-GUI中可选的构建类型
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# 设置WEBRTC的根目录
set(WEBRTC_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/webrtc/ CACHE STRING "WEBRTC root directory")


# 添加选项，用于选择生成动态库、静态库或可执行文件
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_STATIC_LIBS "Build using static libraries" OFF)
option(BUILD_EXECUTABLE "Build executable" OFF)

# 收集额外的对象文件
FILE(GLOB_RECURSE WEBRTJSONCPPCOBJS ${WEBRTCOBJS}/third_party/jsoncpp/jsoncpp/*${CMAKE_C_OUTPUT_EXTENSION})
FILE(GLOB_RECURSE WEBRTP2POBJ ${WEBRTCOBJS}/p2p/p2p_server_utils/*${CMAKE_C_OUTPUT_EXTENSION})
SET(WEBRTCEXTRAOBJS ${WEBRTJSONCPPCOBJS} ${WEBRTP2POBJ} ${WEBRTTASK})

# 设置项目的目标
FILE(GLOB SOURCE 
  ${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/civetweb/src/*.*
)



# 设置预处理定义
if(WIN32)
  add_definitions(
    NDEBUG
    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    CUDNN_HALF
    CUDNN
    GPU
    _TIMESPEC_DEFINED
    _SCL_SECURE_NO_WARNINGS
    WIN32
    WIN64
    USE_WEBSOCKET
  )
   message("WIN32")
elseif(UNIX AND NOT APPLE)
  add_definitions(
    -DLINUX
    -DUSE_WEBSOCKET
    -DWEBRTC_LINUX
    -DWEBRTC_POSIX  
    -DNO_SSL   
  )
  message("UNIX AND NOT APPLE")
endif()


# 根据选项生成对应的目标
if (BUILD_SHARED_LIBS)
    add_library(${CMAKE_PROJECT_NAME} ${LIB_TYPE} ${SOURCE} ${WEBRTCEXTRAOBJS})
    set(EXTRA_LIBS ${EXTRA_LIBS} ${CMAKE_DL_LIBS})
elseif (BUILD_STATIC_LIBS)
    add_library(${CMAKE_PROJECT_NAME} STATIC ${SOURCE} ${WEBRTCEXTRAOBJS})
else()
    add_executable(${CMAKE_PROJECT_NAME} ${SOURCE} ${WEBRTCEXTRAOBJS})
endif()



target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/
  ${CMAKE_CURRENT_SOURCE_DIR}/capture
    ${CMAKE_CURRENT_SOURCE_DIR}/civetweb/include
  ${WEBRTC_ROOT_PATH}/include
  ${WEBRTC_ROOT_PATH}/include/third_party/abseil-cpp
  ${WEBRTC_ROOT_PATH}/include/third_party/libyuv/include
  ${WEBRTC_ROOT_PATH}/include/third_party/jsoncpp/source/include 
 )


target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
${WEBRTC_ROOT_PATH}/lib/linux86/json.o
${WEBRTC_ROOT_PATH}/lib/linux86/json_reader.o
${WEBRTC_ROOT_PATH}/lib/linux86/json_value.o
${WEBRTC_ROOT_PATH}/lib/linux86/json_writer.o
${WEBRTC_ROOT_PATH}/lib/linux86/libwebrtc.a
) # 手动指定要链接的 webrtc 库


# 其他的链接库，比如Boost和线程库
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    dl
    m
    X11
    pthread
    rt
    atomic
    dl
    glib-2.0
    rt
    z
)

# 如果可用，使用静态链接的stdc++
if (NOT APPLE)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-static-libgcc" CXX_SUPPORTS_STATIC_LIBGCC)
    if (CXX_SUPPORTS_STATIC_LIBGCC)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE -static-libgcc)
    endif()
    CHECK_CXX_COMPILER_FLAG("-static-libstdc++" CXX_SUPPORTS_STATIC_LIBSTDCPP)
    if (CXX_SUPPORTS_STATIC_LIBSTDCPP)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE -static-libstdc++)
    endif()
endif()

# 安装目标和相关文件
# 设置安装路径
set(INSTALL_BIN_DIR      "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "exe和dll的安装路径")
set(INSTALL_LIB_DIR      "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "lib的安装路径")
set(INSTALL_INCLUDE_DIR  "./"           CACHE PATH "头文件的安装路径")
set(INSTALL_CMAKE_DIR    "./"             CACHE PATH "cmake配置的安装路径")


install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION .)
install(DIRECTORY html DESTINATION .)
install(FILES config.json DESTINATION .)
install(FILES Procfile DESTINATION .)

#配置CPack，指定生成的包的格式、系统名称、联系方式等。
SET(CPACK_GENERATOR "ZZC")
SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE})
SET(CPACK_PACKAGE_CONTACT "125388771@qq.com") 
if(PROJECTVERSION)
    SET(CPACK_PACKAGE_VERSION "${PROJECTVERSION}")
endif()
INCLUDE(CPack)
