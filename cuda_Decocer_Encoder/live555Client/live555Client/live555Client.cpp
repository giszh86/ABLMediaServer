
#include "stdafx.h"
#include "live555Client.h"

typedef boost::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;
CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;
std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
CMediaFifo                                                       pDisconnectBaseNetFifo;  //������ѵ����� 
CMediaFifo                                                       pErrorDisconnectBaseNetFifo;  //�����쳣����Ҫ�Ͽ�
CMediaFifo                                                       pReConnectStreamProxyFifo; //��Ҫ�������Ӵ���ID 
bool                                                             ABL_bInitLive555ClientFlag = false;
bool                                                             ABL_bExitMediaServerRunFlag = false;
bool                                                             ABL_bLive555ClientRunFlag = true;
CNetBaseThreadPool*                                              NetBaseThreadPool;

CNetRevcBase_ptr CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	Rtsp_WriteLog(Log_Debug, "CreateNetRevcBaseClient(() ��ʼ netClientType = %d serverHandle = %llu CltHandle = %llu ",  netClientType, serverHandle, CltHandle);
	
	CNetRevcBase_ptr pXHClient = NULL;
	try
	{
		do
		{
			if (netClientType == NetRevcBaseClient_addStreamProxyControl)
			{//������������
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetClientAddStreamProxy>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL, pCustomerPtr, callbackFunc, hParent, nXHRtspURLType);
			}
            else if (netClientType == NetRevcBaseClient_addStreamProxy)
			{//��������
				if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".m3u8") != NULL)
				{//hls ��ʱ��֧�� hls ���� 
#if    0
					//�ڹ��캯�������첽���ӣ������һ��nClientֵ 
					pXHClient = boost::make_shared<CNetClientRecvHttpHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
#endif 
				}
				else if (memcmp(szIP, "rtsp://", 7) == 0)
				{//rtsp 
					pXHClient = boost::make_shared<CNetClientRecvRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL, pCustomerPtr, callbackFunc, hParent, nXHRtspURLType);
					CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
					if (CltHandle == 0)
					{//����ʧ��
						Rtsp_WriteLog(Log_Debug, "CreateNetRevcBaseClient()������ rtsp ������ ʧ�� szURL = %s , szIP = %s ,port = %d ", szIP, pXHClient->m_rtspStruct.szIP, pXHClient->m_rtspStruct.szPort);
						pDisconnectBaseNetFifo.push((unsigned char*)&pXHClient->nClient, sizeof(pXHClient->nClient));
					}
				}				
				else if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".flv") != NULL)
				{//flv 
					//pXHClient = boost::make_shared<CNetClientRecvFLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					//CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
 				else if (memcmp(szIP, "rtmp://", 7) == 0)
				{//rtmp
					//pXHClient = boost::make_shared<CNetClientRecvRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					//CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
				else
					return NULL;
			}
 
		} while (pXHClient == NULL);
	}
	catch (const std::exception &e)
	{
		return NULL;
	}

	std::pair<boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>::iterator, bool> ret =
		xh_ABLNetRevcBaseMap.insert(std::make_pair(CltHandle, pXHClient));
	if (!ret.second)
	{
		pXHClient.reset();
		return NULL;
	}
	
	Rtsp_WriteLog(Log_Debug, "CreateNetRevcBaseClient(() ��� netClientType = %d serverHandle = %llu CltHandle = %llu ",  netClientType, serverHandle, CltHandle);

	return pXHClient;
}

CNetRevcBase_ptr GetNetRevcBaseClient(NETHANDLE CltHandle)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		return NULL;
	}
}

//�������ң�������Ѿ�����
CNetRevcBase_ptr GetNetRevcBaseClientNoLock(NETHANDLE CltHandle)
{
	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		return NULL;
	}
}

bool  DeleteNetRevcBaseClient(NETHANDLE CltHandle)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);
	Rtsp_WriteLog(Log_Debug, "DeleteNetRevcBaseClient() �û������Ͽ� nClient = %llu ", CltHandle);

	CNetRevcBase_ptrMap::iterator iterator1;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
		{//rtsp ������ֱ�ӶϿ�
			xh_ABLNetRevcBaseMap.erase(iterator1);
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_addStreamProxyControl)
		{//�������� 
			//�Ѿ��ɹ���ʧ�ܣ�ֱ��ɾ��
			if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_ConnectSuccess || ((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_ConnectFailed) 
				xh_ABLNetRevcBaseMap.erase(iterator1);
			else if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
			{//�������ӣ���Ҫ�ռ��������ȴ���һ���ж� 
				 pDisconnectBaseNetFifo.push((unsigned char*)&CltHandle, sizeof(CltHandle)); //�ռ��û������Ͽ�������
				 //if (pDisconnectBaseNetFifo.push((unsigned char*)&CltHandle, sizeof(CltHandle))) //�ռ��û������Ͽ�������
				 //	Rtsp_WriteLog(Log_Debug, "DeleteNetRevcBaseClient(�������ӣ�����ɾ��)���ռ������ȴ�һ���ж� rtspChan = %llu ", ((*iterator1).second)->nClient );
			}
		}else
			xh_ABLNetRevcBaseMap.erase(iterator1);

		return true;
	}
	else
	{
		return false;
	}
}

void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	void* address)
{
	char           temp[256] = { 0 };
	unsigned short nPort = 5567;
	uint64_t       hParent;
 
	if (address)
	{
		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(address);
		sprintf(temp, "%s", ::inet_ntoa(addr->sin_addr));
		nPort = ::ntohs(addr->sin_port);
	}
}

void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptr pRtspRecv = GetNetRevcBaseClientNoLock(clihandle);
	if (pRtspRecv != NULL)
	{
		pRtspRecv->InputNetData(srvhandle, clihandle, data, datasize);
		NetBaseThreadPool->InsertIntoTask(clihandle);
	}
}

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle)
{ //�Ӻ�����Ͽ��Ļ��������������ͬʱ�Ͽ�������ɿ�ס���Ӷ������Ƶ���٣�Ҫ�ȼ�������Ȼ���ڣ�500����Ͽ�һ·�� 
	Rtsp_WriteLog(Log_Debug, "onclose() nClient = %llu �ͻ��˶Ͽ� srvhandle = %llu", clihandle, srvhandle);

	pDisconnectBaseNetFifo.push((unsigned char*)&clihandle, sizeof(clihandle));
}

void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	if (result == 0)
	{
		CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(clihandle);
		if (pClient != NULL)
		{
			//�����������
			CNetRevcBase_ptr pParent = GetNetRevcBaseClientNoLock(pClient->m_hParent);
			if (pParent != NULL)
			{
				pParent->nRtspConnectStatus = RtspConnectStatus_ConnectFailed; //����ʧ�� 
				strcpy(pParent->szCBErrorMessage, message_connError);
			}
			Rtsp_WriteLog(Log_Debug, "nClient = %llu ,URL: %s ,����ʧ�� result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
		}
		pDisconnectBaseNetFifo.push((unsigned char*)&clihandle, sizeof(clihandle));
	}
	else if (result == 1)
	{//������ӳɹ������͵�һ������
		CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(clihandle);
		if (pClient)
		{
			//�����������
			CNetRevcBase_ptr pParent = GetNetRevcBaseClientNoLock(pClient->m_hParent);
			if (pParent != NULL)
			{
				pParent->nRtspConnectStatus = RtspConnectStatus_ConnectSuccess; //���ӳɹ�
 			}
		
			Rtsp_WriteLog(Log_Debug, "nClient = %llu ,URL: %s , ���ӳɹ� result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
			pClient->SendFirstRequst();
		}
	}
}

//���������� ������M3u8���� 
int  CheckNetRevcBaseClientDisconnect()
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                           nDiconnectCount = 0;

 
	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		if (
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv ||      //�������Rtsp����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientRecv ||      //�������Rtmp����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFlvClientRecv ||   //�������HttpFlv����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv    //�������HttpHLS����
			)
		{//���ڼ�� HLS ������� �����������ӱ�����ͼ��
			((*iterator1).second)->nRecvDataTimerBySecond ++;
			((*iterator1).second)->nSendOptionsCount ++;

			if (((*iterator1).second)->m_nXHRtspURLType == XHRtspURLType_Liveing || ((*iterator1).second)->m_nXHRtspURLType == XHRtspURLType_RecordDownload)
			{//ʵ����¼��������Ҫ��⣬�����������Ҫ�����ر�
				if (((*iterator1).second)->nRecvDataTimerBySecond >= MaxRecvDataTimerBySecondDiconnect)
				{
					nDiconnectCount ++;
					Rtsp_WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() nClient = %llu ��⵽�����쳣�Ͽ� url %s ", ((*iterator1).second)->nClient, ((*iterator1).second)->m_addStreamProxyStruct.url);

					 //�Ѹ����������״̬�޸�Ϊ����ʧ��
					 CNetRevcBase_ptr pParent = GetNetRevcBaseClientNoLock(((*iterator1).second)->m_hParent);
					if (pParent)
					{
						if (pParent->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
						   pParent->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;//����ʧ�� 
 					}
                    //test ��ʱ���� ����ʽ�汾�� 
					pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
				}
			}

			//����rtcp��
			if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
			{
				CNetClientRecvRtsp* pRtspClient = (CNetClientRecvRtsp*)(*iterator1).second.get();
				if(pRtspClient != NULL)
				{
			   	  if (pRtspClient->bSendRRReportFlag)
				  {
				    time_t tNowTime;
					time(&tNowTime);
					if (tNowTime - pRtspClient->nCurrentTime >= 3)
					{
						pRtspClient->SendRtcpReportData();
					}
				 }
			   }
			}
 			
			/*//����options ���� 
			if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
			{
				CNetClientRecvRtsp* pRtspClient = (CNetClientRecvRtsp*)(*iterator1).second.get();
				if (((*iterator1).second)->nSendOptionsCount % 60 == 0)
				{//30�� 
					pRtspClient->SendOptions(pRtspClient->AuthenticateType, false);
 				}
			}*/

		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_addStreamProxyControl || ((*iterator1).second)->netBaseNetType == NetBaseNetType_addPushProxyControl)
		{//���ƴ�����������������,�����������Ƿ��ж���

			//����������60�룬���ڴ�������״̬������Ϊ����ʧ�� 
			if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
			{
				time_t tCurrentTime;
				time(&tCurrentTime);
				if (tCurrentTime - ((*iterator1).second)->nCreateDateTime >= 60)
				{
					((*iterator1).second)->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;
					Rtsp_WriteLog(Log_Debug, " nClient = %llu , �����Ѿ�����30�룬����Ϊ����ʧ�ܣ�����ö������ٲ��� ", ((*iterator1).second)->nClient);
				}
			}

			CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(((*iterator1).second)->nMediaClient);
			if (pClient == NULL)
			{//404 ����
				 if (((*iterator1).second)->bReConnectFlag == true)
				{//��Ҫ����
					if (((*iterator1).second)->nReconnctTimeCount >= MaxReconnctTimeCount)
					{
						 strcpy(((*iterator1).second)->szCBErrorMessage, message_timeout); //���ӳ�ʱ

 						 Rtsp_WriteLog(Log_Debug, " nClient = %llu , nMediaClient = %llu �Ѿ����� %s �ﵽ %d �Σ�ִ������ ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReconnctTimeCount);
	 
						 ((*iterator1).second)->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;//����ʧ�� 
						
						pErrorDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					}
					else
					{
					   ((*iterator1).second)->nReconnctTimeCount ++;
					   Rtsp_WriteLog(Log_Debug, " nClient = %llu , nMediaClient = %llu ��⵽�����쳣�Ͽ� %s ����ִ�е� %d ������", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReconnctTimeCount);
					  pReConnectStreamProxyFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
 					}
				}
				else
				{//rtsp �����Ѿ��Ͽ� ����Ҫ���ٴ�������
			         time_t tCurrentTime;
				     time(&tCurrentTime);
			         if(tCurrentTime - ((*iterator1).second)->nCreateDateTime >= 3)
					 {//��ֹ������rtsp������δ��������ɾ���˴������� 
 					    if (strlen(((*iterator1).second)->szCBErrorMessage) == 0)
						  strcpy(((*iterator1).second)->szCBErrorMessage, message_ConnectBreak);
 
						if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
					    ((*iterator1).second)->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;//����ʧ�� 

					     Rtsp_WriteLog(Log_Debug, " nClient = %llu , nMediaClient = %llu , %s  �����Ѿ��Ͽ� ����Ҫ���ٴ������� ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url);
					   
					    pErrorDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					 }
				} 
			}
		}
	}

	return nDiconnectCount;
}

//һЩ������ 
void*  ABLLive555ClientProcessThread(void* lpVoid)
{
	int nDeleteBreakTimer = 0;
	int nReconnectTimer = 0;

	unsigned char* pData = NULL;
	int            nLength;
	uint64_t       nClient;
	uint64_t       nListSize = 0;
	int            i;

  	ABL_bExitMediaServerRunFlag = false;
	while (ABL_bLive555ClientRunFlag)
	{
  		//ִ��ɾ��,��ֹ��ѭ��
		nListSize = pDisconnectBaseNetFifo.GetSize();
		if (nListSize > 0)
		{
			for (i = 0; i < nListSize ;  i++)
			{
				pData = pDisconnectBaseNetFifo.pop(&nLength);

				if (pData != NULL && nLength == sizeof(nClient))
				{
					memcpy((char*)&nClient, pData, sizeof(nClient));
					if (nClient >= 0)
					{
  						DeleteNetRevcBaseClient(nClient);//ִ��ɾ�� 
					}
				}

				pDisconnectBaseNetFifo.pop_front();
			}
		}
 
		//ִ������ 
		if (nReconnectTimer >= 5)
		{
			nReconnectTimer = 0;
			CheckNetRevcBaseClientDisconnect();

			while ((pData = pReConnectStreamProxyFifo.pop(&nLength)) != NULL)
			{
				if (nLength == sizeof(nClient))
				{
					memcpy((char*)&nClient, pData, sizeof(nClient));
					if (nClient >= 0)
					{
						CNetRevcBase_ptr pClient = GetNetRevcBaseClient(nClient);
						if (pClient)
							pClient->SendFirstRequst(); //ִ������
					}
				}

				pReConnectStreamProxyFifo.pop_front();
			}
		}

		//ɾ�������쳣������
 		while ((pData = pErrorDisconnectBaseNetFifo.pop(&nLength)) != NULL)
		{
			if (nLength == sizeof(nClient))
			{
				memcpy((char*)&nClient, pData, sizeof(nClient));
				if (nClient >= 0)
				{
					CNetRevcBase_ptr pClient = GetNetRevcBaseClient(nClient);

					if (pClient)
					{
						Rtsp_WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect()  , ������߻ص�֪ͨ��ʼ %s   nClient = %d ", pClient->szCBErrorMessage, pClient->nClient);

						if (pClient->m_callbackFunc != NULL)
							(*pClient->m_callbackFunc) (pClient->nClient, XHRtspDataType_Message, "error", (unsigned char*)pClient->szCBErrorMessage, strlen(pClient->szCBErrorMessage), 0, pClient->m_pCustomerPtr);

						Rtsp_WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect()  , ������߻ص�֪ͨ��� %s   nClient = %d ", pClient->szCBErrorMessage, pClient->nClient);
 					}
 				}

				DeleteNetRevcBaseClient(nClient);//ִ��ɾ�� 
			}

			pErrorDisconnectBaseNetFifo.pop_front();
		}

		if (nDeleteBreakTimer >= 10*10)
		{//10��
			Rtsp_WriteLog(Log_Debug, "ABLLive555ClientProcessThread() ��ǰ�������� xh_ABLNetRevcBaseMap.size() = %llu ", xh_ABLNetRevcBaseMap.size());
		    nDeleteBreakTimer = 0;
		}

		nDeleteBreakTimer ++;
		nReconnectTimer ++;
		usleep(100 * OneMicroSecondTime);
	}
	ABL_bExitMediaServerRunFlag = true;
	return 0;
}

bool live555Client_Init(LIVE555RTSP_AudioVideo callbackFunc)
{ 
	if (!ABL_bInitLive555ClientFlag)
	{
		pthread_t  pDeleteBreakRtspThread, pFreeRtspClientThread;

		int ret = XHNetSDK_Init(2, 8);

		//������־
		Rtsp_InitLogFile();

 		pDisconnectBaseNetFifo.InitFifo(1024 * 1024 * 4);  //������ѵ����� 
		pReConnectStreamProxyFifo.InitFifo(1024 * 1024 * 2);//��Ҫ����������
		pErrorDisconnectBaseNetFifo.InitFifo(1024 * 1024 * 4);
	    ABL_bInitLive555ClientFlag = true ;
		NetBaseThreadPool = new CNetBaseThreadPool(32);

		pthread_create(&pDeleteBreakRtspThread, NULL, ABLLive555ClientProcessThread, (void*)NULL);
	}
	return true;
}


bool live555Client_Connect(char* szURL, bool bTCPFlag, void* pCustomerPtr, int& nOutChan)
{
	return true;
}

bool live555Client_ConnectCallBack(char* szURL, XHRtspURLType nXHRtspURLType, bool bTCPFlag, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, int& nOutChan)
{
	if (szURL == NULL || callbackFunc == NULL)
		return false;

	boost::shared_ptr<CNetRevcBase> pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxyControl, 0, 0, szURL, 0, "", pCustomerPtr, callbackFunc, 0, nXHRtspURLType);
	if (pClient != NULL)
	{
		strcpy(pClient->m_addStreamProxyStruct.url, szURL);
		pClient->SendFirstRequst();//ִ�е�һ������

		nOutChan = pClient->nClient;
		return true;
	}
	else
		return false;
}

bool live555Client_Speed(int nChan, float fSpeed)
{
	boost::shared_ptr<CNetRevcBase> pParent = NULL;
	pParent = GetNetRevcBaseClient(nChan);
	if (pParent != NULL)
	{
		boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(pParent->nMediaClient);
		if (pClient != NULL)
		{
			CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)pClient.get();
			if (pRtsp)
			  return pRtsp->RtspSpeed((int)fSpeed);
			else
			  return false;
		}
		else
			return false;
	}
	else
		return false;
}

bool live555Client_Seek(int nChan, int64_t timeStamp)
{
	boost::shared_ptr<CNetRevcBase> pParent = NULL;
	pParent = GetNetRevcBaseClient(nChan);
	if (pParent != NULL)
	{
		boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(pParent->nMediaClient) ;
		if (pClient != NULL)
		{
			CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)pClient.get();
			if (pRtsp)
				return pRtsp->RtspSeek(timeStamp);
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}

bool live555Client_Pause(int nChan)
{
	boost::shared_ptr<CNetRevcBase> pParent = NULL;
	pParent = GetNetRevcBaseClient(nChan);
	if (pParent != NULL)
	{
		boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(pParent->nMediaClient);
		if (pClient != NULL)
		{
			CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)pClient.get();
			if (pRtsp)
				return pRtsp->RtspPause();
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}

bool live555Client_Resume(int nChan)
{
	boost::shared_ptr<CNetRevcBase> pParent = NULL;
	pParent = GetNetRevcBaseClient(nChan);
	if (pParent != NULL)
	{
		boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(pParent->nMediaClient) ;
		if (pClient != NULL)
		{
			CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)pClient.get();
			if (pRtsp)
				return pRtsp->RtspResume();
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}

bool live555Client_GetNetStreamStatus(int nChan)
{
	boost::shared_ptr<CNetRevcBase> pParent = NULL;
	pParent = GetNetRevcBaseClient(nChan);
	bool               bNetStreamStatus = true;

	if (pParent != NULL)
	{
		boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(pParent->nMediaClient);
		if (pClient != NULL)
		{
			CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)pClient.get();

			if (pRtsp->m_nXHRtspURLType == XHRtspURLType_RecordPlay)
			{
				if (pRtsp->bPauseFlag)
				{//���������ͣ״̬����Ϊ������������
					  bNetStreamStatus = true ;
				}
				else
				{//�������û����ͣ�����ж������ճ�ʱ����
					if (pRtsp->nRecvDataTimerBySecond >= 30)
					{
						bNetStreamStatus = false ;
					}
				}
			}
			else
				bNetStreamStatus = true;

			return bNetStreamStatus;
		}
		else
			return false;
	}
	else
		return false;
}

bool live555Client_Disconnect(int nChan)
{
    int64_t nClient = nChan ;
	return DeleteNetRevcBaseClient(nClient);
}

bool live555Client_Cleanup()
{
	ABL_bLive555ClientRunFlag = false;
	while (!ABL_bExitMediaServerRunFlag)
		usleep(100 * OneMicroSecondTime);

	unsigned char* pData = NULL;
	int            nLength;
	uint64_t       nClient;
   	while ((pData = pDisconnectBaseNetFifo.pop(&nLength)) != NULL)
	{
		if (nLength == sizeof(nClient))
		{
			memcpy((char*)&nClient, pData, sizeof(nClient));
			if (nClient >= 0)
			{
				DeleteNetRevcBaseClient(nClient);//ִ��ɾ�� 
			}
		}

		pDisconnectBaseNetFifo.pop_front();
	}

	if (ABL_bInitLive555ClientFlag)
	{
		pDisconnectBaseNetFifo.FreeFifo();  //������ѵ����� 
		pReConnectStreamProxyFifo.FreeFifo();//��Ҫ����������
		pErrorDisconnectBaseNetFifo.FreeFifo();

		ABL_bInitLive555ClientFlag = false ;
	}
	NetBaseThreadPool->StopTask();
	delete NetBaseThreadPool;
	XHNetSDK_Deinit();

	return true;
}
