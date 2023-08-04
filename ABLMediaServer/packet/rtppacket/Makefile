INCLUDES = -I../boost/include
LIBS = -L../boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic 

libRtpPacket.so:common.o packet.o rtp_packet.o session.o session_manager.o
	g++ -shared -fvisibility=hidden -fPIC -o libRtpPacket.so common.o packet.o rtp_packet.o session.o session_manager.o $(LIBS) 
common.o:common.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c common.cpp  
packet.o:packet.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c packet.cpp  
rtp_packet.o:rtp_packet.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c rtp_packet.cpp  
session.o:session.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c session.cpp  
session_manager.o:session_manager.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c session_manager.cpp  
	
clean:
	rm *.o
	rm libRtpPacket.so