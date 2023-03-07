/*
功能：
    负责控制主动拉流 rtsp\rtmp\flv\hls 
	检测拉流是否断线、断线重连、彻底销毁拉流等等功能 
	 
日期     2021-07-27
作者     罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientAddStreamProxy.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);

CNetClientAddStreamProxy::CNetClientAddStreamProxy(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_addStreamProxyControl;
	nMediaClient = 0;
	nClient = hClient;
	m_hParent = hParent;
	m_nXHRtspURLType = nXHRtspURLType;

	m_pCustomerPtr = pCustomerPtr;
	m_callbackFunc = callbackFunc;
	nRtspConnectStatus = RtspConnectStatus_AtConnecting;

	time(&nCreateDateTime);
	time(&nProxyDisconnectTime);
	Rtsp_WriteLog(Log_Debug, "CNetClientAddStreamProxy 构造 = %X nClient = %llu , nCreateDateTime = %d", this, nClient, nCreateDateTime);
}

CNetClientAddStreamProxy::~CNetClientAddStreamProxy()
{
	Rtsp_WriteLog(Log_Debug, "CNetClientAddStreamProxy 析构 = %X  nClient = %llu ,nMediaClient = %llu", this, nClient, nMediaClient);
	boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClientNoLock(nMediaClient);
	if (pClient)
	{
		CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)pClient.get();
		pRtsp->bRunFlag = false;
	}
	pDisconnectBaseNetFifo.push((unsigned char*)&nMediaClient,sizeof(nMediaClient));
}

int CNetClientAddStreamProxy::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetClientAddStreamProxy::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetClientAddStreamProxy::SendVideo()
{
	return 0;
}

int CNetClientAddStreamProxy::SendAudio()
{

	return 0;
}

int CNetClientAddStreamProxy::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength)
{
    return 0;
}

int CNetClientAddStreamProxy::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetClientAddStreamProxy::SendFirstRequst()
{
	//nProxyDisconnectTime = GetTickCount64();
	time(&nCreateDateTime);

	//正在连接
	nRtspConnectStatus = RtspConnectStatus_AtConnecting;

	if (strlen(m_addStreamProxyStruct.url) > 0)
	{
		boost::shared_ptr<CNetRevcBase> pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, m_addStreamProxyStruct.url, 0, m_szShareMediaURL, m_pCustomerPtr, m_callbackFunc, nClient,m_nXHRtspURLType);
	   if (pClient)
	   {
 		 nMediaClient = pClient->nClient;  //把rtsp拉流对象赋值给 代理拉流 
 	   }
 	}

	return 0;
}

//请求m3u8文件
bool  CNetClientAddStreamProxy::RequestM3u8File()
{
	return true;
}
