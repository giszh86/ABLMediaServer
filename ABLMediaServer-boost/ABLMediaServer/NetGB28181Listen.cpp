/*
功能：
    实现国标28181 TCP方式接收码流的listen过程 
 	 
日期    2022-03-16
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181Listen.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern MediaServerPort                       ABL_MediaServerPort; 
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;          //消息通知FIFO

CNetGB28181Listen::CNetGB28181Listen(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL, szShareMediaURL);
	netBaseNetType = NetBaseNetType_NetGB28181RtpServerListen;
	nClient = hClient;
	nClientPort = nPort;
	if (strlen(szShareMediaURL) > 0)
		SplitterAppStream(szShareMediaURL);
	nMediaClient = 0;

	WriteLog(Log_Debug, "CNetGB28181Listen 构造 = %X  nClient = %llu ", this, nClient);
}

CNetGB28181Listen::~CNetGB28181Listen()
{
	XHNetSDK_Unlisten(nClient);
 
	if(nMediaClient > 0)
		pDisconnectBaseNetFifo.push((unsigned char*)&nMediaClient, sizeof(nMediaClient));
	else
	{//码流没有达到通知
 		if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.nClientNotArrive > 0 && bUpdateVideoFrameSpeedFlag == false)
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = ABL_MediaServerPort.nClientNotArrive;
			sprintf(msgNotice.szMsg, "{\"mediaServerId\":\"%s\",\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"key\":%llu}", ABL_MediaServerPort.mediaServerID, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, netBaseNetType, nClient);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}
	}

	WriteLog(Log_Debug, "CNetGB28181Listen 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetGB28181Listen::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetGB28181Listen::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetGB28181Listen::SendVideo()
{
  
	return 0;
}

int CNetGB28181Listen::SendAudio()
{

	return 0;
}

int CNetGB28181Listen::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetGB28181Listen::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetGB28181Listen::SendFirstRequst()
{
 
    return 0;
}

//请求m3u8文件
bool  CNetGB28181Listen::RequestM3u8File()
{
	return true;
}