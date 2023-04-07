# 指定cmake最低版本
cmake_minimum_required(VERSION 3.5)

# 设置项目名称和版本号
project(cudaEncodeDLLDemo VERSION 1.0)

# 定义宏定义，根据操作系统平台添加 _WIN32 宏定义
if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32)
endif()

# 添加C++编译器选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../bin)

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/cudaEncodeDLL)
include_directories(${PROJECT_SOURCE_DIR}/cudaCodecDLL)

# 添加动态库路径
link_directories(${PROJECT_SOURCE_DIR}/../../bin)

# 先执行这个目录下的cmake
# 添加子目录
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cudaEncodeDLL ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/cudaEncodeDLL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cudaCodecDLL ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/cudaCodecDLL)

# 添加可执行文件编译目标
add_executable(${PROJECT_NAME} cudaEncodeDLLDemo.cpp)

# 添加头文件搜索路径
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../cudaEncodeDLL
	${CMAKE_CURRENT_SOURCE_DIR}/../cudaCodecDLL
)

# 添加依赖库搜索路径
target_link_directories(${PROJECT_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/../../bin
)

# 添加需要链接的依赖库
target_link_libraries(${PROJECT_NAME} PRIVATE 
pthread
dl
cudaEncodeDLL
cudaCodecDLL
)
