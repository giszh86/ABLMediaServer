INCLUDES = -I../Video_Codec_SDK_11.1.5/Interface \
           -I/usr/local/cuda-11.4/targets/aarch64-linux/include \
           -I/home/ABLMediaServer/Video_Codec_SDK_11.1.5/Samples/NvCodec \
		   -I/home/admin-nx/include
		   
LIBS = -L/usr/local/cuda-11.4/targets/aarch64-linux/lib \
       -L/home/ABLMediaServer/Video_Codec_SDK_11.1.5/Lib/linux/stubs/aarch64 \
       -L/home/ABLMediaServer/boost_1_73_so_ARM -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic -lcuda 
 
libcudaCodecDLL.so:CudaChanManager.o cudaCodecDLL.o CudaVideoDecode.o NvDecoder.o
	g++ -std=c++11 -g -fPIC -shared -fvisibility=hidden -o libcudaCodecDLL.so CudaChanManager.o cudaCodecDLL.o NvDecoder.o CudaVideoDecode.o $(LIBS) -lpthread 
CudaChanManager.o:CudaChanManager.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c CudaChanManager.cpp 
cudaCodecDLL.o:cudaCodecDLL.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c cudaCodecDLL.cpp 
CudaVideoDecode.o:CudaVideoDecode.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c CudaVideoDecode.cpp 
NvDecoder.o:/home/ABLMediaServer/Video_Codec_SDK_11.1.5/Samples/NvCodec/NvDecoder/NvDecoder.cpp 
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c /home/ABLMediaServer/Video_Codec_SDK_11.1.5/Samples/NvCodec/NvDecoder/NvDecoder.cpp 
	
clean:
	rm *.o
	rm libcudaCodecDLL.so