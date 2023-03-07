/*
���ܣ�
   ������ա�������� �����������麯�� 
   1 ������������
      virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) = 0;

   2 ִ�д��� 
      virtual int ProcessNetData() = 0;//�����������ݣ�������н���������������ݵȵ�

����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetRecvBase.h"

CNetRevcBase::CNetRevcBase()
{
	VideoFrameSpeed = 25;//��ʱ�̶�Ϊ25֡ 
	nPushVideoFrameCount = 0;//��λʱ���ڼ�����Ƶ֡���� 
	nCalcFrameSpeedStartTime = 0; //����֡�ٶȿ�ʼʱ��
	nCalcFrameSpeedEndTime = 0;  //����֡�ٶȽ���ʱ��
	nCalcFrameSpeedCount = 0;   //�Ѿ�������Ƶ֡�ٶȴ���
	nSendOptionsCount = 0;

	netBaseNetType = NetBaseNetType_Unknown;
	bPushMediaSuccessFlag = false;

	memset(szClientIP,0x00,sizeof(szClientIP)); //���������Ŀͻ���IP 
	nClientPort = 0 ; //���������Ŀͻ��˶˿� 
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
		return; //��Ƶ֡�ٶ��Ѿ�ƽ�ȣ�����Ҫ�ټ��� 

	nPushVideoFrameCount ++;//��λʱ���ڼ�����Ƶ֡���� 
	//if (abs(nCalcFrameSpeedStartTime - 0) < 0.001)
	//	nCalcFrameSpeedStartTime = ::GetTickCount();//����֡�ٶȿ�ʼʱ��
	
	//��5�� ��������Ƶ֡�ٶ� 												
	if (nPushVideoFrameCount >= 25 * 5)
	{
		//nCalcFrameSpeedEndTime = ::GetTickCount();  //����֡�ٶȽ���ʱ��
		TempVideoFrameSpeed = (nPushVideoFrameCount / (nCalcFrameSpeedEndTime - nCalcFrameSpeedStartTime)) * 1000.00;

		if (abs(TempVideoFrameSpeed - VideoFrameSpeed) >= 5)
		{//֡�ٶ������5֡ʱ����������Ƶ֡�ٶȣ���������VLC����������ʧ 
			VideoFrameSpeed =  TempVideoFrameSpeed;
			Rtsp_WriteLog(Log_Debug, "CNetRevcBase= %X,��Ƶ֡�ٶ��б仯��ǰ���ٶ�����5֡����Ҫ������Ƶ֡�ٶ� nClient = %llu,TempVideoFrameSpeed = %d,  VideoFrameSpeed = %d ", this, nClient, TempVideoFrameSpeed,VideoFrameSpeed);
		}

		nPushVideoFrameCount = 0;
		nCalcFrameSpeedStartTime = 0;
		nCalcFrameSpeedCount ++;//�ۼ� ������Ƶ֡�ٶȴ��� 
	}
}

//����rtsp\rtmp\http��ز�����IP���˿ڣ��û�������
bool  CNetRevcBase::ParseRtspRtmpHttpURL(char* szURL)
{//rtsp://admin:szga2019@190.15.240.189:554
	int nPos1, nPos2, nPos3, nPos4, nPos5;
	string strRtspURL = szURL;
	char   szIPPort[128] = { 0 };
	string strIPPort;
	char   szSrcRtspPullUrl[1024] = { 0 };

	//ȫ��תΪСд
	strcpy(szSrcRtspPullUrl, szURL);
	to_lower(szSrcRtspPullUrl);

	if ( !(memcmp(szSrcRtspPullUrl, "rtsp://", 7) == 0 || memcmp(szSrcRtspPullUrl, "rtmp://", 7) == 0 || memcmp(szSrcRtspPullUrl, "http://", 7) == 0))
		return false;

	memset((char*)&m_rtspStruct, 0x00, sizeof(m_rtspStruct));
	strcpy(m_rtspStruct.szSrcRtspPullUrl, szURL);

	//���� @ ��λ��
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

				//���� / ,�����IP���˿�
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
				{//��ָ���˿�
					memcpy(m_rtspStruct.szIP, szIPPort, nPos5);
					memcpy(m_rtspStruct.szPort, szIPPort + nPos5 + 1, strlen(szIPPort) - nPos5 - 1);
				}
				else
				{//û��ָ���˿�
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

		//�ظ���ʱ��ȥ���û�������
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

			//���� / ,�����IP���˿�
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
			{//��ָ���˿�
				memcpy(m_rtspStruct.szIP, szIPPort, nPos5);
				memcpy(m_rtspStruct.szPort, szIPPort + nPos5 + 1, strlen(szIPPort) - nPos5 - 1);
			}
			else
			{//û��ָ���˿�
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
�����Ƶ�Ƿ���I֡
*/
bool  CNetRevcBase::CheckVideoIsIFrame(char* szVideoName,unsigned char* szPVideoData, int nPVideoLength)
{
	int nPos = 0;
	bool bVideoIsIFrameFlag = false;
	unsigned char  nFrameType = 0x00;

	for (int i = 0; i< nPVideoLength; i++)
	{
		if (memcmp(szPVideoData + i, szVideoFrameHead, 4) == 0)
		{//�ҵ�֡Ƭ��
			if (strcmp(szVideoName, "H264") == 0)
			{
				nFrameType = (szPVideoData[i + 4] & 0x1F);
				if (nFrameType == 7 || nFrameType == 8 || nFrameType == 5)
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
				else
					break;//��I֡ 
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
					break;//��I֡ 
			}
		}
	}

	return bVideoIsIFrameFlag;
}

//url���� 
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

	char  *p = Src;  // ����ѭ��  
	int    i = 0;    // i��������url����  

	/* ��ʱ����url���������
	����: %1A%2B%3C
	*/
	char  t = '\0';
	while (*p != '\0' && MaxLen--)
	{
		if (*p == 0x25) // 0x25 = '%'  
		{
			/* ������ʮ���������г����ֵĴ�д��ĸ,Сд��ĸ,���ֵ��ж� */
			if (p[1] >= 'A' && p[1] <= 'Z') // ��д��ĸ  
			{
				t = p[1] - 'A' + 10;  // A = 10,��ͬ  
			}
			else if (p[1] >= 'a' && p[1] <= 'z') // Сд��ĸ  
			{
				t = p[1] - 'a' + 10;
			}
			else if (p[1] >= '0' && p[1] <= '9') // ����  
			{
				t = p[1] - '0';
			}

			t *= 16;  // �����ŵ�ʮλ��ȥ  

			if (p[2] >= 'A' && p[2] <= 'Z') // ��д��ĸ  
			{
				t += p[2] - 'A' + 10;
			}
			else if (p[2] >= 'a' && p[2] <= 'z') // Сд��ĸ  
			{
				t += p[2] - 'a' + 10;
			}
			else if (p[2] >= '0' && p[2] <= '9') // ����  
			{
				t += p[2] - '0';
			}

			// ���˺ϳ���һ��ʮ��������  
			url[i] = t;
			p += 3, i++;
		}
		else
		{
			// û�б�url���������  
			// '+'���⴦��.���൱��һ���ո�  
			if (*p != '+')
			{
				url[i] = *p;
			}
			else
			{
				url[i] = *p;//+ �Ż��ǲ������
			}
			i++;
			p++;
		}
	}
	url[i] = '\0';  // ������  
	return true;
}
