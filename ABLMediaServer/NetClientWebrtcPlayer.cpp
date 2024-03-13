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
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
#endif

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;

#include "../webrtc-streamer/inc/rtc_obj_sdk.h"

CNetClientWebrtcPlayer::CNetClientWebrtcPlayer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
#ifdef WebRtcVideoFileFlag
	nWriteFileCount = 0;
	char szFileName[256] = { 0 };
	sprintf(szFileName,"%s%X.264", ABL_MediaSeverRunPath, this);
	fWriteVideoFile = fopen(szFileName, "wb");

	sprintf(szFileName, "%s%X_Length.txt", ABL_MediaSeverRunPath, this);
 	fWriteFrameLengthFile = fopen(szFileName, "wb");;
#endif
	nSpsPositionPos = 0;
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
	if(pMediaSource->bCreateWebRtcPlaySourceFlag.load() == false )
	{
		if (VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL) != NULL)
		{
	      VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL)->Init("H264", pMediaSource->m_mediaCodecInfo.nWidth, pMediaSource->m_mediaCodecInfo.nHeight, pMediaSource->m_mediaCodecInfo.nVideoFrameRate);
	      pMediaSource->bCreateWebRtcPlaySourceFlag.exchange(true);
 	    }
	}
	pMediaSource->AddClientToMap(nClient);
	pMediaSource->nWebRtcPushStreamID = nClient;
	SplitterAppStream(szShareMediaURL);
	sprintf(m_addStreamProxyStruct.url, "http://%s:%d/webrtc-streamer.html?video=/%s/%s", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
	WriteLog(Log_Debug, "CNetClientWebrtcPlayer ���� = %X  nClient = %llu ", this, nClient);
}

CNetClientWebrtcPlayer::~CNetClientWebrtcPlayer()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (pMediaSource != NULL)
	{
		if (pMediaSource->bCreateWebRtcPlaySourceFlag.load() == true)
			pMediaSource->bCreateWebRtcPlaySourceFlag.exchange(false);
 	}
#ifdef WebRtcVideoFileFlag
	if(fWriteVideoFile)
	  fclose(fWriteVideoFile);
	if(fWriteFrameLengthFile)
	  fclose(fWriteFrameLengthFile);
#endif
  	WriteLog(Log_Debug, "CNetClientWebrtcPlayer ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientWebrtcPlayer::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag)
		return -1;

	nRecvDataTimerBySecond = 0 ;

	if(strcmp(szVideoCodec,"H264") != 0  )
		return -1 ;

#ifdef WebRtcVideoFileFlag
	if (fWriteVideoFile && nDataLength >= 1000 * 100 * 200 )
	{
		nWriteFileCount += nDataLength ;
		if (nWriteFileCount < 1024 * 1024 * 50)
		{
		  fwrite(pVideoData,1, nDataLength,fWriteVideoFile);
		  fflush(fWriteVideoFile);
 		}
 	}

	if (fWriteFrameLengthFile)
	{
		if (nWriteFileCount < 1024 * 1024 * 50)
		{
		   fprintf(fWriteFrameLengthFile, "frame = %d  , %02x %02x %02x %02x %02x \r\n", nDataLength, pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4]);
		   fflush(fWriteFrameLengthFile);
		}
	}
#endif
	nSpsPositionPos = FindSPSPositionPos("H264", pVideoData, nDataLength);

	if (VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL) && nSpsPositionPos >= 0 )
	{
	   VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL)->onData("H264", pVideoData + nSpsPositionPos , nDataLength - nSpsPositionPos, 1);
 	}
 
	return 0;
}

int CNetClientWebrtcPlayer::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag)
		return -1;

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