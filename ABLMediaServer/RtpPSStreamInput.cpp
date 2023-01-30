/*
���ܣ�
    ����ps�����γ�ý��Դ�����ڽ�������ps��
 	 
����    2022-07-12
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "RtpPSStreamInput.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo;       //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256];   //��ǰ·��
extern int                                   SampleRateArray[];

void RTP_DEPACKET_CALL_METHOD RTP10000_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CRtpPSStreamInput* pThis = (CRtpPSStreamInput*)cb->userdata;
 
	if (pThis != NULL && pThis->bRunFlag)
	{
		if (pThis->nSSRC == 0)
			pThis->nSSRC = cb->ssrc; //Ĭ�ϵ�һ��ssrc 
		if (pThis->nSSRC == cb->ssrc)
  		   ps_demux_input(pThis->psDeMuxHandle, cb->data, cb->datasize);
	}
}

void PS_DEMUX_CALL_METHOD RTP10000_RtpRecv_demux_callback(_ps_demux_cb* cb)
{
	CRtpPSStreamInput* pThis = (CRtpPSStreamInput*)cb->userdata;
	if (!pThis->bRunFlag)
		return;

	if (pThis && cb->streamtype == e_rtpdepkt_st_h264 || cb->streamtype == e_rtpdepkt_st_h265 ||
		cb->streamtype == e_rtpdepkt_st_mpeg4 || cb->streamtype == e_rtpdepkt_st_mjpeg)
	{
		if (cb->streamtype == e_rtpdepkt_st_h264)
			pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
		else if (cb->streamtype == e_rtpdepkt_st_h265)
			pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//������ƵԴ��֡�ٶ�
			int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(cb->pts, 90000);
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->pMediaSource->netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;//ָ��ΪPS������
				pThis->bUpdateVideoFrameSpeedFlag = true;

				if (pThis->pMediaSource)
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��

				WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
			}
		}
	}
	else if (pThis)
	{
		if (cb->streamtype == e_rtpdepkt_st_aac)
		{//aac
			pThis->GetAACAudioInfo(cb->data, cb->datasize);//��ȡAACý����Ϣ
			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, pThis->mediaCodecInfo.szAudioName, pThis->mediaCodecInfo.nChannels, pThis->mediaCodecInfo.nSampleRate);
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

CRtpPSStreamInput::CRtpPSStreamInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
 	netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;
	strcpy(m_szShareMediaURL, szShareMediaURL);
	nClient = hClient;
	bRunFlag = true;
  
	m_gbPayload = 96;
	SplitterAppStream(m_szShareMediaURL);
	strcpy(m_addStreamProxyStruct.url, szIP);
	pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
	if (pMediaSource == NULL)
	{
		WriteLog(Log_Debug, "CRtpPSStreamInput ���� = %X ����ý��Դʧ��  nClient = %llu ,m_szShareMediaURL = %s ", this, nClient, m_szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	hRtpHandle = 0;

	WriteLog(Log_Debug, "CRtpPSStreamInput ���� = %X  nClient = %llu ,m_szShareMediaURL = %s ", this, nClient,m_szShareMediaURL);
}

CRtpPSStreamInput::~CRtpPSStreamInput()
{
	std::lock_guard<std::mutex> lock(psRecvLock);
	bRunFlag = false;

	m_videoFifo.FreeFifo();

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

	//ɾ���ַ�Դ
	if (strlen(m_szShareMediaURL) > 0)
		DeleteMediaStreamSource(m_szShareMediaURL);

	WriteLog(Log_Debug, "CRtpPSStreamInput ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

//����AAC��Ƶ���ݻ�ȡAACý����Ϣ
void CRtpPSStreamInput::GetAACAudioInfo(unsigned char* nAudioData, int nLength)
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


int CRtpPSStreamInput::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CRtpPSStreamInput::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CRtpPSStreamInput::SendVideo()
{
	return 0;
}

int CRtpPSStreamInput::SendAudio()
{

	return 0;
}

int CRtpPSStreamInput::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag)
		return -1;
	std::lock_guard<std::mutex> lock(psRecvLock);
	nRecvDataTimerBySecond = 0;

	if (hRtpHandle == 0)
	{
		rtp_depacket_start(RTP10000_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);
		rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_gbps);

		ps_demux_start(RTP10000_RtpRecv_demux_callback, (void*)this, e_ps_dux_timestamp, &psDeMuxHandle);

		WriteLog(Log_Debug, "CRtpPSStreamInput = %X ,����rtp��� hRtpHandle = %d ,psDeMuxHandle = %d", this, hRtpHandle, psDeMuxHandle);
	}

	if (hRtpHandle > 0)
		rtp_depacket_input(hRtpHandle, pData, nDataLength);
  
    return 0;
}

int CRtpPSStreamInput::ProcessNetData()
{
  
 	return 0;
}

//���͵�һ������
int CRtpPSStreamInput::SendFirstRequst()
{
  	 return 0;
}

//����m3u8�ļ�
bool  CRtpPSStreamInput::RequestM3u8File()
{
	return true;
}