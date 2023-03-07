/*
���ܣ�
    ��������������� rtsp\rtmp\flv\hls 
	��������Ƿ���ߡ������������������������ȵȹ��� 
	 
����     2021-07-27
����     �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientAddStreamProxy.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
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
	Rtsp_WriteLog(Log_Debug, "CNetClientAddStreamProxy ���� = %X nClient = %llu , nCreateDateTime = %d", this, nClient, nCreateDateTime);
}

CNetClientAddStreamProxy::~CNetClientAddStreamProxy()
{
	Rtsp_WriteLog(Log_Debug, "CNetClientAddStreamProxy ���� = %X  nClient = %llu ,nMediaClient = %llu", this, nClient, nMediaClient);
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

//���͵�һ������
int CNetClientAddStreamProxy::SendFirstRequst()
{
	//nProxyDisconnectTime = GetTickCount64();
	time(&nCreateDateTime);

	//��������
	nRtspConnectStatus = RtspConnectStatus_AtConnecting;

	if (strlen(m_addStreamProxyStruct.url) > 0)
	{
		boost::shared_ptr<CNetRevcBase> pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, m_addStreamProxyStruct.url, 0, m_szShareMediaURL, m_pCustomerPtr, m_callbackFunc, nClient,m_nXHRtspURLType);
	   if (pClient)
	   {
 		 nMediaClient = pClient->nClient;  //��rtsp��������ֵ�� �������� 
 	   }
 	}

	return 0;
}

//����m3u8�ļ�
bool  CNetClientAddStreamProxy::RequestM3u8File()
{
	return true;
}
