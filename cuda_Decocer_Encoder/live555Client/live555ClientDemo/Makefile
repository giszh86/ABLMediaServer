INCLUDES = -I/home/luoshizhen/boost_1_69_0/boost_1_69_0/
LIBS = -L/home/ABLMediaServer/bin \
      -L/usr/local/cuda-11.7/targets/x86_64-linux/lib/stubs/ \
	  -L//home/CUDA/Video_Codec_SDK_11.1.5/Lib/linux/stubs/x86_64 -llive555Client -lXHNetSDK -lRtpDepacket -lcudaCodecDLL -lcudaEncodeDLL -lcuda -lnvcuvid
  
Demo:Demo.o 
	g++ -std=c++11 -g -o Demo Demo.o $(LIBS) -lpthread 
Demo.o:Demo.cpp
	g++ -std=c++11 -g $(INCLUDES) -c Demo.cpp
clean:
	rm *.o
	rm Demo