
#include "stdafx.h"
#include "live555Client.h"

typedef boost::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;
CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;
std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
CMediaFifo                                                       pDisconnectBaseNetFifo;  //清理断裂的链接 
CMediaFifo                                                       pErrorDisconnectBaseNetFifo;  //网络异常，需要断开
CMediaFifo                                                       pReConnectStreamProxyFifo; //需要重新连接代理ID 
bool                                                             ABL_bInitLive555ClientFlag = false;
bool                                                             ABL_bExitMediaServerRunFlag = false;
bool                                                             ABL_bLive555ClientRunFlag = true;
CNetBaseThreadPool*                                              NetBaseThreadPool;

CNetRevcBase_ptr CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	Rtsp_WriteLog(Log_Debug, "CreateNetRevcBaseClient(() 开始 netClientType = %d serverHandle = %llu CltHandle = %llu ",  netClientType, serverHandle, CltHandle);
	
	CNetRevcBase_ptr pXHClient = NULL;
	try
	{
		do
		{
			if (netClientType == NetRevcBaseClient_addStreamProxyControl)
			{//代理拉流控制
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetClientAddStreamProxy>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL, pCustomerPtr, callbackFunc, hParent, nXHRtspURLType);
			}
            else if (netClientType == NetRevcBaseClient_addStreamProxy)
			{//代理拉流
				if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".m3u8") != NULL)
				{//hls 暂时不支持 hls 拉流 
#if    0
					//在构造函数进行异步连接，会产生一个nClient值 
					pXHClient = boost::make_shared<CNetClientRecvHttpHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
#endif 
				}
				else if (memcmp(szIP, "rtsp://", 7) == 0)
				{//rtsp 
					pXHClient = boost::make_shared<CNetClientRecvRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL, pCustomerPtr, callbackFunc, hParent, nXHRtspURLType);
					CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
					if (CltHandle == 0)
					{//连接失败
						Rtsp_WriteLog(Log_Debug, "CreateNetRevcBaseClient()，连接 rtsp 服务器 失败 szURL = %s , szIP = %s ,port = %d ", szIP, pXHClient->m_rtspStruct.szIP, pXHClient->m_rtspStruct.szPort);
						pDisconnectBaseNetFifo.push((unsigned char*)&pXHClient->nClient, sizeof(pXHClient->nClient));
					}
				}				
				else if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".flv") != NULL)
				{//flv 
					//pXHClient = boost::make_shared<CNetClientRecvFLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					//CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
 				else if (memcmp(szIP, "rtmp://", 7) == 0)
				{//rtmp
					//pXHClient = boost::make_shared<CNetClientRecvRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					//CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
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
	
	Rtsp_WriteLog(Log_Debug, "CreateNetRevcBaseClient(() 完成 netClientType = %d serverHandle = %llu CltHandle = %llu ",  netClientType, serverHandle, CltHandle);

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

//无锁查找，在外层已经有锁
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
	Rtsp_WriteLog(Log_Debug, "DeleteNetRevcBaseClient() 用户主动断开 nClient = %llu ", CltHandle);

	CNetRevcBase_ptrMap::iterator iterator1;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
		{//rtsp 拉流，直接断开
			xh_ABLNetRevcBaseMap.erase(iterator1);
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_addStreamProxyControl)
		{//代理拉流 
			//已经成功、失败，直接删除
			if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_ConnectSuccess || ((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_ConnectFailed) 
				xh_ABLNetRevcBaseMap.erase(iterator1);
			else if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
			{//正在连接，需要收集起来，等待下一次判断 
				 pDisconnectBaseNetFifo.push((unsigned char*)&CltHandle, sizeof(CltHandle)); //收集用户主动断开的连接
				 //if (pDisconnectBaseNetFifo.push((unsigned char*)&CltHandle, sizeof(CltHandle))) //收集用户主动断开的连接
				 //	Rtsp_WriteLog(Log_Debug, "DeleteNetRevcBaseClient(正在连接，不能删除)，收集起来等待一次判断 rtspChan = %llu ", ((*iterator1).second)->nClient );
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
{ //从何这里断开的话，如果大量链接同时断开，会造成卡住，从而造成视频卡顿，要先加入链表，然后定期（500毫秒断开一路） 
	Rtsp_WriteLog(Log_Debug, "onclose() nClient = %llu 客户端断开 srvhandle = %llu", clihandle, srvhandle);

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
			//代理拉流句柄
			CNetRevcBase_ptr pParent = GetNetRevcBaseClientNoLock(pClient->m_hParent);
			if (pParent != NULL)
			{
				pParent->nRtspConnectStatus = RtspConnectStatus_ConnectFailed; //连接失败 
				strcpy(pParent->szCBErrorMessage, message_connError);
			}
			Rtsp_WriteLog(Log_Debug, "nClient = %llu ,URL: %s ,连接失败 result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
		}
		pDisconnectBaseNetFifo.push((unsigned char*)&clihandle, sizeof(clihandle));
	}
	else if (result == 1)
	{//如果链接成功，发送第一个请求
		CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(clihandle);
		if (pClient)
		{
			//代理拉流句柄
			CNetRevcBase_ptr pParent = GetNetRevcBaseClientNoLock(pClient->m_hParent);
			if (pParent != NULL)
			{
				pParent->nRtspConnectStatus = RtspConnectStatus_ConnectSuccess; //连接成功
 			}
		
			Rtsp_WriteLog(Log_Debug, "nClient = %llu ,URL: %s , 连接成功 result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
			pClient->SendFirstRequst();
		}
	}
}

//检测网络断线 ，发送M3u8请求 
int  CheckNetRevcBaseClientDisconnect()
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                           nDiconnectCount = 0;

 
	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		if (
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv ||      //代理接收Rtsp推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientRecv ||      //代理接收Rtmp推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFlvClientRecv ||   //代理接收HttpFlv推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv    //代理接收HttpHLS推流
			)
		{//现在检测 HLS 网络断线 ，还可以增加别的类型检测
			((*iterator1).second)->nRecvDataTimerBySecond ++;
			((*iterator1).second)->nSendOptionsCount ++;

			if (((*iterator1).second)->m_nXHRtspURLType == XHRtspURLType_Liveing || ((*iterator1).second)->m_nXHRtspURLType == XHRtspURLType_RecordDownload)
			{//实况、录像下载需要检测，如果断流，需要立即关闭
				if (((*iterator1).second)->nRecvDataTimerBySecond >= MaxRecvDataTimerBySecondDiconnect)
				{
					nDiconnectCount ++;
					Rtsp_WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() nClient = %llu 检测到网络异常断开 url %s ", ((*iterator1).second)->nClient, ((*iterator1).second)->m_addStreamProxyStruct.url);

					 //把父句柄的连接状态修改为连接失败
					 CNetRevcBase_ptr pParent = GetNetRevcBaseClientNoLock(((*iterator1).second)->m_hParent);
					if (pParent)
					{
						if (pParent->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
						   pParent->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;//连接失败 
 					}
                    //test 暂时测试 ，正式版本打开 
					pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
				}
			}

			//发送rtcp包
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
 			
			/*//发送options 命令 
			if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
			{
				CNetClientRecvRtsp* pRtspClient = (CNetClientRecvRtsp*)(*iterator1).second.get();
				if (((*iterator1).second)->nSendOptionsCount % 60 == 0)
				{//30秒 
					pRtspClient->SendOptions(pRtspClient->AuthenticateType, false);
 				}
			}*/

		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_addStreamProxyControl || ((*iterator1).second)->netBaseNetType == NetBaseNetType_addPushProxyControl)
		{//控制代理拉流、代理推流,检测代理拉流是否有断线

			//检测如果超过60秒，还在处于连接状态，设置为连接失败 
			if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
			{
				time_t tCurrentTime;
				time(&tCurrentTime);
				if (tCurrentTime - ((*iterator1).second)->nCreateDateTime >= 60)
				{
					((*iterator1).second)->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;
					Rtsp_WriteLog(Log_Debug, " nClient = %llu , 连接已经超过30秒，设置为连接失败，否则该对象销毁不了 ", ((*iterator1).second)->nClient);
				}
			}

			CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(((*iterator1).second)->nMediaClient);
			if (pClient == NULL)
			{//404 错误
				 if (((*iterator1).second)->bReConnectFlag == true)
				{//需要重连
					if (((*iterator1).second)->nReconnctTimeCount >= MaxReconnctTimeCount)
					{
						 strcpy(((*iterator1).second)->szCBErrorMessage, message_timeout); //连接超时

 						 Rtsp_WriteLog(Log_Debug, " nClient = %llu , nMediaClient = %llu 已经重连 %s 达到 %d 次，执行销毁 ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReconnctTimeCount);
	 
						 ((*iterator1).second)->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;//连接失败 
						
						pErrorDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					}
					else
					{
					   ((*iterator1).second)->nReconnctTimeCount ++;
					   Rtsp_WriteLog(Log_Debug, " nClient = %llu , nMediaClient = %llu 检测到网络异常断开 %s 现在执行第 %d 次重连", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReconnctTimeCount);
					  pReConnectStreamProxyFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
 					}
				}
				else
				{//rtsp 拉流已经断开 ，需要销毁代理拉流
			         time_t tCurrentTime;
				     time(&tCurrentTime);
			         if(tCurrentTime - ((*iterator1).second)->nCreateDateTime >= 3)
					 {//防止真正的rtsp连接尚未创建，就删除了代理连接 
 					    if (strlen(((*iterator1).second)->szCBErrorMessage) == 0)
						  strcpy(((*iterator1).second)->szCBErrorMessage, message_ConnectBreak);
 
						if (((*iterator1).second)->nRtspConnectStatus == RtspConnectStatus_AtConnecting)
					    ((*iterator1).second)->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;//连接失败 

					     Rtsp_WriteLog(Log_Debug, " nClient = %llu , nMediaClient = %llu , %s  拉流已经断开 ，需要销毁代理拉流 ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url);
					   
					    pErrorDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					 }
				} 
			}
		}
	}

	return nDiconnectCount;
}

//一些事务处理 
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
  		//执行删除,防止死循环
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
  						DeleteNetRevcBaseClient(nClient);//执行删除 
					}
				}

				pDisconnectBaseNetFifo.pop_front();
			}
		}
 
		//执行重连 
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
							pClient->SendFirstRequst(); //执行重连
					}
				}

				pReConnectStreamProxyFifo.pop_front();
			}
		}

		//删除网络异常的连接
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
						Rtsp_WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect()  , 网络断线回调通知开始 %s   nClient = %d ", pClient->szCBErrorMessage, pClient->nClient);

						if (pClient->m_callbackFunc != NULL)
							(*pClient->m_callbackFunc) (pClient->nClient, XHRtspDataType_Message, "error", (unsigned char*)pClient->szCBErrorMessage, strlen(pClient->szCBErrorMessage), 0, pClient->m_pCustomerPtr);

						Rtsp_WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect()  , 网络断线回调通知完成 %s   nClient = %d ", pClient->szCBErrorMessage, pClient->nClient);
 					}
 				}

				DeleteNetRevcBaseClient(nClient);//执行删除 
			}

			pErrorDisconnectBaseNetFifo.pop_front();
		}

		if (nDeleteBreakTimer >= 10*10)
		{//10秒
			Rtsp_WriteLog(Log_Debug, "ABLLive555ClientProcessThread() 当前对象总数 xh_ABLNetRevcBaseMap.size() = %llu ", xh_ABLNetRevcBaseMap.size());
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

		//启动日志
		Rtsp_InitLogFile();

 		pDisconnectBaseNetFifo.InitFifo(1024 * 1024 * 4);  //清理断裂的链接 
		pReConnectStreamProxyFifo.InitFifo(1024 * 1024 * 2);//需要重连的连接
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
		pClient->SendFirstRequst();//执行第一个命令

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
				{//如果处于暂停状态，认为网络是正常的
					  bNetStreamStatus = true ;
				}
				else
				{//如果接收没有暂停，再判断流接收超时参数
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
				DeleteNetRevcBaseClient(nClient);//执行删除 
			}
		}

		pDisconnectBaseNetFifo.pop_front();
	}

	if (ABL_bInitLive555ClientFlag)
	{
		pDisconnectBaseNetFifo.FreeFifo();  //清理断裂的链接 
		pReConnectStreamProxyFifo.FreeFifo();//需要重连的连接
		pErrorDisconnectBaseNetFifo.FreeFifo();

		ABL_bInitLive555ClientFlag = false ;
	}
	NetBaseThreadPool->StopTask();
	delete NetBaseThreadPool;
	XHNetSDK_Deinit();

	return true;
}
