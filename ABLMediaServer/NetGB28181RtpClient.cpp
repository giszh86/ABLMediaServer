/*
功能：
    负责发送 GB28181 Rtp 码流，包括UDP、TCP模式 
 	 
日期    2021-08-15
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpClient.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;

void PS_MUX_CALL_METHOD GB28181_Send_mux_callback(_ps_mux_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag)
		return;
 
	pThis->GB28181PsToRtPacket(cb->data, cb->datasize);

#ifdef  WriteGB28181PSFileFlag
	fwrite(cb->data,1,cb->datasize,pThis->writePsFile);
	fflush(pThis->writePsFile);
#endif
}

//rtp打包回调视频
void GB28181_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag)
		return;

	if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//udp 直接发送 
		pThis->SendGBRtpPacketUDP(cb->data, cb->datasize);
	}
	else if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{//TCP 需要拼接 包头
		pThis->GB28181SentRtpVideoData(cb->data, cb->datasize);
	}
}

//PS 数据打包成rtp 
void  CNetGB28181RtpClient::GB28181PsToRtPacket(unsigned char* pPsData, int nLength)
{
	if(hRtpPS > 0)
	{
		inputPS.data = pPsData;
		inputPS.datasize = nLength;
		rtp_packet_input(&inputPS);
	}
}

//国标28181PS码流TCP方式发送 
void  CNetGB28181RtpClient::GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength)
{
	if (bRunFlag == false)
		return;
	
	if ((MaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0)
	{//剩余空间不够存储 ,防止出错 
 		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			bRunFlag = false;

			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标RTP码流出错 ，Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != GB28181VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//产生一帧新的视频 
		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标RTP码流出错 ，Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	if (ABL_MediaServerPort.nGBRtpTCPHeadType == 1)
	{//国标 TCP发送 4个字节方式
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 4;
	}
	else if (ABL_MediaServerPort.nGBRtpTCPHeadType == 2)
	{//国标 TCP发送 2 个字节方式
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + nSendRtpVideoMediaBufferLength, (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 2;
	}
	else
	{
		bRunFlag = false;
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 非法的国标TCP包头发送方式(必须为 1、2 )nGBRtpTCPHeadType = %d ", this, ABL_MediaServerPort.nGBRtpTCPHeadType);
		DeleteNetRevcBaseClient(nClient);
	}
}

CNetGB28181RtpClient::CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	psMuxHandle = 0;
	hRtpPS = 0;
	bRunFlag = true;

	nSendRtpVideoMediaBufferLength = 0; //已经积累的长度  视频
	nStartVideoTimestamp           = GB28181VideoStartTimestampFlag ; //上一帧视频初始时间戳 ，
	nCurrentVideoTimestamp         = 0;// 当前帧时间戳

	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

	if (nPort == 0) 
	{//作为udp 使用，当为tcp时， 在SendFirstRequst() 这函数调用 
		
		boost::shared_ptr<CMediaStreamSource> pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
		if (pMediaSource != NULL)
		{
			memcpy((char*)&mediaCodecInfo,(char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));
			pMediaSource->AddClientToMap(nClient);
		}

		//把nClient 加入Video ,audio 发送线程
		pMediaSendThreadPool->AddClientToThreadPool(nClient);
   }

#ifdef  WriteGB28181PSFileFlag
	char    szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.ps", ABL_MediaSeverRunPath,this);
	writePsFile = fopen(szFileName,"wb");
#endif

 	WriteLog(Log_Debug, "CNetGB28181RtpClient 构造 = %X  nClient = %llu ", this, nClient);
}

CNetGB28181RtpClient::~CNetGB28181RtpClient()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{
		XHNetSDK_DestoryUdp(nClient);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{
		XHNetSDK_Disconnect(nClient);
	}
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
	ps_mux_stop(psDeMuxHandle);
	rtp_packet_stop(hRtpPS);

#ifdef  WriteGB28181PSFileFlag
	fclose(writePsFile);
#endif
	WriteLog(Log_Debug, "CNetGB28181RtpClient 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetGB28181RtpClient::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag)
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);
 	
	nRecvDataTimerBySecond = 0 ;

	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	m_videoFifo.push(pVideoData, nDataLength);
	return 0;
}

int CNetGB28181RtpClient::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (!bRunFlag)
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	m_audioFifo.push(pVideoData, nDataLength);
	return 0;
}

int CNetGB28181RtpClient::SendVideo()
{
	//代表交互成功，连接成功
	if (!bUpdateVideoFrameSpeedFlag)
		bUpdateVideoFrameSpeedFlag = true;

	if (psMuxHandle == 0)
	{
		memset(&init, 0, sizeof(init));
		init.cb = (void*)GB28181_Send_mux_callback;
		init.userdata = this;
		init.alignmode = e_psmux_am_4octet;
		init.ttmode = 0;
		init.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
		init.h = &psMuxHandle;
		int32_t ret = ps_mux_start(&init);

		input.handle = psMuxHandle;

		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ，创建 ps 打包成功  ,nClient = %llu,  nRet = %d", this, nClient, ret);
	}

	if (hRtpPS == 0)
	{
		int nRet = rtp_packet_start(GB28181_rtp_packet_callback_func_send, (void*)this, &hRtpPS);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ，创建视频rtp打包失败,nClient = %llu,  nRet = %d", this, nClient, nRet);
			return false;
		}
		optionPS.handle = hRtpPS;
		optionPS.mediatype = e_rtppkt_mt_video;
		optionPS.streamtype = e_rtppkt_st_gb28181;
		optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
		optionPS.payload = atoi(m_startSendRtpStruct.payload);
		optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
		rtp_packet_setsessionopt(&optionPS);

		inputPS.handle = hRtpPS;
		inputPS.ssrc = optionPS.ssrc;

		memset((char*)&gbDstAddr, 0x00, sizeof(gbDstAddr));
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));

		//记下媒体源
		SplitterAppStream(m_szShareMediaURL);
		sprintf(m_addStreamProxyStruct.url, "rtp://localhost/%s/%s", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
		strcpy(szClientIP, m_startSendRtpStruct.dst_url);
		nClientPort = atoi(m_startSendRtpStruct.dst_port);
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		input.mediatype = e_psmux_mt_video;
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			input.streamtype = e_psmux_st_h264;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			input.streamtype = e_psmux_st_h265;
		else
		{
			m_videoFifo.pop_front();
			return 0;
		}

		input.data = pData;
		input.datasize = nLength;
		ps_mux_input(&input);

		m_videoFifo.pop_front();
	}
	return 0;
}

int CNetGB28181RtpClient::SendAudio()
{
	if (psMuxHandle == 0 || ABL_MediaServerPort.nEnableAudio == 0)
		return 0;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		input.mediatype = e_psmux_mt_audio;
		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			input.streamtype = e_psmux_st_aac;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			input.streamtype = e_psmux_st_g711a;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			input.streamtype = e_psmux_st_g711u;
		else
		{
			m_audioFifo.pop_front();
			return 0;
		}

		input.data = pData;
		input.datasize = nLength;
		ps_mux_input(&input);

		m_audioFifo.pop_front();
	}
	return 0;
}

//udp方式发送rtp包
void  CNetGB28181RtpClient::SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength)
{
	XHNetSDK_Sendto(nClient, pRtpData, nLength, (void*)&gbDstAddr);
}

int CNetGB28181RtpClient::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetGB28181RtpClient::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetGB28181RtpClient::SendFirstRequst()
{//当 gb28181 为tcp时，触发该函数 

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{//回复http请求，连接成功，
		sprintf(szResponseBody, "{\"code\":0,\"port\":%d,\"memo\":\"success\",\"key\":%llu}", nReturnPort, nClient);
		ResponseHttp(nClient_http, szResponseBody, false);
	}

	boost::shared_ptr<CMediaStreamSource> pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
	if (pMediaSource != NULL)
	{
		memcpy((char*)&mediaCodecInfo, (char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));
		pMediaSource->AddClientToMap(nClient);
	}

	//把nClient 加入Video ,audio 发送线程
	pMediaSendThreadPool->AddClientToThreadPool(nClient);
	return 0;
}

//请求m3u8文件
bool  CNetGB28181RtpClient::RequestM3u8File()
{
	return true;
}