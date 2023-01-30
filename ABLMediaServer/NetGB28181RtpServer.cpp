/*
功能：
    负责接收GB28181Rtp码流，包括UDP、TCP模式 
 	 
日期    2021-08-08
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpServer.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern int                                   SampleRateArray[];
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);

void RTP_DEPACKET_CALL_METHOD GB28181_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (!pThis->bRunFlag)
		return;

	if(pThis != NULL)
	{
		if (pThis->nSSRC == 0)
		   pThis->nSSRC = cb->ssrc; //默认第一个ssrc 
	    if(pThis->nSSRC == cb->ssrc )
	       ps_demux_input(pThis->psDeMuxHandle, cb->data, cb->datasize);

	   if (ABL_MediaServerPort.nSaveGB28181Rtp == 1 && pThis->fWritePsFile != NULL && (GetTickCount64() - pThis->nCreateDateTime) < 1000 * 180 )
 	   {
		   fwrite(cb->data,1,cb->datasize,pThis->fWritePsFile);
		   fflush(pThis->fWritePsFile);
	   }
 	}
}

void PS_DEMUX_CALL_METHOD GB28181_RtpRecv_demux_callback(_ps_demux_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (!pThis->bRunFlag)
		return;
 
	if (pThis && cb->streamtype == e_rtpdepkt_st_h264 || cb->streamtype == e_rtpdepkt_st_h265 ||
		cb->streamtype == e_rtpdepkt_st_mpeg4 || cb->streamtype == e_rtpdepkt_st_mjpeg)
	{
		if(pThis->pMediaSource == NULL)
			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->nClient, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
		
		if(pThis->pMediaSource == NULL)
		{
			pThis->bRunFlag = false;
 			DeleteNetRevcBaseClient(pThis->nClient);
			return ;
		}

		if(cb->streamtype == e_rtpdepkt_st_h264)
		   pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
		else if (cb->streamtype == e_rtpdepkt_st_h265)
		   pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//更新视频源的帧速度
			int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(cb->pts, 90000);
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->bUpdateVideoFrameSpeedFlag = true;

				//不管UDP、TCP都设置为码流已经到达 
				boost::shared_ptr<CNetRevcBase>  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;

				if (pThis->pMediaSource)
				{
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//记录是否录像
					pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//记录是否开启hls
				}

				WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
			}
		}
	}
	else 
	{
		if (pThis->pMediaSource == NULL)
		{
			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->nClient, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pMediaSource)
			{
				pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//记录是否录像
				pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//记录是否开启hls
			}
		}

		if (pThis->pMediaSource == NULL)
		{
			pThis->bRunFlag = false;
			DeleteNetRevcBaseClient(pThis->nClient);
			return ;
		}

 		if (cb->streamtype == e_rtpdepkt_st_aac)
		{//aac
			pThis->GetAACAudioInfo(cb->data, cb->datasize);//获取AAC媒体信息
 			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, pThis->mediaCodecInfo.szAudioName,pThis->mediaCodecInfo.nChannels,pThis->mediaCodecInfo.nSampleRate);
		}
		else if (cb->streamtype == e_rtpdepkt_st_g711a)
		{// G711A  
			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "G711_A", 1, 8000);
 		}
		else if (cb->streamtype == e_rtpdepkt_st_g711u)
		{// G711U  
			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "G711_U", 1, 8000);
		}
	}
}

CNetGB28181RtpServer::CNetGB28181RtpServer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nClientRtcp = hParent = 0;
	fWritePsFile = NULL;
	nRtpRtcpPacketType = 0;
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	m_gbPayload = 96;
	hRtpHandle = psDeMuxHandle  = 0;
	memset((char*)&mediaCodecInfo, 0x00, sizeof(mediaCodecInfo));
	bInitFifoFlag = false;
	pMediaSource = NULL; 
	netDataCache = NULL ; //网络数据缓存
	netDataCacheLength = 0;//网络数据缓存大小
	nNetStart = nNetEnd = 0; //网络数据起始位置\结束位置
	MaxNetDataCacheCount = MaxNetDataCacheBufferLength;
	nSendRtcpTime = 0;
	pRtpAddress = NULL;
	bRunFlag = true;
	fWritePsFile  = NULL ;
	pWriteRtpFile = NULL ;

	if (ABL_MediaServerPort.nSaveGB28181Rtp == 1)
	{
		char szVFile[256];
		sprintf(szVFile, "%s%llu_%X.ps", ABL_MediaServerPort.debugPath, nClient, this);
 	    fWritePsFile = fopen(szVFile,"wb") ;
		sprintf(szVFile, "%s%llu_%X.rtp", ABL_MediaServerPort.debugPath, nClient, this);
		pWriteRtpFile = fopen(szVFile, "wb");
	}

 	WriteLog(Log_Debug, "CNetGB28181RtpServer 构造 = %X  nClient = %llu ", this, nClient);
}

CNetGB28181RtpServer::~CNetGB28181RtpServer()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(netDataLock);

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{
		XHNetSDK_DestoryUdp(nClient);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)
	{
		XHNetSDK_Disconnect(nClient);
	}
	if (psDeMuxHandle > 0)
	{
		ps_demux_stop(psDeMuxHandle);
		psDeMuxHandle = 0;
	}

	if (hRtpHandle > 0)
	{
		rtp_depacket_stop(hRtpHandle);
		hRtpHandle = 0;
 	}

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{
	  if(bInitFifoFlag)
	    NetDataFifo.FreeFifo();
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)
	{
	  SAFE_ARRAY_DELETE(netDataCache);
 	}
 
	if (fWritePsFile)
	{
	    fclose(fWritePsFile);
		fWritePsFile = NULL;
	 }
	 if (pWriteRtpFile)
	 {
		 fclose(pWriteRtpFile);
		 pWriteRtpFile = NULL;
	 }
 
	 SAFE_DELETE(pRtpAddress);
 
	 malloc_trim(0);

	 if (hParent > 0)
	 {
		 WriteLog(Log_Debug, "CNetGB28181RtpServer = %X 加入清理国标代理句柄号 hParent = %llu, nClient = %llu ,nMediaClient = %llu", this, hParent, nClient, nMediaClient);
		 pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));
	 }

	 //清理rtcp 
	 if (nClientRtcp > 0)
		 XHNetSDK_DestoryUdp(nClientRtcp); 

	 //最后才删除媒体源
	 if (strlen(m_szShareMediaURL) > 0 && pMediaSource != NULL )
	    DeleteMediaStreamSource(m_szShareMediaURL);

	 WriteLog(Log_Debug, "CNetGB28181RtpServer 析构 = %X  nClient = %llu , hParent= %llu , app = %s ,stream = %s ,bUpdateVideoFrameSpeedFlag = %d ", this, nClient, hParent, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, bUpdateVideoFrameSpeedFlag);
}

int CNetGB28181RtpServer::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetGB28181RtpServer::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetGB28181RtpServer::SendVideo()
{
	return 0;
}

int CNetGB28181RtpServer::SendAudio()
{

	return 0;
}

int CNetGB28181RtpServer::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag)
		return -1;
	std::lock_guard<std::mutex> lock(netDataLock);
 
	//增加保存原始的rtp数据，进行底层分析
	if (ABL_MediaServerPort.nSaveGB28181Rtp == 1 && pWriteRtpFile != NULL && (GetTickCount64() - nCreateDateTime) < 1000 * 300)
	{
		fwrite(pData, 1, nDataLength, pWriteRtpFile);
		fflush(pWriteRtpFile);
	}

	//网络断线检测
	nRecvDataTimerBySecond = 0;

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{//UDP
		if (pRtpAddress == NULL)
		{
			pRtpAddress = new sockaddr_in;
			memcpy((char*)pRtpAddress, (char*)address, sizeof(sockaddr_in));
			unsigned short nPort = ntohs(pRtpAddress->sin_port);
			pRtpAddress->sin_port = htons(nPort + 1);
 		}

		//获取第一个ssrc 
		if (nSSRC == 0 && pData != NULL &&  nDataLength > sizeof(rtpHeader) )
		{
			rtpHeaderPtr = (_rtp_header*)pData;
			nSSRC = rtpHeaderPtr->ssrc;
		}else 
			rtpHeaderPtr = (_rtp_header*)pData;

		if (!bInitFifoFlag)
		{
	        NetDataFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
			bInitFifoFlag = true;
	    }

		if (address && pRtpAddress)
		{//保证只认第一个IP、端口进来的PS码流 
			if (strcmp(inet_ntoa(pRtpAddress->sin_addr), inet_ntoa(((sockaddr_in*)address)->sin_addr)) == 0 &&  //IP 相同
				ntohs(pRtpAddress->sin_port) - 1 == ntohs(((sockaddr_in*)address)->sin_port)  &&  //端口相同
				nSSRC == rtpHeaderPtr->ssrc  // ssrc 相同 
			)
 	         NetDataFifo.push((unsigned char*)pData, nDataLength);
 		}
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)
	{//TCP 
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
					WriteLog(Log_Debug, "CNetGB28181RtpServer = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return 0;
				}
			}
			else
			{//没有剩余，那么 首，尾指针都要复位 
				nNetStart = nNetEnd = netDataCacheLength = 0;
 			}
			memcpy(netDataCache + nNetEnd, pData, nDataLength);
			netDataCacheLength += nDataLength;
			nNetEnd += nDataLength;
		}
 	}

	 return 0;
}

int CNetGB28181RtpServer::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(netDataLock);
	unsigned char* pData = NULL;
	int            nLength;

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{//UDP
		pData = NetDataFifo.pop(&nLength);
		if (pData != NULL)
		{
			if (nLength > 0)
				RtpDepacket(pData, nLength);

			NetDataFifo.pop_front();
		}

		//主动发送rtcp 包
		if (GetTickCount64() - nSendRtcpTime >= 5 * 1000)
		{
			nSendRtcpTime = ::GetTickCount64();

			memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
			rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
			rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

			XHNetSDK_Sendto(nClientRtcp, szRtcpSRBuffer, rtcpSRBufferLength, pRtpAddress);
 		}
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)
	{//TCP 方式的rtp包读取 
		while (netDataCacheLength > 65536 )
		{
			memcpy(rtpHeadOfTCP, netDataCache + nNetStart, 2);
			if ((rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x00) || (rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x01) || (rtpHeadOfTCP[0] == 0x24 ))
			{
			    nNetStart += 2;
				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2; 
				netDataCacheLength -= 4;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 2;//4个字节方式
			}
			else
			{
 				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
			    netDataCacheLength -= 2;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 1;//2个字节方式的
			}

			//获取rtp包的包头
			rtpHeadPtr = (_rtp_header*)(netDataCache + nNetStart);

			nRtpLength = ntohs(nRtpLength);
			if (nRtpLength > 65536 )
			{
				WriteLog(Log_Debug, "CNetGB28181RtpServer = %X rtp包头长度有误  nClient = %llu ,nRtpLength = %llu", this, nClient, nRtpLength);
 				DeleteNetRevcBaseClient(nClient);
				return -1;
			}

			//长度合法 并且是rtp包 （rtpHeadPtr->v == 2) ,防止rtcp数据执行rtp解包
			if(nRtpLength > 0 && rtpHeadPtr->v == 2)
				RtpDepacket(netDataCache + nNetStart, nRtpLength);

			nNetStart += nRtpLength;
			netDataCacheLength -= nRtpLength;
		}

		//主动发送rtcp 包
		if (GetTickCount64() - nSendRtcpTime >= 5 * 1000)
		{
			nSendRtcpTime = ::GetTickCount64();

			memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
			rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
			rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

			ProcessRtcpData(szRtcpSRBuffer, rtcpSRBufferLength, 1);
		}
 	}
 	return 0;
}

//rtp 解包
bool  CNetGB28181RtpServer::RtpDepacket(unsigned char* pData, int nDataLength) 
{
	if (pData == NULL || nDataLength > 65536)
		return false;

	if (hRtpHandle == 0)
	{
		 rtp_depacket_start(GB28181_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);
		 rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_gbps);

		 ps_demux_start(GB28181_RtpRecv_demux_callback, (void*)this, e_ps_dux_timestamp, &psDeMuxHandle);

		 WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ,创建rtp解包 hRtpHandle = %d ,psDeMuxHandle = %d",this, hRtpHandle, psDeMuxHandle);
	}

	if(hRtpHandle > 0)
	  rtp_depacket_input(hRtpHandle, pData, nDataLength);

	return true;
}

//根据AAC音频数据获取AAC媒体信息
void CNetGB28181RtpServer::GetAACAudioInfo(unsigned char* nAudioData, int nLength)
{
	if (mediaCodecInfo.nChannels == 0 && mediaCodecInfo.nSampleRate == 0)
	{
		unsigned char nSampleIndex = 1;
		unsigned char  nChannels = 1;

		nSampleIndex = ((nAudioData[2] & 0x3c) >> 2) & 0x0F;  //从 szAudio[2] 中获取采样频率的序号
		if (nSampleIndex >= 15)
			nSampleIndex = 8;
		mediaCodecInfo.nSampleRate = SampleRateArray[nSampleIndex];

		//通道数量计算 pAVData[2]  中有2个位，在最后2位，根 0x03 与运算，得到两位，左移动2位 ，再 或 上 pAVData[3] 的左边最高2位
		//pAVData[3] 左边最高2位获取方法 先 和 0xc0 与运算，再右移6位，为什么要右移6位？因为这2位是在最高位，所以要往右边移动6位
		nChannels = ((nAudioData[2] & 0x03) << 2) | ((nAudioData[3] & 0xc0) >> 6);
		if (nChannels > 2)
			nChannels = 1;
		mediaCodecInfo.nChannels = nChannels;

		strcpy(mediaCodecInfo.szAudioName, "AAC");

		WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ,获取到国标接入 AAC信息 szAudioName = %s,nChannels = %d ,nSampleRate = %d ", this, mediaCodecInfo.szAudioName, mediaCodecInfo.nChannels, mediaCodecInfo.nSampleRate);
	}
}

//TCP方式发送rtcp包
void  CNetGB28181RtpServer::ProcessRtcpData(unsigned char* szRtcpData, int nDataLength, int nChan)
{
	if (netBaseNetType != NetBaseNetType_NetGB28181RtpServerTCP_Server)
		return;

	if (nRtpRtcpPacketType == 1)
	{//2字节方式
		unsigned short nRtpLen = htons(nDataLength);
		memcpy(szRtcpDataOverTCP, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

		memcpy(szRtcpDataOverTCP + 2, szRtcpData, nDataLength);
		XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 2 , 1);
	}
	else if (nRtpRtcpPacketType == 2)
	{//4
	  szRtcpDataOverTCP[0] = '$';
	  szRtcpDataOverTCP[1] = nChan;
	  unsigned short nRtpLen = htons(nDataLength);
	  memcpy(szRtcpDataOverTCP + 2, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

	  memcpy(szRtcpDataOverTCP + 4, szRtcpData, nDataLength);
	  XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 4, 1);
	}
}

//发送第一个请求
int CNetGB28181RtpServer::SendFirstRequst()
{

	 return 0;
}

//请求m3u8文件
bool  CNetGB28181RtpServer::RequestM3u8File()
{
	return true;
}