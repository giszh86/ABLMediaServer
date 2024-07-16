FROM ubuntu:20.04


	
# 创建目标目录并将二进制文件复制到目标目录
RUN mkdir -p /opt/media/MediaServer

# 将二进制文件复制到目标目录
ADD ./linux /opt/media/MediaServer

# 设置工作目录
WORKDIR /opt/media/MediaServer

RUN chmod +x ./MediaServer


# 设置库路径环境变量
ENV LD_LIBRARY_PATH=/opt/media/MediaServer:$LD_LIBRARY_PATH

# 指定入口点运行 ABLMediaServer
CMD ./MediaServer