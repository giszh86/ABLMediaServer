/*
功能：
   网络接收、处理基类 ，有两个纯虚函数 
   1 接收网络数据
      virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) = 0;

   2 执行处理 
      virtual int ProcessNetData() = 0;//处理网络数据，比如进行解包、发送网络数据等等

日期    2021-03-29
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetRecvBase.h"
#ifdef USE_BOOST
extern CMediaSendThreadPool* pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo;             //清理断裂的链接 
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
#else
extern CMediaSendThreadPool* pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo;             //清理断裂的链接 
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
#endif


CNetRevcBase::CNetRevcBase()
{
	m_rtspPlayerType = RtspPlayerType_Liveing;
	nCurrentVideoFrames = 0;//当前视频帧数
	nTotalVideoFrames = 0;//录像视频总帧数
	nTcp_Switch = 0;
	bSendFirstIDRFrameFlag = false;
	bRunFlag = true;
	nSSRC = 0;
	m_bSendMediaWaitForIFrame = false;
	m_bIsRtspRecordURL = false;
	m_bPauseFlag = false;
	m_nScale = 0;
	netBaseNetType = NetBaseNetType_Unknown;
	bPushMediaSuccessFlag = false;
	bProxySuccessFlag = false;
	memset(app, 0x00, sizeof(app));
	memset(stream, 0x00, sizeof(stream));

	memset(szClientIP,0x00,sizeof(szClientIP)); //连接上来的客户端IP 
	nClientPort = 0 ; //连接上来的客户端端口 
	nRecvDataTimerBySecond = 0;

	szVideoFrameHead[0] = 0x00;
	szVideoFrameHead[1] = 0x00;
	szVideoFrameHead[2] = 0x00;
	szVideoFrameHead[3] = 0x01;
	bPushSPSPPSFrameFlag = false;

	psHeadFlag[0] = 0x00;
	psHeadFlag[1] = 0x00;
	psHeadFlag[2] = 0x01;
	psHeadFlag[3] = 0xBA;

	nVideoStampAdd = 0 ;
	nAsyncAudioStamp = GetTickCount64() ;
	nCreateDateTime = nProxyDisconnectTime  = GetTickCount64();
	bRecordProxyDisconnectTimeFlag = false;

	flvPS = flvAACDts = 0;
	bUserNewAudioTimeStamp = false;
	hParent = 0;
	nPrintTime = GetTickCount64();

	nVideoFrameSpeedOrder = 0;
	oldVideoTimestamp = 0;
	bUpdateVideoFrameSpeedFlag = false;
	memset(szMediaSourceURL, 0x00, sizeof(szMediaSourceURL));
	bResponseHttpFlag = false;
	nGB28181ConnectCount = 0; 
	nReConnectingCount = 0 ;

	memset(szRecordPath, 0x00, sizeof(szRecordPath));
	nReplayClient = 0;

	nCalcVideoFrameCount = 0; //计算次数
	for(int i= 0;i<CalcMaxVideoFrameSpeed ;i++)
	 nVideoFrameSpeedArray[i] = 0;//视频帧速度数组

	nMediaSourceType = MediaSourceType_LiveMedia;//默认实况播放
	duration = 0 ;
	nClientRtcp = 0;
	nRtspRtpPayloadType = RtspRtpPayloadType_Unknow ;  //未知
	bConnectSuccessFlag = false;
	bSnapSuccessFlag = false;
	timeout_sec = 10;
	memset(domainName,0x00,sizeof(domainName)); //域名
    ifConvertFlag = false;//是否需要转换
	tUpdateIPTime = GetTickCount64();
}

CNetRevcBase::~CNetRevcBase()
{
	//关闭回放的ID，就会是否媒体源
	if (nReplayClient > 0)
		pDisconnectBaseNetFifo.push((unsigned char*)&nReplayClient, sizeof(nReplayClient));
	
	 malloc_trim(0);
}

//把域名转换为IP地址
bool   CNetRevcBase::ConvertDemainToIPAddress()
{
	if (!ifConvertFlag)
		return true;

	hostent* host = gethostbyname(domainName);
	if (host == NULL)
		return false;

	char getIP[128] = { 0 };
	for (int i = 0; host->h_addr_list[i]; i++)
	{
		memset(getIP, 0x00, sizeof(getIP));
		strcpy(getIP,inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
		if (strlen(getIP) > 0)
		{
			strcpy(m_rtspStruct.szIP, getIP);
			WriteLog(Log_Debug, "CNetRevcBase = %X ,nClient = %llu ，domainName = %s ,转换IP为 %s ", this, nClient, domainName, m_rtspStruct.szIP);
			return true;
		}
	}

	return false ;
}

//分离rtsp\rtmp\http相关参数，IP，端口，用户，密码
bool  CNetRevcBase::ParseRtspRtmpHttpURL(char* szURL)
{//rtsp://admin:szga2019@190.15.240.189:554
	int nPos1, nPos2, nPos3, nPos4, nPos5;
	string strRtspURL = szURL;
	char   szIPPort[128] = { 0 };
	string strIPPort;
	char   szSrcRtspPullUrl[1024] = { 0 };

	//全部转为小写
	strcpy(szSrcRtspPullUrl, szURL);


#ifdef USE_BOOST
	to_lower(szSrcRtspPullUrl);
#else
	ABL::StrToLwr(szSrcRtspPullUrl);
#endif

	if ( !(memcmp(szSrcRtspPullUrl, "rtsp://", 7) == 0 || memcmp(szSrcRtspPullUrl, "rtmp://", 7) == 0 || memcmp(szSrcRtspPullUrl, "http://", 7) == 0))
		return false;

	memset((char*)&m_rtspStruct, 0x00, sizeof(m_rtspStruct));
	strcpy(m_rtspStruct.szSrcRtspPullUrl, szURL);

	//查找 @ 的位置
	nPos2 = strRtspURL.rfind("@", strlen(szURL));
	if (nPos2 > 0)
	{
		m_rtspStruct.bHavePassword = true;

		nPos1 = strRtspURL.find("//", 0);
		if (nPos1 > 0)
		{
			nPos3 = strRtspURL.find(":", nPos1 + 1);
			if (nPos3 > 0)
			{
				memcpy(m_rtspStruct.szUser, m_rtspStruct.szSrcRtspPullUrl + nPos1 + 2, nPos3 - nPos1 - 2);
				memcpy(m_rtspStruct.szPwd, m_rtspStruct.szSrcRtspPullUrl + nPos3 + 1, nPos2 - nPos3 - 1);

				//查找 / ,分离出IP，端口
				nPos4 = strRtspURL.find("/", nPos2 + 1);
				if (nPos4 > 0)
				{
					memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos2 + 1, nPos4 - nPos2 - 1);
				}
				else
				{
					memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos2 + 1, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2);
				}

				strIPPort = szIPPort;
				nPos5 = strIPPort.find(":", 0);
				if (nPos5 > 0)
				{//有指定端口
					memcpy(m_rtspStruct.szIP, szIPPort, nPos5);
					memcpy(m_rtspStruct.szPort, szIPPort + nPos5 + 1, strlen(szIPPort) - nPos5 - 1);
				}
				else
				{//没有指定端口
					strcpy(m_rtspStruct.szIP, szIPPort);
					if (memcmp(szSrcRtspPullUrl, "rtsp://",7) == 0)
					   strcpy(m_rtspStruct.szPort, "554");
					else  if (memcmp(szSrcRtspPullUrl, "rtmp://", 7) == 0)
						strcpy(m_rtspStruct.szPort, "1935");
					else  if (memcmp(szSrcRtspPullUrl, "http://", 7) == 0)
						strcpy(m_rtspStruct.szPort, "80");
				}
			}
		}

		//回复的时候去掉用户，密码
		memset(szSrcRtspPullUrl, 0x00, sizeof(szSrcRtspPullUrl));
		strcpy(szSrcRtspPullUrl, "rtsp://");
		memcpy(szSrcRtspPullUrl + 7, m_rtspStruct.szSrcRtspPullUrl + (nPos2 + 1), strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2 - 1);

		memset(m_rtspStruct.szSrcRtspPullUrl, 0x00, sizeof(m_rtspStruct.szSrcRtspPullUrl));
		strcpy(m_rtspStruct.szSrcRtspPullUrl, szSrcRtspPullUrl);
	}
	else
	{
		m_rtspStruct.bHavePassword = false;

		nPos1 = strRtspURL.find("//", 0);
		if (nPos1 > 0)
		{
			nPos2 = strRtspURL.find("/", nPos1 + 2);

			//查找 / ,分离出IP，端口
			if (nPos2 > 0)
			{
				memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos1 + 2, nPos2 - nPos1 - 2);
			}
			else
			{
				memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos1 + 2, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos1 - 2);
			}

			strIPPort = szIPPort;
			nPos5 = strIPPort.find(":", 0);
			if (nPos5 > 0)
			{//有指定端口
				memcpy(m_rtspStruct.szIP, szIPPort, nPos5);
				memcpy(m_rtspStruct.szPort, szIPPort + nPos5 + 1, strlen(szIPPort) - nPos5 - 1);
			}
			else
			{//没有指定端口
				strcpy(m_rtspStruct.szIP, szIPPort);
				if (memcmp(szSrcRtspPullUrl, "rtsp://", 6) == 0)
					strcpy(m_rtspStruct.szPort, "554");
				else  if (memcmp(szSrcRtspPullUrl, "rtmp://", 6) == 0)
					strcpy(m_rtspStruct.szPort, "1935");
				else  if (memcmp(szSrcRtspPullUrl, "http://", 6) == 0)
					strcpy(m_rtspStruct.szPort, "80");
			}
		}
	}

	nPos1 = strRtspURL.find("://", 0);
	if (nPos1 > 0)
	{
		nPos2 = strRtspURL.find("/", nPos1 + 4);
		if (nPos2 > 0)
		{
			memcpy(m_rtspStruct.szRequestFile, szURL + nPos2 , strlen(szURL) - nPos2 - 1);
		}
	}	

	if (strlen(m_rtspStruct.szIP) == 0 || strlen(m_rtspStruct.szPort) == 0)
	{
		return false;
	}
	else
	{
		//拷贝域名，判断是否需要转换为IP
		strcpy(domainName, m_rtspStruct.szIP);
		string strDomainName = m_rtspStruct.szIP;
#ifdef USE_BOOST
		replace_all(strDomainName, ".", "");
		if (!boost::all(strDomainName, boost::is_digit()))
#else
		ABL::replace_all(strDomainName, ".", "");
		if (!ABL::is_digits(strDomainName))
#endif
		{//不是数字，需要域名转换为IP
			ifConvertFlag = true;

			if (!ConvertDemainToIPAddress())
			{
				WriteLog(Log_Debug, "CNetRevcBase = %X ,nClient = %llu ，domainName = %s ,域名转为IP 失败 ", this,nClient,domainName);
				return false;
			}
		}

		nPos5 = strRtspURL.find("?", 0);
		if (nPos5 > 0)
			memcpy(m_rtspStruct.szRtspURLTrim, szURL, nPos5);
		else
			strcpy(m_rtspStruct.szRtspURLTrim, szURL);

		return true;
	}
}

/*
检测视频是否是I帧
*/
bool  CNetRevcBase::CheckVideoIsIFrame(char* szVideoName,unsigned char* szPVideoData, int nPVideoLength)
{
	int nPos = 0;
	bool bVideoIsIFrameFlag = false;
	unsigned char  nFrameType = 0x00;

	for (int i = 0; i< nPVideoLength; i++)
	{
		if (memcmp(szPVideoData + i, szVideoFrameHead, 4) == 0)
		{//找到帧片段
			if (strcmp(szVideoName, "H264") == 0)
			{
				nFrameType = (szPVideoData[i + 4] & 0x1F);
				if (nFrameType == 7 || nFrameType == 8 || nFrameType == 5)
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}
			else if (strcmp(szVideoName, "H265") == 0)
			{
				nFrameType = (szPVideoData[i + 4] & 0x7E) >> 1;
				if ((nFrameType >= 16 && nFrameType <= 21) || (nFrameType >= 32 && nFrameType <= 34))
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}
		}

		//不需要全部检查完毕，就可以判断一帧类型
		if (i >= 256)
			return false;
 	}

	return bVideoIsIFrameFlag;
}

void  CNetRevcBase::SyncVideoAudioTimestamp()
{
	//500毫秒同步一次 
	if (GetTickCount() - nAsyncAudioStamp >= 500)
	{
		if (flvPS < flvAACDts)
		{
			nVideoStampAdd = (1000 / mediaCodecInfo.nVideoFrameRate ) + 5 ;
		}
		else if (flvPS > flvAACDts)
		{
			nVideoStampAdd = (1000 / mediaCodecInfo.nVideoFrameRate) - 5 ;
		}
		nAsyncAudioStamp = GetTickCount();

		//WriteLog(Log_Debug, "CMediaStreamSource = %X flvPS = %d ,flvAACDts = %d ", this, flvPS, flvAACDts);
	}
}

//计算视频帧速度
int  CNetRevcBase::CalcVideoFrameSpeed(unsigned char* pRtpData, int nLength)
{
	if (pRtpData == NULL)
		return -1 ;
	
	int nVideoFrameSpeed = 25 ;
 	memcpy((char*)&rtp_header, pRtpData, sizeof(rtp_header));
	if (oldVideoTimestamp == 0)
	{
		oldVideoTimestamp = ntohl(rtp_header.timestamp);
	}
	else
	{
		if (ntohl(rtp_header.timestamp) != oldVideoTimestamp && ntohl(rtp_header.timestamp) > oldVideoTimestamp )
		{
			//WriteLog(Log_Debug, "this = %X ,nVideoFrameSpeed = %llu ", this,(90000 / (ntohl(rtp_header.timestamp) - oldVideoTimestamp)) );

			nVideoFrameSpeed = 90000 / (ntohl(rtp_header.timestamp) - oldVideoTimestamp) ;
			if (nVideoFrameSpeed > 30)
				nVideoFrameSpeed = 30;

			//妈的，有些时间戳乱搞打包，强制赋值为25帧每秒 
			if (nVideoFrameSpeed <= 5)
				nVideoFrameSpeed = 25;

			oldVideoTimestamp = ntohl(rtp_header.timestamp);
			nVideoFrameSpeedOrder ++;
			if (nVideoFrameSpeedOrder < 10)
				return -1;
			else
			   return nVideoFrameSpeed;
		}
		return -1;
	}
	return -1;
}

//计算flv的视频帧速度
int   CNetRevcBase::CalcFlvVideoFrameSpeed(int nVideoPTS, int nMaxValue)
{
	int nVideoFrameSpeed = 25;
	if (oldVideoTimestamp == 0)
	{
		oldVideoTimestamp = nVideoPTS;
	}
	else
	{
		if (nVideoPTS != oldVideoTimestamp && nVideoPTS > oldVideoTimestamp)
		{
			nVideoFrameSpeed = nMaxValue / (nVideoPTS  - oldVideoTimestamp);
			if (nVideoFrameSpeed > 30)
				nVideoFrameSpeed = 30;

			oldVideoTimestamp = nVideoPTS;
			nVideoFrameSpeedOrder ++;
			//WriteLog(Log_Debug, "this = %X ,nVideoFrameSpeed = %llu ", this, nVideoFrameSpeed );
			if (nVideoFrameSpeedOrder < 10)
				return -1;
			else
			{
				if (nCalcVideoFrameCount >= CalcMaxVideoFrameSpeed)
					return m_nVideoFrameSpeed;

 				nVideoFrameSpeedArray[nCalcVideoFrameCount] = nVideoFrameSpeed;//视频帧速度数组
 				nCalcVideoFrameCount ++ ; //计算次数

				if (nCalcVideoFrameCount >= CalcMaxVideoFrameSpeed)
				{
					double dCount = 0;
					double dArrayCount = CalcMaxVideoFrameSpeed;
					for (int i = 0; i < CalcMaxVideoFrameSpeed; i++)
						dCount += nVideoFrameSpeedArray[i];

					double dDec = dCount / dArrayCount ;
					m_nVideoFrameSpeed = dCount / dArrayCount;
					double dJian = dDec - m_nVideoFrameSpeed;
					
					if (dJian > 0.5)
						m_nVideoFrameSpeed += 1;

					if (m_nVideoFrameSpeed >= 24 && m_nVideoFrameSpeed <= 29)
						m_nVideoFrameSpeed = 25;
					else if (m_nVideoFrameSpeed >= 29)
						m_nVideoFrameSpeed = 30;

					return m_nVideoFrameSpeed;
				}
				else
					return -1;
			}
		}
		return -1;
	}
	return -1;
}

//切割app ,stream 
bool  CNetRevcBase::SplitterAppStream(char* szMediaSoureFile)
{
	if (szMediaSoureFile == NULL || szMediaSoureFile[0] != '/' )
		return false;
	string strMediaSource = szMediaSoureFile;
	int  nPos2;

	nPos2 = strMediaSource.find("/", 1);
	if (nPos2 < 0)
		return false;

	strcpy(szMediaSourceURL, szMediaSoureFile);
	memset(m_addStreamProxyStruct.app, 0x00, sizeof(m_addStreamProxyStruct.app));
	memset(m_addStreamProxyStruct.stream, 0x00, sizeof(m_addStreamProxyStruct.stream));

	memcpy(m_addStreamProxyStruct.app, szMediaSoureFile+1, nPos2 - 1 );
	memcpy(m_addStreamProxyStruct.stream, szMediaSoureFile + nPos2 +1 ,strlen(szMediaSoureFile) - nPos2 - 1);

	return true;
}

//回复成功信息
bool  CNetRevcBase::ResponseHttp(uint64_t nHttpClient,char* szSuccessInfo,bool bClose)
{
	//功能实现类已经回复
	bResponseHttpFlag = true;


#ifdef USE_BOOST
	boost::shared_ptr<CNetRevcBase>  pClient = GetNetRevcBaseClientNoLock(nHttpClient);
	if (pClient == NULL)
		return true;
	if (pClient->bResponseHttpFlag)
		return true;

	//回复http请求
	string strReponseError = szSuccessInfo;
	replace_all(strReponseError, "\r\n", " ");
#else
	auto  pClient = GetNetRevcBaseClientNoLock(nHttpClient);
	if (pClient == NULL)
		return true;
	if (pClient->bResponseHttpFlag)
		return true;

	//回复http请求
	string strReponseError = szSuccessInfo;
	ABL::replace_all(strReponseError, "\r\n", " ");
#endif

	strcpy(szSuccessInfo, strReponseError.c_str());

	int nLength = strlen(szSuccessInfo);
	if(bClose == true)
	  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nLength);
	else
	  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nLength);
 
	XHNetSDK_Write(nHttpClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), 1);
	XHNetSDK_Write(nHttpClient, (unsigned char*)szSuccessInfo, nLength, 1);

	pClient->bResponseHttpFlag = true;

	WriteLog(Log_Debug, szSuccessInfo);
	return true;
}

//回复成功信息
bool  CNetRevcBase::ResponseHttp2(uint64_t nHttpClient, char* szSuccessInfo, bool bClose)
{
	if (bResponseHttpFlag)
		return false;

	//功能实现类已经回复
	bResponseHttpFlag = true;

	//回复http请求
	string strReponseError = szSuccessInfo;
#ifdef USE_BOOST
	replace_all(strReponseError, "\r\n", " ");
#else
	ABL::replace_all(strReponseError, "\r\n", " ");
#endif

	strcpy(szSuccessInfo, strReponseError.c_str());

	int nLength = strlen(szSuccessInfo);
	if (bClose == true)
		sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nLength);
	else
		sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nLength);

	XHNetSDK_Write(nHttpClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), 1);
	XHNetSDK_Write(nHttpClient, (unsigned char*)szSuccessInfo, nLength, 1);

	WriteLog(Log_Debug, szSuccessInfo);
	return true;
}

//回复图片
bool  CNetRevcBase::ResponseImage(uint64_t nHttpClient, HttpImageType imageType,unsigned char* pImageBuffer, int nImageLength, bool bClose)
{
	std::lock_guard<std::mutex> lock(httpResponseLock);

 	if (bClose == true)
	{
		if(imageType == HttpImageType_jpeg)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/jpeg;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nImageLength);
		else if (imageType == HttpImageType_png)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/png;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nImageLength);
	}
	else
	{
		if (imageType == HttpImageType_jpeg)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/jpeg;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nImageLength);
		else if (imageType == HttpImageType_png)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/png;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nImageLength);
	}

	XHNetSDK_Write(nHttpClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), 1);

	int nPos = 0;
	int nWriteRet ;
	while (nImageLength > 0 && pImageBuffer != NULL)
	{
		if (nImageLength > Send_ImageFile_MaxPacketCount)
		{
			nWriteRet = XHNetSDK_Write(nHttpClient, (unsigned char*)pImageBuffer + nPos, Send_ImageFile_MaxPacketCount, 1);
			nImageLength -= Send_ImageFile_MaxPacketCount;
			nPos += Send_ImageFile_MaxPacketCount;
		}
		else
		{
			nWriteRet = XHNetSDK_Write(nHttpClient, (unsigned char*)pImageBuffer + nPos, nImageLength, 1);
			nPos += nImageLength;
			nImageLength = 0;
		}

		if (nWriteRet != 0)
		{//发送出错
  			WriteLog(Log_Debug, "CNetRevcBase = %X nHttpClient = %llu  发送图片出错，准备删除 ", this, nHttpClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nHttpClient, sizeof(nHttpClient));
 			return false  ;
 		}
	}
 
 	return true;
}

//url解码 
bool CNetRevcBase::DecodeUrl(char *Src, char  *url, int  MaxLen)  
{  
    if(NULL == url || NULL == Src)  
    {  
        return false;  
    }  
    if(MaxLen == 0)  
    {  
        return false;  
    }  

    char  *p = Src;  // 用来循环  
    int    i = 0;    // i用来控制url数组  

    /* 临时保存url编码的数据
       例如: %1A%2B%3C
    */  
    char  t = '\0';  
    while(*p != '\0' && MaxLen--)  
    {  
        if(*p == 0x25) // 0x25 = '%'  
        {  
            /* 以下是十六进制数中常出现的大写字母,小写字母,数字的判断 */  
            if(p[1] >= 'A' && p[1] <= 'Z') // 大写字母  
            {  
                t = p[1] - 'A' + 10;  // A = 10,下同  
            }  
            else if(p[1] >= 'a' && p[1] <= 'z') // 小写字母  
            {  
                t = p[1] - 'a' + 10;  
            }  
            else if(p[1] >= '0' && p[1] <= '9') // 数字  
            {  
                t = p[1] - '0';  
            }  

            t *= 16;  // 将数放到十位上去  

            if(p[2] >= 'A' && p[2] <= 'Z') // 大写字母  
            {  
                t += p[2] - 'A' + 10;  
            }  
            else if(p[2] >= 'a' && p[2] <= 'z') // 小写字母  
            {  
                t += p[2] - 'a' + 10;  
            }  
            else if(p[2] >= '0' && p[2] <= '9') // 数字  
            {  
                t += p[2] - '0';  
            }  

            // 到此合成了一个十六进制数  
            url[i] = t;  
            p += 3, i++;  
        }  
        else  
        {  
            // 没有被url编码的数据  
            // '+'特殊处理.它相当于一个空格  
            if(*p != '+')  
            {  
                url[i] = *p;  
            }  
            else  
            {  
                url[i] = *p;//+号，还是原来的字符输出，不要修改为 空格，否则华为的url会破坏 
            }  
            i++;  
            p++;  
        }  
    }  
    url[i] = '\0';  // 结束符  
    return true;  
}  

//查找SPS出现的位置
unsigned  int  CNetRevcBase::FindSpsPosition(unsigned char* szVideoBuffer, int nBufferLength, bool &bFind)
{
	unsigned int nPos = 0;
	unsigned char H265HeadFlag[4] = { 0x00,0x00,0x00,0x01 };
	unsigned char nFrameTypeH265, nTempFrame;
	bFind = false;
	for (int i = 0; i < nBufferLength; i++)
	{
		if (memcmp(H265HeadFlag, szVideoBuffer + i, 4) == 0)
		{
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			{
				nTempFrame = (szVideoBuffer[i + 4] & 0x1F);
				if (nTempFrame == 7)
				{
					nPos = i;
					bFind = true;
					break;
				}
			}
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			{
				nTempFrame = szVideoBuffer[i + 4];
				nFrameTypeH265 = (nTempFrame & 0x7E) >> 1;
				if (nFrameTypeH265 == 33)
				{//SPS帧
					nPos = i;
					bFind = true;
					break;
				}
			}
		}
	}

	return nPos + 4;
}

//获取H265相关信息
bool  CNetRevcBase::ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params)
{
	if (size < 20)
	{
		return false;
	}
	NALBitstream bs(data, size);
	// seq_parameter_set_rbsp()  
	bs.GetWord(4);// sps_video_parameter_set_id  
	int sps_max_sub_layers_minus1 = bs.GetWord(3);
	if (sps_max_sub_layers_minus1 > 6)
	{
		return false;
	}
	bs.GetWord(1);
	{
		bs.GetWord(2);
		bs.GetWord(1);
		params.profile = bs.GetWord(5);
		bs.GetWord(32);//  
		bs.GetWord(1);//   
		bs.GetWord(1);//   
		bs.GetWord(1);//   
		bs.GetWord(1);//    
		bs.GetWord(44);//   
		params.level = bs.GetWord(8);// general_level_idc  
		uint8 sub_layer_profile_present_flag[6] = { 0 };
		uint8 sub_layer_level_present_flag[6] = { 0 };
		for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = bs.GetWord(1);
			sub_layer_level_present_flag[i] = bs.GetWord(1);
		}
		if (sps_max_sub_layers_minus1 > 0)
		{
			for (int i = sps_max_sub_layers_minus1; i < 8; i++) {
				uint8 reserved_zero_2bits = bs.GetWord(2);
			}
		}
		for (int i = 0; i < sps_max_sub_layers_minus1; i++)
		{
			if (sub_layer_profile_present_flag[i]) {
				bs.GetWord(2);
				bs.GetWord(1);
				bs.GetWord(5);
				bs.GetWord(32);
				bs.GetWord(1);
				bs.GetWord(1);
				bs.GetWord(1);
				bs.GetWord(1);
				bs.GetWord(44);
			}
			if (sub_layer_level_present_flag[i]) {
				bs.GetWord(8);// sub_layer_level_idc[i]  
			}
		}
	}
	uint32_t sps_seq_parameter_set_id = bs.GetUE();
	if (sps_seq_parameter_set_id > 15) {
		return false;
	}
	uint32_t chroma_format_idc = bs.GetUE();
	if (sps_seq_parameter_set_id > 3) {
		return false;
	}
	if (chroma_format_idc == 3) {
		bs.GetWord(1);//    
	}
	params.width = bs.GetUE(); // pic_width_in_luma_samples  
	params.height = bs.GetUE(); // pic_height_in_luma_samples  
	if (bs.GetWord(1)) {
		bs.GetUE();
		bs.GetUE();
		bs.GetUE();
		bs.GetUE();
	}
	uint32_t bit_depth_luma_minus8 = bs.GetUE();
	uint32_t bit_depth_chroma_minus8 = bs.GetUE();
	if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
		return false;
	}

	//修正高度  
	if (params.height == 1088)
		params.height = 1080;

	return true;
}

//从SPS获取宽、高
bool  CNetRevcBase::GetWidthHeightFromSPS(unsigned char* szSPS, int nSPSLength, int& nWidth, int& nHeight)
{
	s.p = (uint8_t *)szSPS;//在这里,_Resolution是经过BASE64解码后的SPS数据的指针
	s.p_start = (uint8_t *)szSPS;
	s.p_end = (uint8_t *)(szSPS + nSPSLength);//size是SPS数据的长度
	s.i_left = 8;//这个是固定的.是指SPS的对齐宽度

	if (_spsreader.Do_Read_SPS(&s, &nWidth, &nHeight) != 0)
	{
		nWidth = 0;
		nHeight = 0;
		return false ;
	}

	//修复计算出来的高
	if (nWidth == 1920 && nHeight == 1088)
	{
		nHeight = 1080;
	}
	else if (nWidth == 1920 && nHeight == 544)
	{//修正分析错误的分辨率 85038坪葵路金葵中路路口
		nHeight = 1080;
	}
	else if (nWidth == 4096 && nHeight == 1808)
	{
		nHeight = 1800;
	}
	return true ;
}

//根据录像点播的url查询录像文件是否存在 
bool   CNetRevcBase::QueryRecordFileIsExiting(char* szReplayRecordFileURL)
{
	if (strlen(szMediaSourceURL) <= 0)
		return false;

	memset(szSplliterShareURL, 0x00, sizeof(szSplliterShareURL));//录像点播时切割的url 
	memset(szReplayRecordFile, 0x00, sizeof(szReplayRecordFile));//录像点播切割的录像文件名字 
	memset(szSplliterApp, 0x00, sizeof(szSplliterApp));
	memset(szSplliterStream, 0x00, sizeof(szSplliterStream));
 	string strRequestMediaSourceURL = szReplayRecordFileURL;
	int   nPos = strRequestMediaSourceURL.find(RecordFileReplaySplitter, 0);
	if (nPos <= 0)
  		return false ;

 	memcpy(szSplliterShareURL, szMediaSourceURL, nPos);
	memcpy(szReplayRecordFile, szMediaSourceURL + (nPos + strlen(RecordFileReplaySplitter)), strlen(szMediaSourceURL) - nPos - strlen(RecordFileReplaySplitter));

	if (QureyRecordFileFromRecordSource(szSplliterShareURL, szReplayRecordFile) == false)
 		return false ;

	int   nPos2 = strRequestMediaSourceURL.find("/", 2);
	if (nPos2 > 0)
	{
		memcpy(szSplliterApp, szReplayRecordFileURL + 1, nPos2 -1 );
		memcpy(szSplliterStream, szReplayRecordFileURL + nPos2 + 1, nPos - nPos2 -1 );
	}

	return true;
}
#ifdef USE_BOOST
//根据录像文件创建点播的录像媒体源
boost::shared_ptr<CMediaStreamSource>   CNetRevcBase::CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient)
{
#ifdef OS_System_Windows
	sprintf(szRequestReplayRecordFile, "%s%s\\%s\\%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#else
	sprintf(szRequestReplayRecordFile, "%s%s/%s/%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#endif

	boost::shared_ptr<CMediaStreamSource> pTempSource = GetMediaStreamSource(szReplayURL);
	if (pTempSource == NULL)
	{
		boost::shared_ptr<CNetRevcBase> replayClient = CreateNetRevcBaseClient(ReadRecordFileInput_ReadFMP4File, 0, 0, szRequestReplayRecordFile, 0, szSplliterShareURL);
		if (replayClient)//记录录像点播的client 
			*nReturnReplayClient = replayClient->nClient;

		pTempSource = GetMediaStreamSource(szReplayURL);
		if (pTempSource == NULL)
		{
			if (replayClient)
				pDisconnectBaseNetFifo.push((unsigned char*)&replayClient->nClient, sizeof(replayClient->nClient));
			return NULL;
		}
		int nWaitCount = 0;
		while (!pTempSource->bUpdateVideoSpeed)
		{
			nWaitCount++;
			Sleep(200);
			if (nWaitCount >= 10)
				break;
		}
	}
	nMediaSourceType = MediaSourceType_ReplayMedia;
	duration = pTempSource->nMediaDuration;

	return  pTempSource;
}
#else
//根据录像文件创建点播的录像媒体源
std::shared_ptr<CMediaStreamSource>   CNetRevcBase::CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient)
{
#ifdef OS_System_Windows
	sprintf(szRequestReplayRecordFile, "%s%s\\%s\\%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#else
	sprintf(szRequestReplayRecordFile, "%s%s/%s/%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#endif

	std::shared_ptr<CMediaStreamSource> pTempSource = GetMediaStreamSource(szReplayURL);
	if (pTempSource == NULL)
	{
		std::shared_ptr<CNetRevcBase> replayClient = CreateNetRevcBaseClient(ReadRecordFileInput_ReadFMP4File, 0, 0, szRequestReplayRecordFile, 0, szSplliterShareURL);
		if (replayClient)//记录录像点播的client 
			*nReturnReplayClient = replayClient->nClient;

		pTempSource = GetMediaStreamSource(szReplayURL);
		if (pTempSource == NULL)
		{
			if (replayClient)
				pDisconnectBaseNetFifo.push((unsigned char*)&replayClient->nClient, sizeof(replayClient->nClient));
			return NULL;
		}
		int nWaitCount = 0;
		while (!pTempSource->bUpdateVideoSpeed)
		{
			nWaitCount++;
			Sleep(200);
			if (nWaitCount >= 10)
				break;
		}
	}
	nMediaSourceType = MediaSourceType_ReplayMedia;
	duration = pTempSource->nMediaDuration;

	return  pTempSource;
}
#endif

