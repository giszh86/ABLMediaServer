INCLUDES = -I/home/ABLMediaServer/Linux/boost/include \
           -I/usr/local/cuda-11.7/targets/x86_64-linux/include \
           -I/home/CUDA/Video_Codec_SDK_11.1.5/Samples/NvCodec \
		   -I/home/CUDA/Video_Codec_SDK_11.1.5/Interface 		   
		   
LIBS = -L/usr/local/cuda-11.7/targets/x86_64-linux/lib/stubs/ \
       -L//home/CUDA/Video_Codec_SDK_11.1.5/Lib/linux/stubs/x86_64 \
       -L/home/ABLMediaServer/Linux/boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic -lcuda -lnvcuvid -lnvidia-encode 
 
libcudaEncodeDLL.so:CudaChanManager.o cudaEncodeDLL.o CudaVideoEncode.o NvEncoder.o NvEncoderCuda.o
	g++ -std=c++11 -g -fPIC -shared -fvisibility=hidden -o libcudaEncodeDLL.so CudaChanManager.o cudaEncodeDLL.o NvEncoder.o NvEncoderCuda.o CudaVideoEncode.o $(LIBS) -lpthread 
CudaChanManager.o:CudaChanManager.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c CudaChanManager.cpp 
cudaEncodeDLL.o:cudaEncodeDLL.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c cudaEncodeDLL.cpp 
CudaVideoEncode.o:CudaVideoEncode.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c CudaVideoEncode.cpp 
NvEncoder.o:/home/CUDA/Video_Codec_SDK_11.1.5/Samples/NvCodec/NvEncoder/NvEncoder.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c /home/CUDA/Video_Codec_SDK_11.1.5/Samples/NvCodec/NvEncoder/NvEncoder.cpp 
NvEncoderCuda.o:/home/CUDA/Video_Codec_SDK_11.1.5/Samples/NvCodec/NvEncoder/NvEncoderCuda.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c /home/CUDA/Video_Codec_SDK_11.1.5/Samples/NvCodec/NvEncoder/NvEncoderCuda.cpp 
	
clean:
	rm *.o
	rm libcudaEncodeDLL.so