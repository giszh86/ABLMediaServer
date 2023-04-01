/*
功能：
       实现http-flv服务器的媒体数据发送功能 
日期    2021-03-29
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHTTP_FLV.h"
#ifdef USE_BOOST
extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL);
#else
extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
std::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL);
#endif

extern CMediaSendThreadPool*           pMediaSendThreadPool;
extern CMediaFifo                      pDisconnectBaseNetFifo; //清理断裂的链接 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                 ABL_MediaServerPort;

//FLV合成回调函数 
static int NetServerHTTP_FLV_MuxerCB(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetServerHTTP_FLV* pHttpFLV = (CNetServerHTTP_FLV*)flv;

	if (!pHttpFLV->bRunFlag)
		return -1;

#ifdef WriteHttp_FlvFileFlag
	if (pHttpFLV)
 		return flv_writer_input(pHttpFLV->flvWrite, type, data, bytes, timestamp);
#else 
	if (pHttpFLV)
		return flv_writer_input(pHttpFLV->flvWrite, type, data, bytes, timestamp);
#endif
}

int  NetServerHTTP_FLV_OnWrite_CB(void* param, const struct flv_vec_t* vec, int n)
{
	CNetServerHTTP_FLV* pHttpFLV = (CNetServerHTTP_FLV*)param;

	if (pHttpFLV != NULL && pHttpFLV->bRunFlag )
	{
		for (int i = 0; i < n; i++)
		{
			pHttpFLV->nWriteRet = XHNetSDK_Write(pHttpFLV->nClient,(unsigned char*)vec[i].ptr, vec[i].len,true);
			if (pHttpFLV->nWriteRet != 0)
			{
				pHttpFLV->nWriteErrorCount ++;//发送出错累计 
				if (pHttpFLV->nWriteErrorCount >= 30)
				{
					pHttpFLV->bRunFlag = false;
					WriteLog(Log_Debug, "NetServerHTTP_FLV_OnWrite_CB 发送失败，次数 nWriteErrorCount = %d ", pHttpFLV->nWriteErrorCount);
					DeleteNetRevcBaseClient(pHttpFLV->nClient);
				}
			}
			else
				pHttpFLV->nWriteErrorCount = 0;//复位 
  		}
	}
	return 0;
}

CNetServerHTTP_FLV::CNetServerHTTP_FLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	bCheckHttpFlvFlag = false;
	strcpy(m_szShareMediaURL, szShareMediaURL);

	MaxNetDataCacheCount = MaxHttp_FlvNetCacheBufferLength;
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//网络数据缓存大小
	bFindFlvNameFlag = false;
	memset(szFlvName, 0x00, sizeof(szFlvName));
	flvMuxer = NULL;

	flvWrite  = NULL ;
	nWriteRet = 0;
	nWriteErrorCount = 0;

	netBaseNetType = NetBaseNetType_HttpFLVServerSendPush ;
	bRunFlag = true;

    nNewAddAudioTimeStamp = 64;
	bUserNewAudioTimeStamp = false;
	nUseNewAddAudioTimeStamp = 0;
	nPrintTime = GetTickCount64();
 
	WriteLog(Log_Debug, "CNetServerHTTP_FLV 构造 = %X, nClient = %llu ",this, nClient);
}

CNetServerHTTP_FLV::~CNetServerHTTP_FLV()
{
	std::lock_guard<std::mutex> lock(NetServerHTTP_FLVLock);

	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 1 nClient = %llu ",this, nClient);

	XHNetSDK_Disconnect(nClient);

	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 2 nClient = %llu ",this, nClient);
	
	if (flvMuxer)
	{
		flv_muxer_destroy(flvMuxer);
		flvMuxer = NULL;
	}
	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 3 nClient = %llu ",this, nClient);

	if (flvWrite)
	{
		flv_writer_destroy(flvWrite);
		flvWrite = NULL;
	}
	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 4 nClient = %llu ",this, nClient);

	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
	
	WriteLog(Log_Debug, "CNetServerHTTP_FLV 析构 =%X szFlvName = %s, nClient = %llu \r\n", this, szFlvName, nClient);
	
	malloc_trim(0);
}

int CNetServerHTTP_FLV::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;
	m_videoFifo.push(pVideoData, nDataLength);
	return 0 ;
}

int CNetServerHTTP_FLV::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	if (strcmp(szAudioCodec, "AAC") != 0 )
		return 0;

	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

void  CNetServerHTTP_FLV::MuxerVideoFlV(char* codeName, unsigned char* pVideo, int nLength)
{
	//if (dwVideoFirstTime == 0)
	//	dwVideoFirstTime = GetTickCount();
	//flvPS = GetTickCount() - dwVideoFirstTime;

	if (nVideoStampAdd == 0)
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	if (strcmp(codeName, "H264") == 0)
	{
		if (flvMuxer)
			flv_muxer_avc(flvMuxer, pVideo, nLength, flvPS, flvPS);
	}
	else if (strcmp(codeName, "H265") == 0)
	{
		if (flvMuxer)
			flv_muxer_hevc(flvMuxer, pVideo, nLength, flvPS, flvPS);
	}

	//printf("flvPS = %d \r\n", flvPS);
	flvPS += nVideoStampAdd;
}

void  CNetServerHTTP_FLV::MuxerAudioFlV(char* codeName, unsigned char* pAudio, int nLength)
{
	//if (dwAudioFirstTime == 0)
	//	dwAudioFirstTime = GetTickCount();
	//flvAACDts = GetTickCount() - dwAudioFirstTime;
	if (ABL_MediaServerPort.nEnableAudio == 0)
		return;

	if (nAsyncAudioStamp == -1)
		nAsyncAudioStamp = GetTickCount();

	if (strcmp(codeName, "AAC") == 0)
	{
		if (flvMuxer)
			flv_muxer_aac(flvMuxer, pAudio, nLength, flvAACDts, flvAACDts);

		if(bUserNewAudioTimeStamp == false)
		  flvAACDts += mediaCodecInfo.nBaseAddAudioTimeStamp ;
		else
		{
			nUseNewAddAudioTimeStamp --;
			flvAACDts += nNewAddAudioTimeStamp;
			if (nUseNewAddAudioTimeStamp <= 0)
			{
				bUserNewAudioTimeStamp = false;
			}
		}
		//AAC 的时间增量计算 ，以海康的16K采样为例，AAC每1024字节编码一次，那么(1024 / 16000 * 2) * 1000 = 32 毫秒 ，但是海康往往2帧发送一次 ，那么两帧递增 32 * 2 = 64 毫秒 ，64 就是海康摄像头 DTS ,PTS 的增量 
		// 以大华的8K采样为例，AAC每1024字节编码一次，那么(1024 / 8000 * 2) * 1000 = 64 毫秒 ，但是大华往往2帧发送一次 ，那么两帧递增 64 * 2 = 128 毫秒 ，128 就是大华摄像头 DTS ,PTS 的增量 
	}
 
	//同步音视频 
	SyncVideoAudioTimestamp();
}

int CNetServerHTTP_FLV::SendVideo()
{
	std::lock_guard<std::mutex> lock(NetServerHTTP_FLVLock);
	
	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ",nClient);
		DeleteNetRevcBaseClient(nClient);
 		return -1;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if((pData = m_videoFifo.pop(&nLength)) != NULL )
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
			MuxerVideoFlV(mediaCodecInfo.szVideoName, pData, nLength);
		else
			MuxerVideoFlV(mediaCodecInfo.szVideoName, pData+4, nLength-4);
		m_videoFifo.pop_front();
	}

	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ", nClient);
		DeleteNetRevcBaseClient(nClient);
	}

	return 0;
}

int CNetServerHTTP_FLV::SendAudio()
{
	std::lock_guard<std::mutex> lock(NetServerHTTP_FLVLock);
	
	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ", nClient);
		DeleteNetRevcBaseClient(nClient);
		return -1;
	}

	//不是AAC
	if (strcmp(mediaCodecInfo.szAudioName, "AAC") != 0)
		return -1;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
			MuxerAudioFlV(mediaCodecInfo.szAudioName, pData, nLength);
		else
			MuxerAudioFlV(mediaCodecInfo.szAudioName, pData + 4, nLength - 4);

		m_audioFifo.pop_front();
  	}
	if (nWriteErrorCount >= 30)
	{
		DeleteNetRevcBaseClient(nClient);
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ", nClient);
	}

	return 0;
}

int CNetServerHTTP_FLV::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
	{//剩余空间足够
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//剩余空间不够，需要把剩余的buffer往前移动
		if (netDataCacheLength > 0)
		{//如果有少量剩余
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
				DeleteNetRevcBaseClient(nClient);
				return 0;
			}
 		}
		else
		{//没有剩余，那么 首，尾指针都要复位 
			nNetStart = 0;
			nNetEnd = 0;
			netDataCacheLength = 0;
 		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}

	WriteLog(Log_Debug, "InputNetData() ... ");

	return true;
}

int CNetServerHTTP_FLV::ProcessNetData()
{
	if (!bFindFlvNameFlag)
	{
		if (strstr((char*)netDataCache, "\r\n\r\n") == NULL)
		{
			WriteLog(Log_Debug, "数据尚未接收完整 ");
			if (memcmp(netDataCache, "GET ", 4) != 0)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X , nClient = %llu , 接收的数据非法 ", this, nClient);
				DeleteNetRevcBaseClient(nClient);
			}
			return -1;
 		}
	}

	if (!bCheckHttpFlvFlag)
	{
		bCheckHttpFlvFlag = true;

		//把请求的FLV文件读取出来
		char    szTempName[512] = { 0 };
		string  strHttpHead = (char*)netDataCache;
		int     nPos1, nPos2;
		nPos1 = strHttpHead.find("GET ", 0);
		if (nPos1 >= 0)
		{
			nPos2 = strHttpHead.find(" HTTP/", 0);
			if (nPos2 > 0)
			{
				bFindFlvNameFlag = true;
				memset(szTempName, 0x00, sizeof(szTempName));
				memcpy(szTempName, netDataCache + nPos1 + 4, nPos2 - nPos1 - 4);

				string strFlvName = szTempName;
				nPos2 = strFlvName.find("?", 0);
				if (nPos2 > 0)
				{//有？，需要去掉？后面的字符串 
					if(strlen(szPlayParams) == 0)//拷贝鉴权参数
					  memcpy(szPlayParams, szTempName + (nPos2 + 1), strlen(szTempName) - nPos2 - 1);

					memset(szFlvName, 0x00, sizeof(szFlvName));
					memcpy(szFlvName, szTempName, nPos2);
				}
				else//没有？，直接拷贝 
					strcpy(szFlvName, szTempName);
				WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X ,nClient = %llu ,拷贝出FLV 文件名字 %s ", this, nClient, szFlvName);
			}
		}

		if (!bFindFlvNameFlag)
		{
			WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X, 检测出 非法的 Http-flv 协议数据包 nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		//根据FLV文件，进行简单判断是否合法
		if (!(strstr(szFlvName, ".flv") != NULL || strstr(szFlvName, ".FLV") != NULL))
		{
			WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X,  nClient = %llu , 请求的名字非法 %s ", this, nClient, szFlvName);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		//根据FLV文件，进行查找推流对象 
		if (strstr(szFlvName, ".flv") != NULL || strstr(szFlvName, ".FLV") != NULL)
 			szFlvName[strlen(szFlvName) - 4] = 0x00;

		strcpy(szMediaSourceURL, szFlvName);
#ifdef USE_BOOST
		boost::shared_ptr<CMediaStreamSource> pushClient = NULL;
#else
		std::shared_ptr<CMediaStreamSource> pushClient = NULL;
#endif

		if (strstr(szFlvName, RecordFileReplaySplitter) == NULL)
		{//实况点播
		     pushClient = GetMediaStreamSource(szFlvName);
			if (pushClient == NULL)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X, 没有推流对象的地址 %s nClient = %llu ", this, szFlvName, nClient);

				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}
		else
		{//录像点播
		    //查询点播的录像是否存在
			if (QueryRecordFileIsExiting(szFlvName) == false)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X, 没有点播的录像文件 %s nClient = %llu ", this, szFlvName, nClient);
 				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
				DeleteNetRevcBaseClient(nClient);
				return -1;
 			}

			//创建录像文件点播
			pushClient = CreateReplayClient(szFlvName, &nReplayClient);
			if (pushClient == NULL)
			{
 				WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X, 建录像文件点播失败 %s nClient = %llu ", this, szFlvName, nClient);
 				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}

		//记下媒体源
		SplitterAppStream(szFlvName);
		sprintf(m_addStreamProxyStruct.url, "http://localhost:%d/%s/%s.flv", ABL_MediaServerPort.nHttpFlvPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
 
		char szOrigin[256] = { 0 };
		flvParse.ParseSipString((char*)netDataCache);
		flvParse.GetFieldValue("Origin", szOrigin);
		if (strlen(szOrigin) == 0)
			strcpy(szOrigin, "*");

		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: keep-alive\r\nContent-Type: video/x-flv; charset=utf-8\r\nDate: Wed, Apr 20 2022 10:04:31 GMT\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin, MediaServerVerson);
		XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
		if (nWriteRet != 0)
		{
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

 		flvMuxer = flv_muxer_create(NetServerHTTP_FLV_MuxerCB, this);
		if (flvMuxer == NULL)
		{
			WriteLog(Log_Debug, "创建 flv 打包器失败 ");
			DeleteNetRevcBaseClient(nClient);
			return -1;
 		}

#ifdef WriteHttp_FlvFileFlag //写入FLV文件
		char  szWriteFlvName[256] = { 0 };
		sprintf(szWriteFlvName, ".\\%X_%llu.flv", this, nClient);
		flvWrite = flv_writer_create(szWriteFlvName);
#else //通过网络传输 
		if ((strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H265") == 0) &&
			strcmp(pushClient->m_mediaCodecInfo.szAudioName, "AAC") == 0 && ABL_MediaServerPort.nEnableAudio == 1)
		{//H264、H265  && AAC，创建音频，视频
		  flvWrite = flv_writer_create2(1, 1, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
		  WriteLog(Log_Debug, "创建http-flv 输出格式为： 视频 %s、音频 %s  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, pushClient->m_mediaCodecInfo.szAudioName, nClient);
		}
		else if ((strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H265") == 0) &&
			strcmp(pushClient->m_mediaCodecInfo.szAudioName, "AAC") != 0 )
		{//H264、H265 只创建视频
			flvWrite = flv_writer_create2(0, 1, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
			WriteLog(Log_Debug, "创建http-flv 输出格式为： 视频 %s、音频：无音频  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, nClient);
		}
		else
		{
			WriteLog(Log_Debug, "视频 %s、音频 %s 格式有误，不支持http-flv 输出,即将删除 nClient = %llu ",pushClient->m_mediaCodecInfo.szVideoName,pushClient->m_mediaCodecInfo.szAudioName,nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		if (flvWrite == NULL)
		{
			WriteLog(Log_Debug, "创建 flv 发送组件失败 ");
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
#endif

		m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

		//把客户端 加入源流媒体拷贝队列 
		pushClient->AddClientToMap(nClient);

		//把客户端 加入到发送线程池中
		pMediaSendThreadPool->AddClientToThreadPool(nClient);
	}
  
	return 0;
}

//发送第一个请求
int CNetServerHTTP_FLV::SendFirstRequst()
{
	return 0;
}

//请求m3u8文件
bool  CNetServerHTTP_FLV::RequestM3u8File()
{
	return true;
}