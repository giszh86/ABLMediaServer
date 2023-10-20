/*
���ܣ�
    ����webrtc�Ĳ������󡢶Ͽ����� 
	 
����    2023-08-03
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientWebrtcPlayer.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern CMediaFifo                            pWebRtcDisconnectFifo;          //webrtcɾ������ 

extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
#include "../webrtc-streamer/rtc_obj_sdk.h"

CNetClientWebrtcPlayer::CNetClientWebrtcPlayer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_NetClientWebrtcPlayer;
	nMediaClient = 0;
	nCreateDateTime = GetTickCount64();
	nClient = hClient;
	bRunFlag = true;

	pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
	if (pMediaSource == NULL)
	{
		WriteLog(Log_Debug, "CNetClientWebrtcPlayer = %X  nClient = %llu ,������ý��Դ %s ,׼��ɾ�� webrtc ������", this, nClient, m_szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL)->Init("H264", pMediaSource->m_mediaCodecInfo.nWidth, pMediaSource->m_mediaCodecInfo.nHeight, pMediaSource->m_mediaCodecInfo.nVideoFrameRate);
	pMediaSource->AddClientToMap(nClient);
	WriteLog(Log_Debug, "CNetClientWebrtcPlayer ���� = %X  nClient = %llu ", this, nClient);
}

CNetClientWebrtcPlayer::~CNetClientWebrtcPlayer()
{
  	WriteLog(Log_Debug, "CNetClientWebrtcPlayer ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
	pWebRtcDisconnectFifo.push((unsigned char*)webRtcCallStruct.playerID, strlen(webRtcCallStruct.playerID));
}

int CNetClientWebrtcPlayer::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0 ;

	if(strcmp(szVideoCodec,"H264") != 0)
		return -1 ;

	if (VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL))
	{
	   VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL)->onData("H264", pVideoData, nDataLength, 1);
 	}
 
	return 0;
}

int CNetClientWebrtcPlayer::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;

	return 0;
}

int CNetClientWebrtcPlayer::SendVideo()
{
	return 0;
}

int CNetClientWebrtcPlayer::SendAudio()
{

	return 0;
}

int CNetClientWebrtcPlayer::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetClientWebrtcPlayer::ProcessNetData()
{
 	return 0;
}

//���͵�һ������
int CNetClientWebrtcPlayer::SendFirstRequst()
{
	 return 0;
}

//����m3u8�ļ�
bool  CNetClientWebrtcPlayer::RequestM3u8File()
{
	return true;
}