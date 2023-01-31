#include "rtmp-client.h"
#include "flv-writer.h"
#include "flv-demuxer.h"
#include "flv-proto.h"
#include <assert.h>
#include <stdio.h>
#include "XHNetSDK.h"
#include <Windows.h>
#include <stdlib.h>

static void* s_flv;
flv_demuxer_t* flvDemuxer;

//#define  UseXHNetSDKFlag  1 //使用自研网络库
FILE* fFile;

static FILE* aac;
static FILE* h264;

inline const char* ftimestamp(uint32_t t, char* buf)
{
	sprintf(buf, "%02u:%02u:%02u.%03u", t / 3600000, (t / 60000) % 60, (t / 1000) % 60, t % 1000);
	return buf;
}

inline size_t get_adts_length(const uint8_t* data, size_t bytes)
{
	assert(bytes >= 6);
	return ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] >> 5) & 0x07);
}

inline char flv_type(int type)
{
	switch (type)
	{
	case FLV_AUDIO_AAC: return 'A';
	case FLV_AUDIO_MP3: return 'M';
	case FLV_AUDIO_ASC: return 'a';
	case FLV_VIDEO_H264: return 'V';
	case FLV_VIDEO_AVCC: return 'v';
	case FLV_VIDEO_H265: return 'H';
	case FLV_VIDEO_HVCC: return 'h';
	default: return '*';
	}
}

static int onFLV(void* /*param*/, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
	static char s_pts[64], s_dts[64];
	static uint32_t v_pts = 0, v_dts = 0;
	static uint32_t a_pts = 0, a_dts = 0;

	printf("[%c] pts: %s, dts: %s, %u, cts: %d, ", flv_type(codec), ftimestamp(pts, s_pts), ftimestamp(dts, s_dts), dts, (int)(pts - dts));

	if (FLV_AUDIO_AAC == codec)
	{
		printf("diff: %03d/%03d", (int)(pts - a_pts), (int)(dts - a_dts));
		a_pts = pts;
		a_dts = dts;

		assert(bytes == get_adts_length((const uint8_t*)data, bytes));
		fwrite(data, bytes, 1, aac);
	}
	else if (FLV_VIDEO_H264 == codec || FLV_VIDEO_H265 == codec)
	{
		printf("diff: %03d/%03d %s", (int)(pts - v_pts), (int)(dts - v_dts), flags ? "[I]" : "");
		v_pts = pts;
		v_dts = dts;

		fwrite(data, bytes, 1, h264);
	}
	else if (FLV_AUDIO_MP3 == codec)
	{
		fwrite(data, bytes, 1, aac);
	}
	else if (FLV_AUDIO_ASC == codec || FLV_VIDEO_AVCC == codec || FLV_VIDEO_HVCC == codec)
	{
		// nothing to do
	}
	else if ((3 << 4) == codec)
	{
		fwrite(data, bytes, 1, aac);
	}
	else
	{
		// nothing to do
		assert(0);
	}

	printf("\n");
	return 0;
}

static int rtmp_client_send(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
#ifndef  UseXHNetSDKFlag
	socket_t* socket = (socket_t*)param;
	socket_bufvec_t vec[2];
	socket_setbufvec(vec, 0, (void*)header, len);
	socket_setbufvec(vec, 1, (void*)data, bytes);
	return socket_send_v_all_by_time(*socket, vec, bytes ? 2 : 1, 0, 2000);
#else 
	NETHANDLE*  socket = (NETHANDLE*)param;
	NETHANDLE   nNetData = *socket;
	int  nRet;
	if (len > 0)
	{
		nRet = XHNetSDK_Write(nNetData, (uint8_t*)header, len, true);
	}
	if (bytes > 0)
	{
		XHNetSDK_Write(nNetData, (uint8_t*)data, bytes, true);
	}
	return 0;
#endif 
}

static int rtmp_client_onaudio(void* /*param*/, const void* data, size_t bytes, uint32_t timestamp)
{
	flv_demuxer_input(flvDemuxer, FLV_TYPE_AUDIO, data, bytes, timestamp);
	return 0;// flv_writer_input(s_flv, FLV_TYPE_AUDIO, data, bytes, timestamp);
}

static int rtmp_client_onvideo(void* /*param*/, const void* data, size_t bytes, uint32_t timestamp)
{
	//if(bytes > 5 +7)
	flv_demuxer_input(flvDemuxer, FLV_TYPE_VIDEO, data, bytes, timestamp);
	return 0;// flv_writer_input(s_flv, FLV_TYPE_VIDEO, data, bytes, timestamp);
}

static int rtmp_client_onscript(void* /*param*/, const void* data, size_t bytes, uint32_t timestamp)
{
	flv_demuxer_input(flvDemuxer, FLV_TYPE_SCRIPT, data, bytes, timestamp);
	//fwrite(data, 1, bytes, fFile);
	return  flv_writer_input(s_flv, FLV_TYPE_SCRIPT, data, bytes, timestamp);
}

// rtmp://live.alivecdn.com/live/hello?key=xxxxxx
// rtmp_publish_aio_test("live.alivecdn.com", "live", "hello?key=xxxxxx", save-to-local-flv-file-name)
void rtmp_play_test(const char* host, const char* app, const char* stream, const char* flv)
{
	static char packet[2 * 1024 * 1024];
	snprintf(packet, sizeof(packet), "rtmp://%s/%s", host, app); // tcurl

#ifndef UseXHNetSDKFlag
	socket_init();
	socket_t socket = socket_connect_host(host, 1939, 2000);
	socket_setnonblock(socket, 0);
#else
	NETHANDLE socket;
	XHNetSDK_Init(2, 8);
	uint32_t ret = XHNetSDK_Connect((int8_t*)(host), 1935, (int8_t*)(NULL), 0, &socket, NULL, NULL, NULL, 1, 5000, 0);
#endif 

	struct rtmp_client_handler_t handler;
	memset(&handler, 0, sizeof(handler));
	handler.send = rtmp_client_send;
	handler.onaudio = rtmp_client_onaudio;
	handler.onvideo = rtmp_client_onvideo;
	handler.onscript = rtmp_client_onscript;

	aac = fopen("d:\\audio.aac", "wb");
	h264 = fopen("d:\\video.264", "wb");

 	 flvDemuxer = flv_demuxer_create(onFLV, NULL);

	fFile = fopen("d:\\rtmpScript.txt", "wb");
#ifndef UseXHNetSDKFlag
	rtmp_client_t* rtmp = rtmp_client_create(app, stream, packet/*tcurl*/, &socket, &handler);
#else 
	rtmp_client_t* rtmp = rtmp_client_create(app, stream, packet/*tcurl*/, &socket, &handler);
#endif 
	s_flv = flv_writer_create(flv);
	int r = rtmp_client_start(rtmp, 1);

#ifndef UseXHNetSDKFlag
	while ((r = socket_recv(socket, packet, sizeof(packet), 0)) > 0)
	{
		assert(0 == rtmp_client_input(rtmp, packet, r));
	}
#else 
	uint32_t nLength = 2 * 1024 * 1024;
	while (true)
		Sleep(100);
	//while ((r = XHNetSDK_Read(socket,(uint8_t*)packet, &nLength, true ,true )) == 0)
	//{
	//	assert(0 == rtmp_client_input(rtmp, packet, nLength));
	//}
#endif 

	rtmp_client_stop(rtmp);
	flv_writer_destroy(s_flv);
	rtmp_client_destroy(rtmp);
	fclose(fFile);
#ifndef UseXHNetSDKFlag
	socket_close(socket);
	socket_cleanup();
#else 
	XHNetSDK_Disconnect(socket);
	XHNetSDK_Deinit();
#endif 

}

int main(void)
{//rtmp://190.15.240.11:1939/live/h264_AAC_Stream1  rtsp://190.15.240.11:1559/Media/Camera_00001
	rtmp_play_test("190.15.240.11", "Media", "Camera_00001", "D:\\getrtmp2.flv");
	while (true)
		Sleep(100);
	return 0;
}