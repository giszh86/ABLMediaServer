/*
���ܣ�
    �������GB28181Rtp����������UDP��TCPģʽ 
 	 
����    2021-08-08
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpServer.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

extern CMediaSendThreadPool* pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern int                                   SampleRateArray[];
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

extern CMediaSendThreadPool* pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern int                                   SampleRateArray[];
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#endif


void RTP_DEPACKET_CALL_METHOD GB28181_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (!pThis->bRunFlag)
		return;

	if(pThis != NULL)
	{
		if (pThis->pMediaSource == NULL)
		{//���ȴ���ý��Դ,��Ϊ rtp + PS \ rtp + ES \ rtp + XHB ��Ҫʹ�� 
			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->nClient, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pMediaSource != NULL)
				pThis->pMediaSource->netBaseNetType = pThis->netBaseNetType;
		}

		if (pThis->nSSRC == 0)
		   pThis->nSSRC = cb->ssrc; //Ĭ�ϵ�һ��ssrc 
		if (pThis->pMediaSource && pThis->nSSRC == cb->ssrc && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] >= 0x31 && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] <= 0x33 )
		{
 			if (!pThis->bUpdateVideoFrameSpeedFlag && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] >= 0x32 && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] <= 0x33)
			{//������ƵԴ��֡�ٶ�
				int nVideoSpeed = 25;
				if (nVideoSpeed > 0)
				{
					pThis->bUpdateVideoFrameSpeedFlag = true;
  					//����UDP��TCP������Ϊ�����Ѿ����� 

#ifdef USE_BOOST


					boost::shared_ptr<CNetRevcBase>  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
#else

					std::shared_ptr<CNetRevcBase>  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);

#endif
					if (pGB28181Proxy != NULL)
						pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;

					if (pThis->pMediaSource)
					{
						pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
						pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
					}

					WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
					pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
				}
			}

			if (pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x31)
			{//rtp + PS 
				if (ABL_MediaServerPort.gb28181LibraryUse == 1)
				{//����
				  ps_demux_input(pThis->psDeMuxHandle, cb->data, cb->datasize);
				}
				else
				{//�����ϳ�
				   if(pThis->psBeiJingLaoChen)
					 ps_demuxer_input(pThis->psBeiJingLaoChen, cb->data, cb->datasize);
				}
			}
			else if (pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] >= 0x32 && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] <= 0x33)
			{// rtp + ES \ rtp + XHB 
				 if(cb->payload == 98 )
				    pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
				 else if(cb->payload == 99)
				    pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");
			}
  		}

	   if (ABL_MediaServerPort.nSaveGB28181Rtp == 1 && pThis->fWritePsFile != NULL && (GetTickCount64() - pThis->nCreateDateTime) < 1000 * 180 )
 	   {
		   fwrite(cb->data,1,cb->datasize,pThis->fWritePsFile);
		   fflush(pThis->fWritePsFile);
	   }
 	}
}

static int on_gb28181_unpacket(void* param, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)param;
	if (!pThis->bRunFlag)
		return -1;

	if (pThis->pMediaSource == NULL)
	{
		pThis->bRunFlag = false;
		DeleteNetRevcBaseClient(pThis->nClient);
		return -1;
	}

 	if (PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A == avtype || PSI_STREAM_AUDIO_G711U == avtype)
	{
		if (PSI_STREAM_AAC == avtype)
		{//aac
			pThis->GetAACAudioInfo((unsigned char*)data, bytes);//��ȡAACý����Ϣ
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, pThis->mediaCodecInfo.szAudioName, pThis->mediaCodecInfo.nChannels, pThis->mediaCodecInfo.nSampleRate);
		}
		else if (PSI_STREAM_AUDIO_G711A == avtype)
		{// G711A  
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_A", 1, 8000);
		}
		else if (PSI_STREAM_AUDIO_G711U == avtype)
		{// G711U  
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_U", 1, 8000);
		}
	}
	else if (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype || PSI_STREAM_VIDEO_SVAC == avtype)
	{
		if (pThis->m_addStreamProxyStruct.disableVideo[0] == 0x30)
		{//�������û�й��˵���Ƶ
			if (PSI_STREAM_H264 == avtype)
				pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
			else if (PSI_STREAM_H265 == avtype)
				pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
		}
	}

	if (!pThis->bUpdateVideoFrameSpeedFlag)
	{//������ƵԴ��֡�ٶ�
		int nVideoSpeed = 25 ;
		if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
		{
			pThis->bUpdateVideoFrameSpeedFlag = true;
#ifdef USE_BOOST

			//����UDP��TCP������Ϊ�����Ѿ����� 
			boost::shared_ptr<CNetRevcBase>  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
#else
			//����UDP��TCP������Ϊ�����Ѿ����� 
			std::shared_ptr<CNetRevcBase>  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);

#endif
	
			if (pGB28181Proxy != NULL)
				pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;

			if (pThis->pMediaSource)
			{
				pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
				pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
			}

			WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
			pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
		}
	}

	return 0;
}
static void mpeg_ps_dec_testonstream(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}

void PS_DEMUX_CALL_METHOD GB28181_RtpRecv_demux_callback(_ps_demux_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (!pThis->bRunFlag)
		return;
 
	if (pThis && cb->streamtype == e_rtpdepkt_st_h264 || cb->streamtype == e_rtpdepkt_st_h265 ||
		cb->streamtype == e_rtpdepkt_st_mpeg4 || cb->streamtype == e_rtpdepkt_st_mjpeg)
	{
		if (pThis->pMediaSource == NULL)
		{
			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->nClient, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pMediaSource != NULL)
				pThis->pMediaSource->netBaseNetType = pThis->netBaseNetType;
		}
		
		if(pThis->pMediaSource == NULL)
		{
			pThis->bRunFlag = false;
 			DeleteNetRevcBaseClient(pThis->nClient);
			return ;
		}

		if (pThis->m_addStreamProxyStruct.disableVideo[0] == 0x30)
		{//�������û�й��˵���Ƶ
 			if (cb->streamtype == e_rtpdepkt_st_h264)
				pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
			else if (cb->streamtype == e_rtpdepkt_st_h265)
				pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");
		}

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//������ƵԴ��֡�ٶ�
			int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(cb->pts, 90000);
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->bUpdateVideoFrameSpeedFlag = true;

#ifdef USE_BOOST
				//����UDP��TCP������Ϊ�����Ѿ����� 
				boost::shared_ptr<CNetRevcBase>  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
#else
				//����UDP��TCP������Ϊ�����Ѿ����� 
				auto  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
#endif
			

				if (pThis->pMediaSource)
				{
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
					pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
				}

				WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
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
				pThis->pMediaSource->netBaseNetType = pThis->netBaseNetType;
				pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
				pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
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
			pThis->GetAACAudioInfo(cb->data, cb->datasize);//��ȡAACý����Ϣ
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
#ifdef  WriteRtpFileFlag
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.rtp", ABL_MediaSeverRunPath, this);
	fWriteRtpFile = fopen(szFileName, "wb");
#endif

	nClientRtcp = hParent = 0;
	fWritePsFile = NULL;
	nRtpRtcpPacketType = 0;
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	m_gbPayload = 96;
	hRtpHandle = psDeMuxHandle  = 0;
	psBeiJingLaoChen = NULL;
	memset((char*)&mediaCodecInfo, 0x00, sizeof(mediaCodecInfo));
	bInitFifoFlag = false;
	pMediaSource = NULL; 
	netDataCache = NULL ; //�������ݻ���
	netDataCacheLength = 0;//�������ݻ����С
	nNetStart = nNetEnd = 0; //����������ʼλ��\����λ��
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

 	WriteLog(Log_Debug, "CNetGB28181RtpServer ���� = %X  nClient = %llu ", this, nClient);
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

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//����
		if (psDeMuxHandle > 0)
		{
			ps_demux_stop(psDeMuxHandle);
			psDeMuxHandle = 0;
		}
	}
	else
	{//�����ϳ�
		if (psBeiJingLaoChen != NULL)
		{
			ps_demuxer_destroy(psBeiJingLaoChen);
			psBeiJingLaoChen = NULL;
		}
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
		 WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ������������������ hParent = %llu, nClient = %llu ,nMediaClient = %llu", this, hParent, nClient, nMediaClient);
		 pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));
	 }

	 //����rtcp 
	 if (nClientRtcp > 0)
		 XHNetSDK_DestoryUdp(nClientRtcp); 

	 //����ɾ��ý��Դ
	 if (strlen(m_szShareMediaURL) > 0 && pMediaSource != NULL )
	    DeleteMediaStreamSource(m_szShareMediaURL);

#ifdef  WriteRtpFileFlag
	 if(fWriteRtpFile)
 	   fclose(fWriteRtpFile);
#endif
	 WriteLog(Log_Debug, "CNetGB28181RtpServer ���� = %X  nClient = %llu , hParent= %llu , app = %s ,stream = %s ,bUpdateVideoFrameSpeedFlag = %d ", this, nClient, hParent, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, bUpdateVideoFrameSpeedFlag);
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
 
#ifdef  WriteRtpFileFlag
	if (fWriteRtpFile)
	{
		fwrite((unsigned char*)&nDataLength, 1, sizeof(nDataLength), fWriteRtpFile);
		fwrite(pData, 1, nDataLength, fWriteRtpFile);
		fflush(fWriteRtpFile);
		return 0;
     }
#endif

	//���ӱ���ԭʼ��rtp���ݣ����еײ����
	if (ABL_MediaServerPort.nSaveGB28181Rtp == 1 && pWriteRtpFile != NULL && (GetTickCount64() - nCreateDateTime) < 1000 * 300)
	{
		fwrite(pData, 1, nDataLength, pWriteRtpFile);
		fflush(pWriteRtpFile);
	}

	//������߼��
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

		//��ȡ��һ��ssrc 
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
		{//��ֻ֤�ϵ�һ��IP���˿ڽ�����PS���� 
			if (strcmp(inet_ntoa(pRtpAddress->sin_addr), inet_ntoa(((sockaddr_in*)address)->sin_addr)) == 0 &&  //IP ��ͬ
				ntohs(pRtpAddress->sin_port) - 1 == ntohs(((sockaddr_in*)address)->sin_port) &&  //�˿���ͬ
				nSSRC == rtpHeaderPtr->ssrc  // ssrc ��ͬ 
				)
			{
 	         NetDataFifo.push((unsigned char*)pData, nDataLength);
			 if (nClientPort == 0)
			 {
				 strcpy(szClientIP, inet_ntoa(pRtpAddress->sin_addr));
				 nClientPort = ntohs(((sockaddr_in*)address)->sin_port);
			 }
			}
 		}
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)
	{//TCP 
 		if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
		{//ʣ��ռ��㹻
			memcpy(netDataCache + nNetEnd, pData, nDataLength);
			netDataCacheLength += nDataLength;
			nNetEnd += nDataLength;
		}
		else
		{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
			if (netDataCacheLength > 0)
			{//���������ʣ��
				memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
				nNetStart = 0;
				nNetEnd = netDataCacheLength;

				if (MaxNetDataCacheCount - nNetEnd < nDataLength)
				{
					nNetStart = nNetEnd = netDataCacheLength = 0;
					WriteLog(Log_Debug, "CNetGB28181RtpServer = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return 0;
				}
			}
			else
			{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
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
	if(!bRunFlag)
		return -1 ;
	
	unsigned char* pData = NULL;
	int            nLength;

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{//UDP
		pData = NetDataFifo.pop(&nLength);
		if (pData != NULL)
		{
			if (nLength > 0)
			{
				//����rtpͷ�����payload���н��,��Ч��ֹ�û���д��
				if (hRtpHandle == 0)
				{
					rtpHeadPtr = (_rtp_header*)pData ;
					m_gbPayload = rtpHeadPtr->payload;
				}

				RtpDepacket(pData, nLength);
			}

			NetDataFifo.pop_front();
		}

		//��������rtcp ��
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
	{//TCP ��ʽ��rtp����ȡ 
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
					nRtpRtcpPacketType = 2;//4���ֽڷ�ʽ
			}
			else
			{
 				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
			    netDataCacheLength -= 2;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 1;//2���ֽڷ�ʽ��
			}

			//��ȡrtp���İ�ͷ
			rtpHeadPtr = (_rtp_header*)(netDataCache + nNetStart);

			nRtpLength = ntohs(nRtpLength);
			if (nRtpLength > 65536 )
			{
				WriteLog(Log_Debug, "CNetGB28181RtpServer = %X rtp��ͷ��������  nClient = %llu ,nRtpLength = %llu", this, nClient, nRtpLength);
 				DeleteNetRevcBaseClient(nClient);
				return -1;
			}

			//���ȺϷ� ������rtp�� ��rtpHeadPtr->v == 2) ,��ֹrtcp����ִ��rtp���
			if (nRtpLength > 0 && rtpHeadPtr->v == 2)
			{
				//����rtpͷ�����payload���н��,��Ч��ֹ�û���д��
				if (hRtpHandle == 0)
					m_gbPayload = rtpHeadPtr->payload;

				RtpDepacket(netDataCache + nNetStart, nRtpLength);
 			}

			nNetStart += nRtpLength;
			netDataCacheLength -= nRtpLength;
		}

		//��������rtcp ��
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

//rtp ���
struct ps_demuxer_notify_t notify_NetGB28181RtpServer = { mpeg_ps_dec_testonstream,};

bool  CNetGB28181RtpServer::RtpDepacket(unsigned char* pData, int nDataLength) 
{
	if (pData == NULL || nDataLength > 65536 || !bRunFlag || nDataLength < 12 )
		return false;

	//����rtp���
	if (hRtpHandle == 0)
	{
		rtp_depacket_start(GB28181_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);

		if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x31)
		{//rtp + PS
		  rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_gbps);
 		}
		else if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x32)
		{//rtp + ES
			if(m_gbPayload == 98)
			  rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_h264);
			else if(m_gbPayload == 99)
			  rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_h265);
		}
		else if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x33)
		{//rtp + xhb
			rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_xhb);
		}
 
 		WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ,����rtp��� hRtpHandle = %d ,psDeMuxHandle = %d", this, hRtpHandle, psDeMuxHandle);
	}

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//����
		if(psDeMuxHandle == 0)
		  ps_demux_start(GB28181_RtpRecv_demux_callback, (void*)this, e_ps_dux_timestamp, &psDeMuxHandle);
	}
	else
	{//�����ϳ�
		if (psBeiJingLaoChen == NULL)
		{
			psBeiJingLaoChen = ps_demuxer_create(on_gb28181_unpacket, this);
			if(psBeiJingLaoChen != NULL )
		      ps_demuxer_set_notify(psBeiJingLaoChen, &notify_NetGB28181RtpServer, this);
		}
	}

	if (hRtpHandle > 0 )
	{
		if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x31 || m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x32)
		{//rtp + PS �� rtp + ES 
	      rtp_depacket_input(hRtpHandle, pData, nDataLength);
		}
		else if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x33)
		{// rtp + XHB  ��rtp ��ͷ����չ 
 		   rtpHeaderXHB = (_rtp_header*)pData;
		   if (rtpHeaderXHB->x == 1 && nDataLength > 16 )
		   {
			   memcpy((unsigned char*)&rtpExDataLength, pData + 12 + 2, sizeof(rtpExDataLength));
			   rtpExDataLength = ntohs(rtpExDataLength) * 4 ;

			   if (nDataLength - (12 + 4 + rtpExDataLength ) > 0  )
			   {
			     pData[0] = 0x80;
			     memmove(pData + 12, pData + (12 + 4 + rtpExDataLength), nDataLength - (12 + 4 + rtpExDataLength));
			     rtp_depacket_input(hRtpHandle, pData, nDataLength - ( 4 + rtpExDataLength) );
			   }
		   }else
			   rtp_depacket_input(hRtpHandle, pData, nDataLength);
		}
  	}

	return true;
}

//����AAC��Ƶ���ݻ�ȡAACý����Ϣ
void CNetGB28181RtpServer::GetAACAudioInfo(unsigned char* nAudioData, int nLength)
{
	if (mediaCodecInfo.nChannels == 0 && mediaCodecInfo.nSampleRate == 0)
	{
		unsigned char nSampleIndex = 1;
		unsigned char  nChannels = 1;

		nSampleIndex = ((nAudioData[2] & 0x3c) >> 2) & 0x0F;  //�� szAudio[2] �л�ȡ����Ƶ�ʵ����
		if (nSampleIndex >= 15)
			nSampleIndex = 8;
		mediaCodecInfo.nSampleRate = SampleRateArray[nSampleIndex];

		//ͨ���������� pAVData[2]  ����2��λ�������2λ���� 0x03 �����㣬�õ���λ�����ƶ�2λ ���� �� �� pAVData[3] ��������2λ
		//pAVData[3] ������2λ��ȡ���� �� �� 0xc0 �����㣬������6λ��ΪʲôҪ����6λ����Ϊ��2λ�������λ������Ҫ���ұ��ƶ�6λ
		nChannels = ((nAudioData[2] & 0x03) << 2) | ((nAudioData[3] & 0xc0) >> 6);
		if (nChannels > 2)
			nChannels = 1;
		mediaCodecInfo.nChannels = nChannels;

		strcpy(mediaCodecInfo.szAudioName, "AAC");

		WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ,��ȡ��������� AAC��Ϣ szAudioName = %s,nChannels = %d ,nSampleRate = %d ", this, mediaCodecInfo.szAudioName, mediaCodecInfo.nChannels, mediaCodecInfo.nSampleRate);
	}
}

//TCP��ʽ����rtcp��
void  CNetGB28181RtpServer::ProcessRtcpData(unsigned char* szRtcpData, int nDataLength, int nChan)
{
	if (netBaseNetType != NetBaseNetType_NetGB28181RtpServerTCP_Server || !bRunFlag)
		return;

	if (nRtpRtcpPacketType == 1)
	{//2�ֽڷ�ʽ
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

//���͵�һ������
int CNetGB28181RtpServer::SendFirstRequst()
{

	 return 0;
}

//����m3u8�ļ�
bool  CNetGB28181RtpServer::RequestM3u8File()
{
	return true;
}