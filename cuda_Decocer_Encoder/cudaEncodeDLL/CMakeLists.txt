# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.5)

# 设置项目名称和版本号
project(cudaEncodeDLL VERSION 1.0)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 定义宏定义，根据操作系统平台添加 _WIN32 宏定义
if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32)
endif()

# 设置CUDA版本
#find_package(CUDA REQUIRED)

# 设置动态库编译选项
set(BUILD_SHARED_LIBS ON)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_FILES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/NvEncoder SOURCE_FILES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/Samples/NvCodec/ SOURCE_FILES)

# 指定编译为动态库
#cuda_add_library(cudaCodecDLL SHARED ${SOURCES})
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# 设置头文件位置
target_include_directories(${PROJECT_NAME} PRIVATE
	 ${CMAKE_CURRENT_SOURCE_DIR}/NvEncoder
    ../../3rd/Video_Codec_SDK_11.1.5
	../../3rd/Video_Codec_SDK_11.1.5/Interface
	../../3rd/Video_Codec_SDK_11.1.5/Samples
	../../3rd/Video_Codec_SDK_11.1.5/Samples/NvCodec
)

# 设置头文件位置 暴露给其他项目
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置依赖库位置
target_link_directories(${PROJECT_NAME} INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/Video_Codec_SDK_11.1.5/Lib/linux/stubs/x86_64
)

find_library(LIBNVENCODE nvidia-encode HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/Video_Codec_SDK_11.1.5/Lib/linux/stubs/x86_64)
find_library(LIBNVDEC nvcuvid HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/Video_Codec_SDK_11.1.5/Lib/linux/stubs/x86_64)
find_library(LIBCUDA cuda HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/Video_Codec_SDK_11.1.5/Lib/linux/stubs/x86_64)
# 查找需要链接的库
#find_library(LIBNVENCODE nvidia-encode)
#find_library(LIBNVDEC nvcuvid)
#find_library(LIBCUDA cuda)

# 添加需要链接的依赖库
target_link_libraries(${PROJECT_NAME} PRIVATE 
pthread
${LIBNVENCODE} 
${LIBNVDEC}
${LIBCUDA}
)

# 链接CUDA库
#target_link_libraries(cudaCodecDLL ${CUDA_LIBRARIES})

# 设置C++标准
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)


# 设置输出路径，建议使用 set_target_properties 替代
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin
)

# 打印编译信息
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build shared libs: ${BUILD_SHARED_LIBS}")