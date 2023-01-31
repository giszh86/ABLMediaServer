#include "mpeg-ps.h"
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "flv-proto.h"
#include "flv-reader.h"
#include "flv-demuxer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>

#include "rapidjson\document.h"
#include "rapidjson\stringbuffer.h"
#include "rapidjson\writer.h"

#include <string>

using namespace std;
hls_m3u8_t* m3u = NULL  ;
hls_media_t* hls  = NULL  ;

#include "live555Client.h"
CRITICAL_SECTION   hlsLock;

#define  MaxRtspCount   512 

char  ABL_szCurrentPath[256] = { 0 };
int   rtspLoop = 1, LoopTimer = 4;
char  szRtspURLArray[MaxRtspCount][96] = { 0 };
int   nRtspChan[MaxRtspCount] = { 0 };
int   nRtspURLCount = 0;

#ifdef WriteRtspDataFlag
MediaDataHead mediaHead;
FILE*         fMediaFile;
#endif

volatile int nTsFileCount = 0; //TS 切片文件总数 


//获取当前路径
BOOL GetCurrentPath(char *szCurPath)
{
	char    szPath[255] = { 0 };
	string  strTemp;
	int     nPos;

	GetModuleFileName(NULL, szPath, sizeof(szPath));
	strTemp = szPath;

	nPos = strTemp.rfind("\\", strlen(szPath));
	if (nPos >= 0)
	{
		memcpy(szCurPath, szPath, nPos + 1);
		return TRUE;
	}
	else
		return FALSE;
}

//音频，视频回调函数
int nRecvCountV = 0;
int nRecvCountA = 0;
int audioDts = 0;
int videoDts = 0;
static int i = 0;
static int64_t s_dts = -1;

static int hls_handler(void* m3u8, const void* data, size_t bytes, int64_t pts, int64_t dts, int64_t duration)
{
	int discontinue = -1 != s_dts ? 0 : (dts > s_dts + HLS_DURATION / 2 ? 1 : 0);
	s_dts = dts;

	EnterCriticalSection(&hlsLock);

	char name[128] = { 0 };
	snprintf(name, sizeof(name), "%d.ts", i++);
	hls_m3u8_add((hls_m3u8_t*)m3u8, name, pts, duration, discontinue);

	//hls_media_input(hls, STREAM_VIDEO_H264, NULL, 0, 0, 0, 0);
	FILE* fp = fopen(name, "wb");
	if (fp)
	{
		fwrite(data, 1, bytes, fp);
		fclose(fp);
		nTsFileCount ++;
	}

	if (nTsFileCount >= 3 )
	{//重新切片 
 		  char  data[48 * 1024] = { 0 };
		  nTsFileCount = 0;

		  hls_m3u8_playlist(m3u, 1, data, sizeof(data));
		  FILE* fp = fopen("playlist.m3u8", "wb");
		  if (fp)
		  {
			fwrite(data, 1, strlen(data), fp);
			fclose(fp);
		  }/**/

		//audioDts = 0;
		//videoDts = 0;
		//s_dts = -1;

		//hls_m3u8_destroy(m3u);
		//hls_media_destroy(hls);

		m3u = hls_m3u8_create(0, 3);
		hls = hls_media_create(HLS_DURATION * 1000, hls_handler, m3u);
	}
	LeaveCriticalSection(&hlsLock);

	return 0;
}

void CALLBACK live555RTSP_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData)
{
	EnterCriticalSection(&hlsLock);

	if (RtspDataType == XHRtspDataType_Message)
	{//通知消息
		if (strcmp(codeName, "success") == 0)
		{//会收到类似的媒体信息  {"video":"H265","audio":"AAC","channels":1,"sampleRate":16000} ,根据这个进行创建解码播放资源 
			printf("\r\n \r\n ---------rtsp 连接成功，媒体信息为：------------------- \r\n\r\n\r\n\r\n %s  \r\n\r\n\r\n", pAVData);
		}
		else if (strcmp(codeName, "error") == 0)
		{//连接超时
			printf("\r\n \r\n --------- rtsp 连接失败 ------------------- \r\n\r\n\r\n\r\n ");
		}
	}
	else if (RtspDataType == XHRtspDataType_Video)
	{//收到视频 
		if (hls != NULL)
		{
			if (nAVDataLength > 1024 * 64)
				hls_media_input(hls, STREAM_VIDEO_H264, pAVData, nAVDataLength, videoDts, videoDts, HLS_FLAGS_KEYFRAME);
			else
				hls_media_input(hls, STREAM_VIDEO_H264, pAVData, nAVDataLength, videoDts, videoDts, 0);
		
			videoDts += 40;
		}

		if (nRecvCountV % 100 == 0)
		{
 			printf("收到视频 nRtspChan = %d,  RtspDataType =%d,  codeName = %s, nAVDataLength = %d \r\n", nRtspChan, RtspDataType, codeName, nAVDataLength);
		}
	}
	else if (RtspDataType == XHRtspDataType_Audio)
	{//收到音频
		if (hls)
		 hls_media_input(hls, STREAM_AUDIO_AAC, pAVData, nAVDataLength, audioDts, audioDts, 0);

		audioDts += 64;
		
		if (nRecvCountA % 50 == 0)
		{
			printf("收到音频 nRtspChan = %d,  RtspDataType =%d,  codeName = %s, nAVDataLength = %d \r\n", nRtspChan, RtspDataType, codeName, nAVDataLength);
		}
	}

	LeaveCriticalSection(&hlsLock);

	nRecvCountA ++;
	nRecvCountV++;

#ifdef WriteRtspDataFlag
	if (RtspDataType == XHRtspDataType_Video || RtspDataType == XHRtspDataType_Audio)
	{
		mediaHead.mediaDataLength = nAVDataLength;
		if (RtspDataType == XHRtspDataType_Video)
		{
			if (strcmp(codeName, "H264") == 0)
				mediaHead.mediaDataType = MedisDataType_H264;
			else if (strcmp(codeName, "H265") == 0)
				mediaHead.mediaDataType = MedisDataType_H265;
		}
		else if (RtspDataType == XHRtspDataType_Audio)
		{
			if (strcmp(codeName, "G711_A") == 0)
				mediaHead.mediaDataType = MedisDataType_G711A;
			else if (strcmp(codeName, "G711_U") == 0)
				mediaHead.mediaDataType = MedisDataType_G711U;
			else if (strcmp(codeName, "AAC") == 0)
				mediaHead.mediaDataType = MedisDataType_AAC;
		}

		fwrite((char*)&mediaHead, 1, sizeof(mediaHead), fMediaFile);
		fwrite(pAVData, 1, nAVDataLength, fMediaFile);
		fflush(fMediaFile);
	}
#endif
}

bool  bExitRtspThreadFlag = true;
bool  bRunFlag = false;
int CALLBACK OnRtspThread(LPVOID lpVoid)
{
	int i;
	bExitRtspThreadFlag = false;
	bRunFlag = true;
	while (bRunFlag)
	{
		for (i = 0; i < nRtspURLCount; i++)
		{
			live555Client_ConnectCallBack(szRtspURLArray[i], XHRtspURLType_Liveing, true, (void*)NULL, live555RTSP_AudioVideo, nRtspChan[i]);
			Sleep(50);
		}

		Sleep(1000 * LoopTimer);

		for (i = 0; i < nRtspURLCount; i++)
			live555Client_Disconnect(nRtspChan[i]);
	}
	bExitRtspThreadFlag = true;

	return 0;
}

static int flv_handler(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
	hls_media_t* hls = (hls_media_t*)param;

	switch (codec)
	{
	case FLV_AUDIO_AAC:
		return hls_media_input(hls, STREAM_AUDIO_AAC, data, bytes, pts, dts, 0);

	case FLV_AUDIO_MP3:
		return hls_media_input(hls, STREAM_AUDIO_MP3, data, bytes, pts, dts, 0);

	case FLV_VIDEO_H264:
		return hls_media_input(hls, STREAM_VIDEO_H264, data, bytes, pts, dts, flags ? HLS_FLAGS_KEYFRAME : 0);

	case FLV_VIDEO_H265:
		return hls_media_input(hls, STREAM_VIDEO_H265, data, bytes, pts, dts, flags ? HLS_FLAGS_KEYFRAME : 0);

	default:
		// nothing to do
		return 0;
	}
}

void hls_segmenter_flv(const char* file)
{
	 m3u = hls_m3u8_create(0, 3);
	 hls = hls_media_create(HLS_DURATION * 1000, hls_handler, m3u);
	
	 void* flv = flv_reader_create(file);
	 flv_demuxer_t* demuxer = flv_demuxer_create(flv_handler, hls);

	/*int r, type;
	size_t taglen;
	uint32_t timestamp;
	static char data[2 * 1024 * 1024];
	while (1 == flv_reader_read(flv, &type, &timestamp, &taglen, data, sizeof(data)))
	{
		r = flv_demuxer_input(demuxer, type, data, taglen, timestamp);
		assert(0 == r);
	}

	// write m3u8 file
	hls_media_input(hls, STREAM_VIDEO_H264, NULL, 0, 0, 0, 0);
	hls_m3u8_playlist(m3u, 1, data, sizeof(data));
	FILE* fp = fopen("playlist.m3u8", "wb");
    if(fp)
    {
        fwrite(data, 1, strlen(data), fp);
        fclose(fp);
    }

	flv_demuxer_destroy(demuxer);
	flv_reader_destroy(flv);
	hls_media_destroy(hls);
	hls_m3u8_destroy(m3u);*/
}

int main(void)
{
	printf("hello wordl \r\n");
	hls_segmenter_flv("D:\\video\\flv\\H264-2021-05-06.flv");
	
	char* szJonsFileBuffer = new char[1024 * 1024];
	FILE* fFile;
	char  szFileName[256] = { 0 };
	InitializeCriticalSection(&hlsLock);

	GetCurrentPath(ABL_szCurrentPath);
	sprintf(szFileName, "%s%s", ABL_szCurrentPath, "rtspURL.json");
	fFile = fopen(szFileName, "rb");
	if (fFile == NULL)
		return -1;

	memset(szJonsFileBuffer, 0x00, 1024 * 1024);
	fread(szJonsFileBuffer, 1, 48 * 1024, fFile);
	fclose(fFile);

	rapidjson::Document doc;
	doc.Parse<0>((char*)szJonsFileBuffer);
	int listSize;
	if (!doc.HasParseError())
	{
		rtspLoop = doc["rtspLoop"].GetInt64();
		LoopTimer = doc["LoopTimer"].GetInt64();

		listSize = doc["rtspURL"].Size();
		for (int i = 0; i < listSize; i++)
		{
			strcpy(szRtspURLArray[i], doc["rtspURL"][i].GetString());
			if (i >= MaxRtspCount)
				break;
			nRtspURLCount++;
		}
	}
	if (nRtspURLCount == 0)
	{
		delete[] szJonsFileBuffer;
		szJonsFileBuffer = NULL;

		printf("Can't Find RtspURL \r\n");
		return -1;
	}

	live555Client_Init(NULL);

#ifdef WriteRtspDataFlag
	fMediaFile = fopen("F:\\mediaFile2020-10-27.myMP4", "wb");
#endif

	if (rtspLoop == 0)
	{//不循环
		for (int i = 0; i < nRtspURLCount; i++)
		{
			live555Client_ConnectCallBack(szRtspURLArray[i], XHRtspURLType_Liveing, true, (void*)NULL, live555RTSP_AudioVideo, nRtspChan[i]);
		}
	}
	else
	{//循环 
		DWORD dwThread;
		::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnRtspThread, (LPVOID)NULL, 0, &dwThread);
	}

	unsigned char nGetChar;
	while (true)
	{
		nGetChar = getchar();
		if (nGetChar == 'x')
		{
			break;
		}
	}

	delete[] szJonsFileBuffer;
	szJonsFileBuffer = NULL;

	bRunFlag = false;
	while (!bExitRtspThreadFlag)
		Sleep(100);

	for (int i = 0; i < nRtspURLCount; i++)
	{
		live555Client_Disconnect(nRtspChan[i]);
		Sleep(150);
	}
	DeleteCriticalSection(&hlsLock);

#ifdef WriteRtspDataFlag
	fflush(fMediaFile);
	fclose(fMediaFile);
#endif

}