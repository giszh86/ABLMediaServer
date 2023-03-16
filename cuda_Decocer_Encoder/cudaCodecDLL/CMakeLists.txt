cmake_minimum_required(VERSION 3.5)
# 项目名称
project(cudaCodecDLL)

# 设置CUDA版本
#find_package(CUDA REQUIRED)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置动态库编译选项
set(BUILD_SHARED_LIBS ON)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# 设置头文件位置
target_include_directories(cudaCodecDLL PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../3rd/Video_Codec_SDK_11.1.5/Interface
    ../../3rd/Samples/NvCodec
)



# 设置源文件位置
set(SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
   ../../3rd/Samples/NvCodec/*.cpp  
)

# 指定编译为动态库
#cuda_add_library(cudaCodecDLL SHARED ${SOURCES})
add_library(cudaCodecDLL SHARED ${SOURCE_FILES})

# 设置依赖库位置
target_link_directories(cudaCodecDLL PRIVATE
    ../../3rd/Samples/Lib/linux/x86_64
)

# 添加需要链接的依赖库
target_link_libraries(cudaCodecDLL PRIVATE
    libnvidia-encode
)

# 链接CUDA库
#target_link_libraries(cudaCodecDLL ${CUDA_LIBRARIES})

# 设置输出路径
set_target_properties(cudaCodecDLL PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ../bin/
)

# 打印编译信息
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build shared libs: ${BUILD_SHARED_LIBS}")