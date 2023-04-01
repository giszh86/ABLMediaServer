/*
���ܣ�
    ������ GB28181 Rtp ����������UDP��TCPģʽ 
 	 
����    2021-08-15
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpClient.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaSendThreadPool* pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaSendThreadPool* pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;

#endif

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

static void* ps_alloc(void* param, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;
	 
	return pThis->s_buffer;
}

static void ps_free(void* param, void* /*packet*/)
{
	return;
}

static int ps_write(void* param, int stream, void* packet, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;

	if(pThis->bRunFlag)
	  pThis->GB28181PsToRtPacket((unsigned char*)packet, bytes);

	return true;
}

//rtp����ص���Ƶ
void GB28181_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag)
		return;

	if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//udp ֱ�ӷ��� 
		pThis->SendGBRtpPacketUDP(cb->data, cb->datasize);
	}
	else if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{//TCP ��Ҫƴ�� ��ͷ
		pThis->GB28181SentRtpVideoData(cb->data, cb->datasize);
	}
}

//PS ���ݴ����rtp 
void  CNetGB28181RtpClient::GB28181PsToRtPacket(unsigned char* pPsData, int nLength)
{
	if(hRtpPS > 0 && bRunFlag)
	{
		inputPS.data = pPsData;
		inputPS.datasize = nLength;
		rtp_packet_input(&inputPS);
	}
}

//����28181PS����TCP��ʽ���� 
void  CNetGB28181RtpClient::GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength)
{
	if (bRunFlag == false)
		return;
	
	if ((MaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0)
	{//ʣ��ռ䲻���洢 ,��ֹ���� 
 		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			bRunFlag = false;

			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, ���͹���RTP�������� ��Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != GB28181VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//����һ֡�µ���Ƶ 
		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, ���͹���RTP�������� ��Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	if (ABL_MediaServerPort.nGBRtpTCPHeadType == 1)
	{//���� TCP���� 4���ֽڷ�ʽ
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 4;
	}
	else if (ABL_MediaServerPort.nGBRtpTCPHeadType == 2)
	{//���� TCP���� 2 ���ֽڷ�ʽ
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + nSendRtpVideoMediaBufferLength, (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 2;
	}
	else
	{
		bRunFlag = false;
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, �Ƿ��Ĺ���TCP��ͷ���ͷ�ʽ(����Ϊ 1��2 )nGBRtpTCPHeadType = %d ", this, ABL_MediaServerPort.nGBRtpTCPHeadType);
		DeleteNetRevcBaseClient(nClient);
	}
}

CNetGB28181RtpClient::CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	psMuxHandle = 0;

	nVideoStreamID = nAudioStreamID = -1;
	handler.alloc = ps_alloc;
	handler.write = ps_write;
	handler.free = ps_free;
    videoPTS = audioPTS = 0;
	s_buffer = NULL;
	if (ABL_MediaServerPort.gb28181LibraryUse == 2)
	{
		s_buffer = new  char[IDRFrameMaxBufferLength];
		psBeiJingLaoChen = ps_muxer_create(&handler, this);
	}
	hRtpPS = 0;
	bRunFlag = true;

	nSendRtpVideoMediaBufferLength = 0; //�Ѿ����۵ĳ���  ��Ƶ
	nStartVideoTimestamp           = GB28181VideoStartTimestampFlag ; //��һ֡��Ƶ��ʼʱ��� ��
	nCurrentVideoTimestamp         = 0;// ��ǰ֡ʱ���

	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

	if (nPort == 0) 
	{//��Ϊudp ʹ�ã���Ϊtcpʱ�� ��SendFirstRequst() �⺯������ 
#ifdef USE_BOOST
		boost::shared_ptr<CMediaStreamSource> pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
#else
		auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
#endif
	
		if (pMediaSource != NULL)
		{
			memcpy((char*)&mediaCodecInfo,(char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));
			pMediaSource->AddClientToMap(nClient);
		}

		//��nClient ����Video ,audio �����߳�
		pMediaSendThreadPool->AddClientToThreadPool(nClient);
   }

#ifdef  WriteGB28181PSFileFlag
	char    szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.ps", ABL_MediaSeverRunPath,this);
	writePsFile = fopen(szFileName,"wb");
#endif

 	WriteLog(Log_Debug, "CNetGB28181RtpClient ���� = %X  nClient = %llu ", this, nClient);
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
	if(psBeiJingLaoChen != NULL )
	  ps_muxer_destroy(psBeiJingLaoChen);
	SAFE_ARRAY_DELETE(s_buffer);
#ifdef  WriteGB28181PSFileFlag
	fclose(writePsFile);
#endif
	WriteLog(Log_Debug, "CNetGB28181RtpClient ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
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
	nRecvDataTimerBySecond = 0;

	//�� m_startSendRtpStruct.RtpPayloadDataType[0] != 0x31 ��Ϊ PS ���ʱ,��֧����Ƶ���ݼ����� 
	if (!bRunFlag || m_startSendRtpStruct.RtpPayloadDataType[0] != 0x31)
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

void  CNetGB28181RtpClient::CreateRtpHandle()
{
	if (hRtpPS == 0)
	{
		int nRet = rtp_packet_start(GB28181_rtp_packet_callback_func_send, (void*)this, &hRtpPS);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ��������Ƶrtp���ʧ��,nClient = %llu,  nRet = %d", this, nClient, nRet);
			return ;
		}
		optionPS.handle = hRtpPS;
		optionPS.mediatype = e_rtppkt_mt_video;
		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//����PS���
			optionPS.payload = atoi(m_startSendRtpStruct.payload);
			optionPS.streamtype = e_rtppkt_st_gb28181;
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES \ XHB ���
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			{
			  strcpy(m_startSendRtpStruct.payload, "98");
			  optionPS.payload = 98 ;
			  optionPS.streamtype = e_rtppkt_st_h264;
			}
			else  if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			{
				strcpy(m_startSendRtpStruct.payload, "99");
				optionPS.payload = 99 ;
				optionPS.streamtype = e_rtppkt_st_h265;
			}
		}
		optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
		optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
		rtp_packet_setsessionopt(&optionPS);

		inputPS.handle = hRtpPS;
		inputPS.ssrc = optionPS.ssrc;

		memset((char*)&gbDstAddr, 0x00, sizeof(gbDstAddr));
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));

		//����ý��Դ
		SplitterAppStream(m_szShareMediaURL);
		sprintf(m_addStreamProxyStruct.url, "rtp://localhost/%s/%s", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
		strcpy(szClientIP, m_startSendRtpStruct.dst_url);
		nClientPort = atoi(m_startSendRtpStruct.dst_port);
	}
}

int CNetGB28181RtpClient::SendVideo()
{
	if (!bRunFlag)
		return -1;

	//�������ɹ������ӳɹ�
	if (!bUpdateVideoFrameSpeedFlag)
		bUpdateVideoFrameSpeedFlag = true;

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//����
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

			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ������ ps ����ɹ�  ,nClient = %llu,  nRet = %d", this, nClient, ret);
		}
	}
	else
	{//�����ϳ�
		if (nVideoStreamID == -1 && psBeiJingLaoChen != NULL )
		{
			if (strcmp(mediaCodecInfo.szVideoName,"H264") == 0 )
			  nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_H264, NULL, 0);
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			  nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_H265, NULL, 0);
		}
	}

	//����rtp���
	CreateRtpHandle();

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

		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//PS ���
			//����PS���
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{
	  		  input.data = pData;
			  input.datasize = nLength;
			  ps_mux_input(&input);
			}
			else
			{//�����ϳ�PS���
				if (nVideoStreamID != -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szVideoName) > 0 )
				{
					if(strcmp(mediaCodecInfo.szVideoName,"H264") == 0)
					  nflags = CheckVideoIsIFrame("H264", pData, nLength);
					else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
					  nflags = CheckVideoIsIFrame("H265", pData, nLength);

					ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nVideoStreamID, nflags, videoPTS, videoPTS, pData, nLength);
					videoPTS += (90000 / mediaCodecInfo.nVideoFrameRate);
				}
			}
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES ��� \ XHB ���
			if (hRtpPS > 0 && bRunFlag)
			{
				inputPS.data = pData;
				inputPS.datasize = nLength;
				rtp_packet_input(&inputPS);
			}
		}
 
		m_videoFifo.pop_front();
	}
	return 0;
}

int CNetGB28181RtpClient::SendAudio()
{
	if ( ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag || m_startSendRtpStruct.RtpPayloadDataType[0] != 0x31 )
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

		//����rtp���
		CreateRtpHandle();

		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//����PS���ʱ,���԰���Ƶ����Ƶ�����һ��,����ES���,��Ƶ\��Ƶ ���ܴ����һ��
			//����PS���
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{
			   input.data = pData;
			   input.datasize = nLength;
 			   ps_mux_input(&input);
			}
			else
			{//�����ϳ�PS���
				if (nAudioStreamID == -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szAudioName) > 0 )
				{
					if ( strcmp(mediaCodecInfo.szAudioName,"AAC") == 0 )
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AAC, NULL, 0);
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AUDIO_G711A, NULL, 0);
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AUDIO_G711U, NULL, 0);
				}

				if (nAudioStreamID != -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szAudioName) > 0 )
				{
					ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nAudioStreamID, 0, audioPTS, audioPTS, pData, nLength);

					if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
						audioPTS += mediaCodecInfo.nBaseAddAudioTimeStamp;
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
						audioPTS += nLength / 8;
				}
			}
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES\XHB ���

		}
 
		m_audioFifo.pop_front();
	}
	return 0;
}

//udp��ʽ����rtp��
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

//���͵�һ������
int CNetGB28181RtpClient::SendFirstRequst()
{//�� gb28181 Ϊtcpʱ�������ú��� 

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{//�ظ�http�������ӳɹ���
		sprintf(szResponseBody, "{\"code\":0,\"port\":%d,\"memo\":\"success\",\"key\":%llu}", nReturnPort, nClient);
		ResponseHttp(nClient_http, szResponseBody, false);
	}
#ifdef USE_BOOST
	boost::shared_ptr<CMediaStreamSource> pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
#else
	auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
#endif

	if (pMediaSource != NULL)
	{
		memcpy((char*)&mediaCodecInfo, (char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));
		pMediaSource->AddClientToMap(nClient);
	}

	//��nClient ����Video ,audio �����߳�
	pMediaSendThreadPool->AddClientToThreadPool(nClient);
	return 0;
}

//����m3u8�ļ�
bool  CNetGB28181RtpClient::RequestM3u8File()
{
	return true;
}