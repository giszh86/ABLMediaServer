INCLUDES = -I/home/boost_1_69_0
LIBS = -L /home/boost_1_69_0/stage/lib -lXHNetSDK

tcp_server:tcp_server.o 
	g++ tcp_server.o -o tcp_server $(LIBS) -lpthread 
tcp_server.o:tcp_server.cpp
	g++ $(INCLUDES) -c tcp_server.cpp
	
clean:
	rm *.o
	rm tcp_server