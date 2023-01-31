#include <stdio.h>
#include <stdint.h>
#include <time.h>

#if (defined _WIN32)
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "libnet.h"

#define LISTEN_PORT 40018

void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	void* address);

void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result);

void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

int main(int argc, char** argv)
{
	int32_t ret = 0; 
	printf("Start Run 1....\r\n");
	
	ret = XHNetSDK_Init(1, 4);
	printf("Start Run 2....\r\n");
	if (0 != ret)
	{
		printf("libnet_init failed, error_code: %d\n", ret);
		getchar();

		return -1;
	}
	else
	{
		printf("libnet_init successful\n");
	}

	NETHANDLE srvhandle = 0;

	ret = XHNetSDK_Listen((int8_t*)("0.0.0.0"), LISTEN_PORT, &srvhandle, onaccept, onread, onclose, true);
	if (0 != ret)
	{
		printf("libnet_listen failed, error_code: %d\n", ret);
		goto CLEAN;
	}
	else
	{
		printf("libnet_listen successful, listen_port: %d\n", LISTEN_PORT);
	}

	while (true)
	{
#if (defined _WIN32 || defined _WIN64)
		Sleep(10000);
#else
		sleep(10);
#endif

	}

CLEAN:

	XHNetSDK_Unlisten(srvhandle);
	XHNetSDK_Deinit();
	getchar();

	return 0;
}

void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	void* address)
{
	static int acceptcount = 0;
	
	char host[64] = { 0 };

	if (!address)
	{
		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(address);
		sprintf(host, "%s:%d", ::inet_ntoa(addr->sin_addr), ::ntohs(addr->sin_port));
	}

	printf("a new client[%llu] connected,  host: %s, all_connect£º%d\n", clihandle, host, ++acceptcount);

}

void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result)
{

}

void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address)
{
	static int readcount = 0;

	printf("read client[%llu] data, size: %lu, body: %s, all_read_count: %d\n", 
		clihandle, datasize, reinterpret_cast<char*>(data), ++readcount);

	int32_t ret = XHNetSDK_Write(clihandle, data, datasize, 1);
	if (0 != ret)
	{
		printf("echo client[%llu] data failed, error_code : %d\n", clihandle, ret);
	}
}

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle)
{
	static int closecount = 0;

	printf("client[%llu] closed, all_close_count: %d\n", clihandle, ++closecount);
}