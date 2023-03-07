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

CNetRevcBase::CNetRevcBase()
{
	VideoFrameSpeed = 25;//暂时固定为25帧 
	nPushVideoFrameCount = 0;//单位时间内加入视频帧总数 
	nCalcFrameSpeedStartTime = 0; //计算帧速度开始时间
	nCalcFrameSpeedEndTime = 0;  //计算帧速度结束时间
	nCalcFrameSpeedCount = 0;   //已经计算视频帧速度次数
	nSendOptionsCount = 0;

	netBaseNetType = NetBaseNetType_Unknown;
	bPushMediaSuccessFlag = false;

	memset(szClientIP,0x00,sizeof(szClientIP)); //连接上来的客户端IP 
	nClientPort = 0 ; //连接上来的客户端端口 
	nRecvDataTimerBySecond = 0;

	szVideoFrameHead[0] = 0x00;
	szVideoFrameHead[1] = 0x00;
	szVideoFrameHead[2] = 0x00;
	szVideoFrameHead[3] = 0x01;
	bPushSPSPPSFrameFlag = false;

	nVideoStampAdd = 40;
	nAsyncAudioStamp = -1;

	bRecordProxyDisconnectTimeFlag = false;

	m_hParent = 0;
	m_callbackFunc = NULL;
	bReConnectFlag = false;
	nReconnctTimeCount = 0;
	memset(szCBErrorMessage, 0x00, sizeof(szCBErrorMessage));
}

CNetRevcBase::~CNetRevcBase()
{

}

void CNetRevcBase::CalcVideoFrameSpeed()
{
	if (nCalcFrameSpeedCount >= 30)
		return; //视频帧速度已经平稳，不需要再计算 

	nPushVideoFrameCount ++;//单位时间内加入视频帧总数 
	//if (abs(nCalcFrameSpeedStartTime - 0) < 0.001)
	//	nCalcFrameSpeedStartTime = ::GetTickCount();//计算帧速度开始时间
	
	//够5秒 ，计算视频帧速度 												
	if (nPushVideoFrameCount >= 25 * 5)
	{
		//nCalcFrameSpeedEndTime = ::GetTickCount();  //计算帧速度结束时间
		TempVideoFrameSpeed = (nPushVideoFrameCount / (nCalcFrameSpeedEndTime - nCalcFrameSpeedStartTime)) * 1000.00;

		if (abs(TempVideoFrameSpeed - VideoFrameSpeed) >= 5)
		{//帧速度误差有5帧时，才修正视频帧速度，否则会操作VLC播放声音丢失 
			VideoFrameSpeed =  TempVideoFrameSpeed;
			Rtsp_WriteLog(Log_Debug, "CNetRevcBase= %X,视频帧速度有变化，前后速度误差超过5帧，需要修正视频帧速度 nClient = %llu,TempVideoFrameSpeed = %d,  VideoFrameSpeed = %d ", this, nClient, TempVideoFrameSpeed,VideoFrameSpeed);
		}

		nPushVideoFrameCount = 0;
		nCalcFrameSpeedStartTime = 0;
		nCalcFrameSpeedCount ++;//累计 计算视频帧速度次数 
	}
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
	to_lower(szSrcRtspPullUrl);

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
				else
					break;//非I帧 
			}
			else if (strcmp(szVideoName, "H265") == 0)
			{
				nFrameType = (szPVideoData[i + 4] & 0x7E) >> 1;
				if ((nFrameType >= 16 && nFrameType <= 21) || (nFrameType >= 32 && nFrameType <= 34))
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
				else
					break;//非I帧 
			}
		}
	}

	return bVideoIsIFrameFlag;
}

//url解码 
bool CNetRevcBase::DecodeUrl(char *Src, char  *url, int  MaxLen)
{
	if (NULL == url || NULL == Src)
	{
		return false;
	}
	if (MaxLen == 0)
	{
		return false;
	}

	char  *p = Src;  // 用来循环  
	int    i = 0;    // i用来控制url数组  

	/* 临时保存url编码的数据
	例如: %1A%2B%3C
	*/
	char  t = '\0';
	while (*p != '\0' && MaxLen--)
	{
		if (*p == 0x25) // 0x25 = '%'  
		{
			/* 以下是十六进制数中常出现的大写字母,小写字母,数字的判断 */
			if (p[1] >= 'A' && p[1] <= 'Z') // 大写字母  
			{
				t = p[1] - 'A' + 10;  // A = 10,下同  
			}
			else if (p[1] >= 'a' && p[1] <= 'z') // 小写字母  
			{
				t = p[1] - 'a' + 10;
			}
			else if (p[1] >= '0' && p[1] <= '9') // 数字  
			{
				t = p[1] - '0';
			}

			t *= 16;  // 将数放到十位上去  

			if (p[2] >= 'A' && p[2] <= 'Z') // 大写字母  
			{
				t += p[2] - 'A' + 10;
			}
			else if (p[2] >= 'a' && p[2] <= 'z') // 小写字母  
			{
				t += p[2] - 'a' + 10;
			}
			else if (p[2] >= '0' && p[2] <= '9') // 数字  
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
			if (*p != '+')
			{
				url[i] = *p;
			}
			else
			{
				url[i] = *p;//+ 号还是不变输出
			}
			i++;
			p++;
		}
	}
	url[i] = '\0';  // 结束符  
	return true;
}
