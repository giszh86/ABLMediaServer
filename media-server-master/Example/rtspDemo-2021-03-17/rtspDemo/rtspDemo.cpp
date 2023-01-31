// rtspDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "live555Client.h"
#include "mov-writer.h"
#include "mov-format.h"
#include "mpeg4-avc.h"
#include "mpeg4-aac.h"
#include "mpeg4-hevc.h"

#define  MaxRtspCount   512 

char  ABL_szCurrentPath[256] = { 0 };
int   rtspLoop = 1 , LoopTimer = 4;
char  szRtspURLArray[MaxRtspCount][96] = { 0 };
int   nRtspChan[MaxRtspCount] = { 0 };
int   nRtspURLCount = 0 ;

int   nRecordFlag = 0;//是否录像
unsigned char* s_buffer = new unsigned char[1024 * 1024 * 2];
unsigned char s_extra_data[48 * 1024];
   
struct mov_h264_test_t
{
	mov_writer_t* mov;
	struct mpeg4_avc_t avc;
	struct mpeg4_aac_t aac;

	struct mpeg4_hevc_t hevc;

	int track;
	int trackAudio;

	int width;
	int height;
	uint32_t pts, dts;
	const uint8_t* ptr;

	uint32_t ptsAudio, dtsAudio;

	int vcl;
};

#ifdef WriteRtspDataFlag
MediaDataHead mediaHead;
FILE*         fMediaFile;
#endif

struct mov_h264_test_t ctx;
extern "C" const struct mov_buffer_t* mov_file_buffer(void);


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

void CALLBACK live555RTSP_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData)
{
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
			int vcl = 0;
			int update = 0;
			int  n;
			int extra_data_size;

			if(strcmp(codeName,"H264") == 0)
			  n = h264_annexbtomp4(&ctx.avc, pAVData, nAVDataLength, s_buffer, 1024*1024*2, &vcl, &update);
			else if(strcmp(codeName,"H265") == 0)
			  n = h265_annexbtomp4(&ctx.hevc, pAVData, nAVDataLength, s_buffer, 1024 * 1024 * 2, &vcl, &update);

			if (ctx.track < 0)
			{
				if (strcmp(codeName, "H264") == 0)
				{//H264 等待 SPS、PPS 的方法 
					if (ctx.avc.nb_sps < 1 || ctx.avc.nb_pps < 1)
					{
						//ctx->ptr = end;
						return; // waiting for sps/pps
					}
				}
				else if (strcmp(codeName, "H265") == 0)
				{//H265 等待SPS、PPS的方法 
					if (ctx.hevc.numOfArrays < 1)
					{
						//ctx->ptr = end;
						return; // waiting for vps/sps/pps
					}
				}

				if (strcmp(codeName, "H264") == 0)
 				     extra_data_size = mpeg4_avc_decoder_configuration_record_save(&ctx.avc, s_extra_data, sizeof(s_extra_data));
				else if (strcmp(codeName, "H265") == 0)
					extra_data_size = mpeg4_hevc_decoder_configuration_record_save(&ctx.hevc, s_extra_data, sizeof(s_extra_data));

				if (extra_data_size <= 0)
				{
					// invalid AVCC
 					return ;
				}

				// TODO: waiting for key frame ???
				if (strcmp(codeName, "H264") == 0)
				   ctx.track = mov_writer_add_video(ctx.mov, MOV_OBJECT_H264, ctx.width, ctx.height, s_extra_data, extra_data_size);
 				else if (strcmp(codeName, "H265") == 0)
					ctx.track = mov_writer_add_video(ctx.mov, MOV_OBJECT_HEVC, ctx.width, ctx.height, s_extra_data, extra_data_size);
 
				if (ctx.track < 0)
					return ;
			}

 			 mov_writer_write(ctx.mov, ctx.track, s_buffer, n, ctx.pts, ctx.pts, 1 == vcl ? MOV_AV_FLAG_KEYFREAME : 0);
 
			ctx.pts += 40;
			ctx.dts += 40;
		}
		if (nRecvCount % 100 == 0)
		{
			printf("收到视频 nRtspChan = %d,  RtspDataType =%d,  codeName = %s, nAVDataLength = %d \r\n", nRtspChan, RtspDataType, codeName, nAVDataLength);
		}
	}
	else if (RtspDataType == XHRtspDataType_Audio)
	{//收到音频
		int nAACLength;
		if (nRecordFlag == 1)
		{
			if (strcmp(codeName, "AAC") == 0)
			{//AAC
				nAACLength = mpeg4_aac_adts_frame_length(pAVData, nAVDataLength);
				if (nAACLength < 0)
					return;

				if (-1 == ctx.trackAudio)
				{
					uint8_t asc[16];
					mpeg4_aac_adts_load(pAVData, nAVDataLength, &ctx.aac);
					int len = mpeg4_aac_audio_specific_config_save(&ctx.aac, asc, sizeof(asc));

					ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_AAC, ctx.aac.channels, 16, ctx.aac.sampling_frequency, asc, len);
 				}

				mov_writer_write(ctx.mov, ctx.trackAudio, pAVData + 7, nAVDataLength - 7, ctx.ptsAudio, ctx.ptsAudio, 0);
				ctx.ptsAudio += 1024 /*samples*/ * 1000 / ctx.aac.sampling_frequency;
			}
			else if (strcmp(codeName, "G711_A") == 0 || strcmp(codeName, "G711_U") == 0)
			{
				if (-1 == ctx.trackAudio)
				{
					if(strcmp(codeName, "G711_A") == 0)
					  ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_G711a, 1, 16, 8000, NULL, 0);
					else if( strcmp(codeName, "G711_U") == 0)
					  ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_G711u, 1, 16, 8000, NULL, 0);

					if (-1 == ctx.trackAudio)
						return  ;
				}

 				mov_writer_write(ctx.mov, ctx.trackAudio, pAVData, nAVDataLength, ctx.ptsAudio, ctx.ptsAudio, 0);
				ctx.ptsAudio += nAVDataLength / 8;
 			}
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

	live555Client_Init(NULL);
 
#ifdef WriteRtspDataFlag
     fMediaFile  = fopen("F:\\mediaFile2020-10-27.myMP4","wb");
#endif

	 FILE* fp = NULL ;
	 if (nRecordFlag == 1)
	 {
	    memset(&ctx, 0, sizeof(ctx));
	    ctx.track = ctx.trackAudio = -1;
	    ctx.width = 1920;
	    ctx.height = 1080;

		srand(GetTickCount());
	    char  szRecordName[256] = { 0 };
	    sprintf(szRecordName, "%s%d.mp4", ABL_szCurrentPath,rand());
	    fp = fopen(szRecordName, "wb+");
	    ctx.mov = mov_writer_create(mov_file_buffer(), fp, MOV_FLAG_FASTSTART);
 	 }

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

	if (nRecordFlag == 1)
	{
		mov_writer_destroy(ctx.mov);
	    fclose(fp);
	}

	delete[] s_buffer;
	s_buffer = NULL;

#ifdef WriteRtspDataFlag
	fflush(fMediaFile);
	fclose(fMediaFile);
#endif

	return 0;
}

