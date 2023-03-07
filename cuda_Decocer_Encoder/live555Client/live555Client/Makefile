INCLUDES = -I/home/ABLMediaServer/Linux/boost/include
LIBS = -L/home/ABLMediaServer/bin \
       -L/home/ABLMediaServer/Linux/boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic -lXHNetSDK -lRtpDepacket 
 
liblive555Client.so:RtspABLogFile.o NetRecvBase.o NetClientRecvRtsp.o NetClientAddStreamProxy.o NetBaseThreadPool.o RtspABLSipParse.o MediaFifo.o LCbase64.o Base64.o DigestAuthentication.o netBase64.o ourMD5.o strDup.o RtcpPacket.o live555Client.o
	g++ -std=c++11 -g -fPIC -shared -fvisibility=hidden -o liblive555Client.so RtspABLogFile.o NetRecvBase.o NetClientRecvRtsp.o NetClientAddStreamProxy.o NetBaseThreadPool.o RtspABLSipParse.o MediaFifo.o LCbase64.o Base64.o DigestAuthentication.o netBase64.o ourMD5.o strDup.o RtcpPacket.o live555Client.o $(LIBS) -lpthread 
RtspABLogFile.o:RtspABLogFile.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c RtspABLogFile.cpp 
NetRecvBase.o:NetRecvBase.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c NetRecvBase.cpp 
NetClientRecvRtsp.o:NetClientRecvRtsp.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c NetClientRecvRtsp.cpp 
NetClientAddStreamProxy.o:NetClientAddStreamProxy.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c NetClientAddStreamProxy.cpp 
NetBaseThreadPool.o:NetBaseThreadPool.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c NetBaseThreadPool.cpp 
RtspABLSipParse.o:RtspABLSipParse.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c RtspABLSipParse.cpp 
MediaFifo.o:MediaFifo.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c MediaFifo.cpp 
LCbase64.o:LCbase64.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c LCbase64.cpp 
Base64.o:Base64.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c Base64.cpp 
DigestAuthentication.o:DigestAuthentication.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c DigestAuthentication.cpp
netBase64.o:netBase64.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c netBase64.cpp 
ourMD5.o:ourMD5.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c ourMD5.cpp 
strDup.o:strDup.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c strDup.cpp 
RtcpPacket.o:RtcpPacket.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c RtcpPacket.cpp 
live555Client.o:live555Client.cpp
	g++ -std=c++11 -g $(INCLUDES) -fvisibility=hidden -fPIC -c live555Client.cpp 
	
clean:
	rm *.o
	rm liblive555Client.so
