// rtspDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "live555Client.h"

#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <map>

#define  MaxRtspCount   512 

char  ABL_szCurrentPath[256] = { 0 };
int   rtspLoop = 1 , LoopTimer = 4;
char  szRtspURLArray[MaxRtspCount][96] = { 0 };
int   nRtspChan[MaxRtspCount] = { 0 };
int   nRtspURLCount = 0 ;

int   nRecordFlag = 0;//是否录像

#define WriteAACFileFlag   1
#ifdef  WriteAACFileFlag
FILE* fileAAC = NULL;
#endif 

void* tsPacketHandle = NULL ;

#ifdef WriteRtspDataFlag
MediaDataHead mediaHead;
FILE*         fMediaFile;
#endif

static void* ts_alloc(void* param, size_t bytes)
{
	static char s_buffer[188];
	assert(bytes <= sizeof(s_buffer));
	return s_buffer;
}

static void ts_free(void* param, void* /*packet*/)
{
	return;
}

static int ts_write(void* param, const void* packet, size_t bytes)
{
	return 1 == fwrite(packet, bytes, 1, (FILE*)param) ? 0 : ferror((FILE*)param);
}

inline const char* ts_type(int type)
{
	switch (type)
	{
	case PSI_STREAM_MP3: return "MP3";
	case PSI_STREAM_AAC: return "AAC";
	case PSI_STREAM_H264: return "H264";
	case PSI_STREAM_H265: return "H265";
	default: return "*";
	}
}

static int ts_stream(void* ts, int codecid)
{
	static std::map<int, int> streams;
	std::map<int, int>::const_iterator it = streams.find(codecid);
	if (streams.end() != it)
		return it->second;

	int i = mpeg_ts_add_stream(ts, codecid, NULL, 0);
	streams[codecid] = i;
	return i;
}

static int on_ts_packet(void* ts, int program, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	printf("[%s] pts: %08lu, dts: %08lu%s\n", ts_type(avtype), (unsigned long)pts, (unsigned long)dts, flags ? " [I]" : "");

	return mpeg_ts_write(ts, ts_stream(ts, avtype), flags, pts, dts, data, bytes);
}


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
int nRecvCount = 0;
int64_t pts = 0;

/*
检测视频是否是I帧
*/
unsigned char szVideoFrameHead[] = { 0x00,0x00,0x00,0x01 };
bool CheckVideoIsIFrame(int nVideoFrameType, unsigned char* szPVideoData, int nPVideoLength)
{
	int nPos = 0;
	bool bVideoIsIFrameFlag = false;
	unsigned char  nFrameType = 0x00;

	for (int i = 0; i < nPVideoLength; i++)
	{
		if (memcmp(szPVideoData + i, szVideoFrameHead, 4) == 0)
		{//找到帧片段
			if (nVideoFrameType == PSI_STREAM_H264)
			{
				nFrameType = (szPVideoData[i + 4] & 0x1F);
				if (nFrameType == 7 || nFrameType == 8 || nFrameType == 5)
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}
			else if (nVideoFrameType == PSI_STREAM_H264)
			{
				nFrameType = (szPVideoData[i + 4] & 0x7E) >> 1;
				if ((nFrameType >= 16 && nFrameType <= 21) || (nFrameType >= 32 && nFrameType <= 34))
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
 					break;
				}
			}
			else
				break;
		}
	}

	return bVideoIsIFrameFlag;
}

int64_t  nVideoOrder = 0;
int64_t  nAudioOrder = 0;
void CALLBACK live555RTSP_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData)
{
	int  avtype;
	int  flags = 0;
	if (RtspDataType == XHRtspDataType_Message)
	{//通知消息
		if (strcmp(codeName, "success") == 0)
		{//会收到类似的媒体信息  {"video":"H265","audio":"AAC","channels":1,"sampleRate":16000} ,根据这个进行创建解码播放资源 
			printf("\r\n \r\n ---------rtsp 连接成功，媒体信息为：------------------- \r\n\r\n\r\n\r\n %s  \r\n\r\n\r\n", pAVData);
		}
		else if (strcmp(codeName,"error") == 0)
		{//连接超时
			printf("\r\n \r\n --------- rtsp 连接失败 ------------------- \r\n\r\n\r\n\r\n ");
		}
	}
	else if (RtspDataType == XHRtspDataType_Video)
	{//收到视频 
		if (nRecordFlag == 1)
		{
			if (strcmp(codeName, "H264") == 0)
				avtype = PSI_STREAM_H264;
			else if (strcmp(codeName, "H265") == 0)
				avtype = PSI_STREAM_H265;

			if (CheckVideoIsIFrame(avtype, pAVData, nAVDataLength) == true)
				flags = 1;
			else
				flags = 0;

			pts = nVideoOrder * 40 * 90;
			mpeg_ts_write(tsPacketHandle, ts_stream(tsPacketHandle, avtype), flags, pts, pts, pAVData, nAVDataLength);
			nVideoOrder++;
		}

		if (nRecvCount % 100 == 0)
		{
			printf("收到视频 nRtspChan = %d,  RtspDataType =%d,  codeName = %s, nAVDataLength = %d \r\n", nRtspChan, RtspDataType, codeName, nAVDataLength);
		}
	}
	else if (RtspDataType == XHRtspDataType_Audio && strcmp(codeName,"AAC") == 0)
	{//收到音频
#ifdef  WriteAACFileFlag
		fwrite(pAVData,1, nAVDataLength,fileAAC);
		fflush(fileAAC);

#endif 		
 		if (nRecordFlag == 1)
		{
		   if (strcmp(codeName, "AAC") == 0)
			  avtype = PSI_STREAM_AAC;

	     	mpeg_ts_write(tsPacketHandle, ts_stream(tsPacketHandle, avtype), 0, nAudioOrder * 64 * 90, nAudioOrder * 64 * 90, pAVData , nAVDataLength);
		    nAudioOrder ++;
		}
 
		if (nRecvCount % 100 == 0)
		{
			printf("收到音频 nRtspChan = %d,  RtspDataType =%d,  codeName = %s, nAVDataLength = %d \r\n", nRtspChan, RtspDataType, codeName, nAVDataLength);
		}
	}

	nRecvCount ++;

#ifdef WriteRtspDataFlag
	if (RtspDataType == XHRtspDataType_Video || RtspDataType == XHRtspDataType_Audio)
	{
		mediaHead.mediaDataLength = nAVDataLength ;
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

int _tmain(int argc, _TCHAR* argv[])
{
	char* szJonsFileBuffer = new char [1024 * 1024];
	FILE* fFile;
	char  szFileName[256] = { 0 };

	GetCurrentPath(ABL_szCurrentPath);
	sprintf(szFileName, "%s%s", ABL_szCurrentPath, "rtspURL.json");
	fFile = fopen(szFileName, "rb");
	if (fFile == NULL)
		return -1;

	memset(szJonsFileBuffer, 0x00, 1024 * 1024);
	fread(szJonsFileBuffer, 1, 48*1024, fFile);
	fclose(fFile);

	rapidjson::Document doc;
	doc.Parse<0>((char*)szJonsFileBuffer);
	int listSize;
	if (!doc.HasParseError())
	{
		rtspLoop = doc["rtspLoop"].GetInt64();
		LoopTimer = doc["LoopTimer"].GetInt64();
		nRecordFlag = doc["Record"].GetInt64();

		listSize = doc["rtspURL"].Size();
		for (int i = 0; i < listSize; i++)
		{
			strcpy(szRtspURLArray[i], doc["rtspURL"][i].GetString());
			if (i >= MaxRtspCount)
				break;
			nRtspURLCount ++;
		}
   	}
	if (nRtspURLCount == 0)
	{
		delete[] szJonsFileBuffer;
		szJonsFileBuffer = NULL;

		printf("Can't Find RtspURL \r\n");
		return -1;
	}

	char  szOutputName[256] = { 0 };
	if (nRecordFlag == 1)
	{
		struct mpeg_ts_func_t tshandler;
		tshandler.alloc = ts_alloc;
		tshandler.write = ts_write;
		tshandler.free = ts_free;

		srand(GetTickCount());
		sprintf(szOutputName, "%s%d.ts", ABL_szCurrentPath, rand());
		FILE* fp = fopen(szOutputName, "wb");
		tsPacketHandle = mpeg_ts_create(&tshandler, fp);
	}

#ifdef  WriteAACFileFlag
	 sprintf(szOutputName,"%s%d.aac", ABL_szCurrentPath, rand());
	 fileAAC = fopen(szOutputName,"wb");
#endif 

	live555Client_Init(NULL);
 
#ifdef WriteRtspDataFlag
     fMediaFile  = fopen("F:\\mediaFile2020-10-27.myMP4","wb");
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

	delete [] szJonsFileBuffer;
	szJonsFileBuffer = NULL;

	bRunFlag = false;
	while (!bExitRtspThreadFlag)
		Sleep(100);

	for (int i = 0; i < nRtspURLCount; i++)
	{
	  live555Client_Disconnect(nRtspChan[i]);
	  Sleep(150);
	}

	if(nRecordFlag == 1)
	  mpeg_ts_destroy(tsPacketHandle);

#ifdef  WriteAACFileFlag
	fclose(fileAAC) ;
#endif 

#ifdef WriteRtspDataFlag
	fflush(fMediaFile);
	fclose(fMediaFile);
#endif

	return 0;
}

