/*
���ܣ�
       ʵ��
        rtsp���� 
����    2021-07-24
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientRecvRtsp.h"
#include "LCbase64.h"

#include "netBase64.h"
#include "Base64.hh"

uint64_t                                     CNetClientRecvRtsp::Session = 1000;
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern size_t base64_decode(void* target, const char *source, size_t bytes);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);

//AAC����Ƶ�����
//AAC����Ƶ�����
int avpriv_mpeg4audio_sample_rates[] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000, 7350
};

extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result);

extern void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

//rtp����
void RTP_DEPACKET_CALL_METHOD rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	//	int nGet = 5; ABLRtspChan
	CNetClientRecvRtsp* pUserHandle = (CNetClientRecvRtsp*)cb->userdata;

	if (pUserHandle != NULL )
	{
	    if (pUserHandle->m_callbackFunc && pUserHandle->cbMediaCodecNameFlag == false)
		{
			pUserHandle->cbMediaCodecNameFlag = true;
			(*pUserHandle->m_callbackFunc) (pUserHandle->m_hParent, XHRtspDataType_Message, "success", (unsigned char*)pUserHandle->szMediaCodecName, strlen(pUserHandle->szMediaCodecName), pUserHandle->timeValue, pUserHandle->m_pCustomerPtr);
		}
	
		
 		if (cb->handle == pUserHandle->hRtpHandle[0])
		{
			if (pUserHandle->netBaseNetType == NetBaseNetType_RtspClientRecv )
			{
				if (*pUserHandle->m_callbackFunc)
				{
					if (pUserHandle->m_bHaveSPSPPSFlag && pUserHandle->m_bCallBackSPSFlag == false && pUserHandle->m_nSpsPPSLength > 0)
					{
						(*pUserHandle->m_callbackFunc) (pUserHandle->m_hParent, XHRtspDataType_Video, pUserHandle->szVideoName,(unsigned char*)pUserHandle->m_pSpsPPSBuffer, pUserHandle->m_nSpsPPSLength, cb->timestamp, pUserHandle->m_pCustomerPtr);
						 pUserHandle->m_bCallBackSPSFlag = true;
 					}
				   (*pUserHandle->m_callbackFunc) (pUserHandle->m_hParent, XHRtspDataType_Video, pUserHandle->szVideoName, cb->data, cb->datasize, cb->timestamp, pUserHandle->m_pCustomerPtr);
				}
  			}
 		}
		else if (cb->handle == pUserHandle->hRtpHandle[1])
		{
 			if (strcmp(pUserHandle->szAudioName, "AAC") == 0)
			{//aac
				pUserHandle->SplitterRtpAACData(cb->data, cb->datasize,cb->timestamp);
			}
			else
			{// G711A ��G711U
				(*pUserHandle->m_callbackFunc) (pUserHandle->m_hParent, XHRtspDataType_Audio, pUserHandle->szAudioName, cb->data, cb->datasize, cb->timestamp, pUserHandle->m_pCustomerPtr);
			}
 
 		}
	}
}

//��AAC��rtp�������и�
void  CNetClientRecvRtsp::SplitterRtpAACData(unsigned char* rtpAAC, int nLength, uint32_t nTimestamp)
{
	au_header_length = (rtpAAC[0] << 8) + rtpAAC[1];
	au_header_length = (au_header_length + 7) / 8;  
	ptr = rtpAAC;

	au_size = 2;  
	au_numbers = au_header_length / au_size;

	ptr += 2;  
	pau = ptr + au_header_length;  

	for (int i = 0; i < au_numbers; i++)
	{
		SplitterSize[i] = (ptr[0] << 8) | (ptr[1] & 0xF8);
		SplitterSize[i] = SplitterSize[i] >> 3; 

 		if (SplitterSize[i] > nLength || SplitterSize[i] <= 0 )
		{
			Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp=%X ,nClient = %llu, rtp �и�� ���� , SplitterSize[i] = %d ,nLength = %d ,au_numbers = %d ", this, nClient, SplitterSize[i], nLength, au_numbers);;

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}/**/
	 
		 AddADTSHeadToAAC((unsigned char*)pau, SplitterSize[i]);

		if (netBaseNetType == NetBaseNetType_RtspClientRecv )
		{
			if(m_callbackFunc)
			   (*m_callbackFunc) (m_hParent, XHRtspDataType_Audio, szAudioName, aacData, SplitterSize[i] + 7, nTimestamp, m_pCustomerPtr);

#ifdef WriteRtpDepacketFileFlag //����д���AAC��Ƶ�ļ���ֱ�Ӳ��� 
		   fwrite(aacData, 1, SplitterSize[i] + 7, fWriteRtpAudio);
		   fflush(fWriteRtpAudio);
#endif
		}
 
		ptr += au_size;
		pau += SplitterSize[i];
	}
}

//׷��adts��Ϣͷ
void  CNetClientRecvRtsp::AddADTSHeadToAAC(unsigned char* szData, int nAACLength)
{
	int len = nAACLength + 7;
	uint8_t profile = 2;
	uint8_t sampling_frequency_index = sample_index;
	uint8_t channel_configuration = nChannels;
	aacData[0] = 0xFF; /* 12-syncword */
	aacData[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
	aacData[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
	aacData[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
	aacData[4] = (uint8_t)(len >> 3);
	aacData[5] = ((len & 0x07) << 5) | 0x1F;
	aacData[6] = 0xFC | ((len / 1024) & 0x03);

	memcpy(aacData + 7, szData, nAACLength);
}

CNetClientRecvRtsp::CNetClientRecvRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType)
{
	nServer = hServer;
	nClient = hClient;
	hRtpVideo = 0;
	hRtpAudio = 0;
	nSendRtpFailCount = 0;//�ۼƷ���rtp��ʧ�ܴ��� 
	strcpy(m_szShareMediaURL, szShareMediaURL);
	m_hParent = hParent;
	CSeq = 1;
    cbMediaCodecNameFlag = false ;
	m_bCallBackSPSFlag = false;
	m_nXHRtspURLType = nXHRtspURLType;
	
	m_pCustomerPtr = pCustomerPtr;
	m_callbackFunc = callbackFunc;

	memset(szClientIP, 0x00, sizeof(szClientIP));
	DecodeUrl(szIP, szClientIP, sizeof(szClientIP));
	nClientPort = nPort;
	nPrintCount = 0;
 	bRunFlag = true;
	bIsInvalidConnectFlag = false;
	
	netDataCacheLength = 0;//�������ݻ����С
	nNetStart = nNetEnd = 0; //����������ʼλ��\����λ��
	if (m_nXHRtspURLType == XHRtspURLType_Liveing)//ʵ��
	  MaxNetDataCacheCount = MaxNetDataCacheBufferLength * 1 ; 
	else if (m_nXHRtspURLType == XHRtspURLType_RecordPlay)//¼��ط�
	  MaxNetDataCacheCount = MaxNetDataCacheBufferLength * 2;
	else if (m_nXHRtspURLType == XHRtspURLType_RecordDownload)//¼������
	  MaxNetDataCacheCount = MaxNetDataCacheBufferLength * 3 + (1024 * 1024 * 2) ;
	else 
	  MaxNetDataCacheCount = MaxNetDataCacheBufferLength * 1;
  
	data_Length = 0;
	netDataCache = new unsigned char[MaxNetDataCacheCount];

	nRecvLength = 0;
	memset((char*)szHttpHeadEndFlag, 0x00, sizeof(szHttpHeadEndFlag));
	strcpy((char*)szHttpHeadEndFlag, "\r\n\r\n");
	nHttpHeadEndLength = 0;
	nContentLength = 0;
	memset(szResponseHttpHead, 0x00, sizeof(szResponseHttpHead));
	
	m_bHaveSPSPPSFlag = false;
	m_nSpsPPSLength = 0;
	memset(s_extra_data,0x00,sizeof(s_extra_data));
	extra_data_size = 0;

	RtspProtectArrayOrder = 0;
	for (int i = 0; i < MaxRtspProtectCount; i++)
		memset((char*)&RtspProtectArray[i], 0x00, sizeof(RtspProtectArray[i]));

	for (int i = 0; i < MaxRtpHandleCount; i++)
  		hRtpHandle[i] = 0 ;
 
	for (int i = 0; i < 3; i++)
		bExitProcessFlagArray[i] = true;
	
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp step 4 ");

	m_nSpsPPSLength = 0;
	AuthenticateType = WWW_Authenticate_None;
	nSendSetupCount = 0;
	netBaseNetType = NetBaseNetType_RtspClientRecv;

	for (int i = 0; i < 16; i++)
		memset(szTrackIDArray[i], 0x00, sizeof(szTrackIDArray[i]));
	
	if (ParseRtspRtmpHttpURL(szClientIP) == true)
		uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)NULL, 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, 5000, 1);
	
	nVideoSSRC = rand();
	time(&nCurrentTime);
	//����Ƿ��Ǵ������
	if (strstr(m_rtspStruct.szSrcRtspPullUrl, "realmonitor") != NULL)
	{//������ͷʵ��url��ַ����  realmonitor  
		bSendRRReportFlag = true;
    }
	nVideoSSRC = audioSSRC = 0;
	strcpy(szAudioName, "NONE");
	bPauseFlag = false;
	dRange = 0;

	psHandle = 0;
#ifdef WriteRtpDepacketFileFlag
	fWriteRtpVideo = fopen("d:\\rtspRecv.264", "wb");
 	fWriteRtpAudio = fopen("d:\\rtspRecv.aac", "wb");
	bStartWriteFlag = false;
#endif

	time(&nCreateDateTime);
	time(&nProxyDisconnectTime);

	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp ���� ��� nClient = %llu , nCreateDateTime = %d  ", nClient, nCreateDateTime);
}

CNetClientRecvRtsp::~CNetClientRecvRtsp()
{
	std::lock_guard<std::mutex> lock(netDataLock);
	time_t tStartTime;
	time(&tStartTime);
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp �ȴ������˳� nTime = %llu, nClient = %llu ", tStartTime, nClient);
	
    bRunFlag = false;
	XHNetSDK_Disconnect(nClient);

 	for (int i = 0; i < 3; i++)
	{
		while (!bExitProcessFlagArray[i])
			usleep(5 * OneMicroSecondTime);
	}
	time(&tStartTime);
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp �����˳���� nTime = %llu, nClient = %llu ", tStartTime, nClient);
	
 	for (int i = 0; i < MaxRtpHandleCount; i++)
	{
		if(hRtpHandle[i] > 0 )
			rtp_depacket_stop(hRtpHandle[i]);
 	}

#ifdef WriteRtpDepacketFileFlag
	if(fWriteRtpVideo)
	  fclose(fWriteRtpVideo);
	if(fWriteRtpAudio)
	  fclose(fWriteRtpAudio);
#endif

	if (netDataCache != NULL)
	{
		delete [] netDataCache;
		netDataCache = NULL;
	}
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp ���� nClient = %llu \r\n", nClient);
}
int CNetClientRecvRtsp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	//������Ƶ֡�ٶ� 
	CalcVideoFrameSpeed();

	return 0;
}

int CNetClientRecvRtsp::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	return 0;
}

int CNetClientRecvRtsp::SendVideo()
{

	return 0;
}

int CNetClientRecvRtsp::SendAudio()
{

	return 0;
}

//��������ƴ�� 
int CNetClientRecvRtsp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength)
{
	std::lock_guard<std::mutex> lock(netDataLock);

	//������߼��
	nRecvDataTimerBySecond = 0;

	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
	{//ʣ��ռ��㹻
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
		if (netDataCacheLength > 0)
		{//���������ʣ��
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;
			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp = %X nClient = %llu �����쳣��ִ��ɾ��", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
		}
		else
		{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
			nNetStart = nNetEnd = netDataCacheLength = 0;
 		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
 	return 0 ;
}

//��ȡ�������� ��ģ��ԭ���ײ�������ȡ���� 
int32_t  CNetClientRecvRtsp::XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain)
{
	int nWaitCount = 0;
	bExitProcessFlagArray[0] = false;
	while (!bIsInvalidConnectFlag && bRunFlag)
	{
		if (netDataCacheLength >= *buffsize)
		{
			memcpy(buffer, netDataCache + nNetStart, *buffsize);
			nNetStart += *buffsize;
			netDataCacheLength -= *buffsize;
			
			bExitProcessFlagArray[0] = true;
			return 0;
		}
		usleep(20 * OneMicroSecondTime);
		
		nWaitCount ++;
		if (nWaitCount >= 100 * 5)
			break;
	}
	bExitProcessFlagArray[0] = true;

	return -1;  
}

bool   CNetClientRecvRtsp::ReadRtspEnd()
{
	unsigned int nReadLength = 1;
	unsigned int nRet;
	bool     bRet = false;
	bExitProcessFlagArray[1] = false;
	while (!bIsInvalidConnectFlag && bRunFlag)
	{
		nReadLength = 1;
		nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
		if (nRet == 0 && nReadLength == 1)
		{
			data_Length += 1;
			if (data_Length > 4 && data_[data_Length - 4] == '\r' && data_[data_Length - 3] == '\n' && data_[data_Length - 2] == '\r' && data_[data_Length - 1] == '\n')
			{
				bRet = true;
				break;
			}
		}
		else
		{
			Rtsp_WriteLog(Log_Debug, "ReadRtspEnd() ,��δ��ȡ������ ��CABLRtspClient =%X ,dwClient=%llu ", this, nClient);
			break;
		}

		if (data_Length >= RtspServerRecvDataLength)
		{
			Rtsp_WriteLog(Log_Debug, "ReadRtspEnd() ,�Ҳ��� rtsp �������� ��CABLRtspClient =%X ,dwClient = %llu ", this, nClient);
			break;
		}
	}
	bExitProcessFlagArray[1] = true;
	return bRet;
}

//����
int  CNetClientRecvRtsp::FindHttpHeadEndFlag()
{
	if (data_Length <= 0)
		return -1 ;

	for (int i = 0; i < data_Length; i++)
	{
		if (memcmp(data_ + i, szHttpHeadEndFlag, 4) == 0)
		{
			nHttpHeadEndLength = i + 4;
			return nHttpHeadEndLength;
		}
	}
	return -1;
}

int  CNetClientRecvRtsp::FindKeyValueFlag(char* szData)
{
	int nSize = strlen(szData);
	for (int i = 0; i < nSize; i++)
	{
		if (memcmp(szData + i, ": ", 2) == 0)
			return i;
	}
	return -1;
}

//��ȡHTTP������httpURL 
void CNetClientRecvRtsp::GetHttpModemHttpURL(char* szMedomHttpURL)
{//"POST /getUserName?userName=admin&password=123456 HTTP/1.1"
	if (RtspProtectArrayOrder >= MaxRtspProtectCount)
		return;

	int nPos1, nPos2, nPos3, nPos4;
	string strHttpURL = szMedomHttpURL;
	char   szTempRtsp[512] = { 0 };
	string strTempRtsp;

	strcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspCmdString, szMedomHttpURL);

	nPos1 = strHttpURL.find(" ", 0);
	if (nPos1 > 0)
	{
		nPos2 = strHttpURL.find(" ", nPos1 + 1);
		if (nPos1 > 0 && nPos2 > 0)
		{
			memcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspCommand, szMedomHttpURL, nPos1);

			//memcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspURL, szMedomHttpURL + nPos1 + 1, nPos2 - nPos1 - 1);
			memcpy(szTempRtsp, szMedomHttpURL + nPos1 + 1, nPos2 - nPos1 - 1);
			strTempRtsp = szTempRtsp;
			nPos3 = strTempRtsp.find("?", 0);
			if (nPos3 > 0)
			{//ȥ�������
				szTempRtsp[nPos3] = 0x00;
			}
			strTempRtsp = szTempRtsp;

			//����554 �˿�
			nPos4 = strTempRtsp.find(":", 8);
			if (nPos4 <= 0)
			{//û��554 �˿�
				nPos3 = strTempRtsp.find("/", 8);
				if (nPos3 > 0)
				{
					strTempRtsp.insert(nPos3, ":554");
				}
			}

			strcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspURL, strTempRtsp.c_str());
		}
	}
}

 //��httpͷ������䵽�ṹ��
int  CNetClientRecvRtsp::FillHttpHeadToStruct()
{
	RtspProtectArrayOrder = 0;
	if (RtspProtectArrayOrder >= MaxRtspProtectCount)
		return true;

	int  nStart = 0;
	int  nPos = 0;
	int  nFlagLength;
	if (nHttpHeadEndLength <= 0)
		return true;
	int  nKeyCount = 0;
	char szTemp[1024] = { 0 };

	for (int i = 0; i < nHttpHeadEndLength - 2; i++)
	{
		if (memcmp(data_ + i, szHttpHeadEndFlag, 2) == 0)
		{
			memset(szTemp, 0x00, sizeof(szTemp));
			memcpy(szTemp, data_ + nPos, i - nPos);

			if ((nFlagLength = FindKeyValueFlag(szTemp)) >= 0)
			{
				memset(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, 0x00, sizeof(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey));//Ҫ���
				memset(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue, 0x00, sizeof(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue));//Ҫ��� 

 				memcpy(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, szTemp, nFlagLength);
				memcpy(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue, szTemp + nFlagLength + 2, strlen(szTemp) - nFlagLength - 2);

				if (memcmp(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, "Content-Length", 14) == 0)
				{//���ݳ���
					nContentLength = atoi(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue);
					RtspProtectArray[RtspProtectArrayOrder].nRtspSDPLength = nContentLength;
				}

				nKeyCount++;

				//��ֹ������Χ
				if (nKeyCount >= MaxRtspValueCount)
					return true;
			}
			else
			{//���� http ������URL 
				GetHttpModemHttpURL(szTemp);
			} 

			nPos = i + 2;
		}
	}

	return true;
} 

bool CNetClientRecvRtsp::GetFieldValue(char* szFieldName, char* szFieldValue)
{
	bool bFindFlag = false;

	for (int i = 0; i < MaxRtspProtectCount; i++)
	{
		for (int j = 0; j < MaxRtspValueCount; j++)
		{
			if (strcmp(RtspProtectArray[i].rtspField[j].szKey, szFieldName) == 0)
			{
				bFindFlag = true;
				strcpy(szFieldValue, RtspProtectArray[i].rtspField[j].szValue);
				break;
			}
		}
	}

	return bFindFlag;
}

//ͳ��rtspURL  rtsp://190.15.240.11:554/Media/Camera_00001 ·�� / ������ 
int  CNetClientRecvRtsp::GetRtspPathCount(char* szRtspURL)
{
	string strCurRtspURL = szRtspURL;
	int    nPos1, nPos2;
	int    nPathCount = 0;
 	nPos1 = strCurRtspURL.find("//", 0);
	if (nPos1 < 0)
		return 0;//��ַ�Ƿ� 

	while (true)
	{
		nPos1 = strCurRtspURL.find("/", nPos1 + 2);
		if (nPos1 >= 0)
		{
			nPos1 += 1;
			nPathCount ++;
		}
		else
			break;
	}
	return nPathCount;
}

//����rtsp����
void  CNetClientRecvRtsp::InputRtspData(unsigned char* pRecvData, int nDataLength)
{
#ifdef PrintRtspLogFlag
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp= %X , nClient = %llu \r\n%s", this, nClient,pRecvData);
#endif

    if (memcmp(data_, "RTSP/1.0 401", 12) == 0 && nRtspProcessStep == RtspProcessStep_OPTIONS && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//�������,OPTIONS��Ҫ����md5��ʽ��֤
		if (!GetWWW_Authenticate())
		{
			DeleteNetRevcBaseClient(nClient);
			return;
		}
		SendOptions(AuthenticateType,true);
	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_OPTIONS && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		SendDescribe(AuthenticateType);
   	}
	else if (nRtspProcessStep == RtspProcessStep_DESCRIBE && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//&& nRecvLength > 500
 		if (data_Length - nHttpHeadEndLength < nContentLength)
		{
			Rtsp_WriteLog(Log_Debug, "������δ�������� ");
 			return;
		}

		//��ҪMD5��֤
		if (memcmp(pRecvData, "RTSP/1.0 401", 12) == 0)
		{
			if (!GetWWW_Authenticate())
			{
				DeleteNetRevcBaseClient(nClient);
			    return;
 		     }

		   SendDescribe(AuthenticateType);
		   return ;
		}

		//����Ƿ���Ҫ����
		if (memcmp(pRecvData, "RTSP/1.0 200", 12) != 0)
		{
			boost::shared_ptr<CNetRevcBase> pParentClient = GetNetRevcBaseClientNoLock(m_hParent);
			if (pParentClient)
				pParentClient->bReConnectFlag = true;

			Rtsp_WriteLog(Log_Debug,"rtsp �������� \r\n%s", pRecvData);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		memset(szCSeq, 0x00, sizeof(szCSeq));
		GetFieldValue("CSeq", szCSeq);

		currentSession = Session;
		Session++;
 
		//����SDP��Ϣ
		memset(szRtspContentSDP, 0x00, sizeof(szRtspContentSDP));
		memcpy(szRtspContentSDP, data_ + nHttpHeadEndLength, nContentLength);
		nContentLength = 0; //nContentLength���� Ҫ��λ

		//��SDP�л�ȡ��Ƶ����Ƶ��ʽ��Ϣ 
		if (!GetMediaInfoFromRtspSDP())
		{
 			Rtsp_WriteLog(Log_Debug, "DESCRIBE SDP ��Ϣ��û�кϷ���ý������ %s ", szRtspContentSDP);
 			DeleteNetRevcBaseClient(nClient);
			return ;
		}

		//ȷ������Ƶ���
		FindVideoAudioInSDP();

		GetSPSPPSFromDescribeSDP();
  
		int nRet2 = rtp_depacket_start(rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle[0]);
		if (strcmp(szVideoName, "H264") == 0)
			rtp_depacket_setpayload(hRtpHandle[0], nVideoPayload, e_rtpdepkt_st_h264);
		else if (strcmp(szVideoName, "H265") == 0)
			rtp_depacket_setpayload(hRtpHandle[0], nVideoPayload, e_rtpdepkt_st_h265);

		nRet2 = rtp_depacket_start(rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle[1]);

 		if (strcmp(szAudioName, "G711_A") == 0)
		{
			rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_g711a);
		}
		else if (strcmp(szAudioName, "G711_U") == 0)
		{
			rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_g711u);
		}
		else if (strcmp(szAudioName, "AAC") == 0)
		{
			rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_aac);
		}
		else if (strstr(szAudioName, "G726LE") != NULL)
		{
			rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_g726le);
		}

		//ȷ��ADTSͷ��ز���
		sample_index = 11;
		for (int i = 0; i < 13; i++)
		{
			if (avpriv_mpeg4audio_sample_rates[i] == nSampleRate)
			{
				sample_index = i;
				break;
			}
		} 
		SendSetup(AuthenticateType);

		//�ص�ý����Ϣ
		if (dRange > 0 )
			sprintf(szMediaCodecName, "{\"video\":\"%s\",\"audio\":\"%s\",\"channels\":%d,\"sampleRate\":%d,\"range\":%d}", szVideoName, szAudioName, nChannels, nSampleRate,(int)dRange);
		else
		    sprintf(szMediaCodecName, "{\"video\":\"%s\",\"audio\":\"%s\",\"channels\":%d,\"sampleRate\":%d}", szVideoName, szAudioName, nChannels, nSampleRate);
		Rtsp_WriteLog(Log_Debug, "��Describe�У�szMediaCodecName %s ", szMediaCodecName);
 		//if (m_callbackFunc)
		//	(*m_callbackFunc) (m_hParent, XHRtspDataType_Message, "success", (unsigned char*)szMediaCodecName, strlen(szMediaCodecName), timeValue, m_pCustomerPtr);
  	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_SETUP && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		GetFieldValue("Session", szSessionID);
		if (strlen(szSessionID) == 0)
		{//��Щ���������Session,������ԣ������̹ر�
			string strSessionID;
			char   szTempSessionID[128] = { 0 };
			int    nPos;
			if (GetFieldValue("session", szTempSessionID))
			{
				strSessionID = szTempSessionID;
				nPos = strSessionID.find(";", 0);
				if (nPos > 0)
				{//�г�ʱ���Ҫȥ����ʱѡ��
					memcpy(szSessionID, szTempSessionID, nPos);
				}
				else
					strcpy(szSessionID, szTempSessionID);
			}
			else
				strcpy(szSessionID, "1000005");
		}

		if (nMediaCount == 1)
		{//ֻ��1��ý�壬ֱ�ӷ���Player
			SendPlay(AuthenticateType);
		}
		else if (nMediaCount == 2)
		{
			if (nSendSetupCount == 1)
				SendSetup(AuthenticateType);//��Ҫ�ٷ���
			else
			{
				SendPlay(AuthenticateType);
			}
		}

		nRecvLength = nHttpHeadEndLength = 0;
   	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_PLAY && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		nRtspProcessStep = RtspProcessStep_PLAYSucess ;
		boost::shared_ptr<CNetRevcBase> pParentClient = GetNetRevcBaseClientNoLock(m_hParent);
		if (pParentClient)
		{
			pParentClient->bReConnectFlag = false ;//�Ѿ��ɹ����ӡ����ҽ�����ɣ�����Ҫ��������
			pParentClient->nRtspConnectStatus = RtspConnectStatus_ConnectSuccess;//���ӳɹ� 
		}

		Rtsp_WriteLog(Log_Debug, "�յ� Play �ظ����rtsp������� nClient = %llu ", nClient);
	}
	else if (memcmp(pRecvData, "TEARDOWN", 8) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		Rtsp_WriteLog(Log_Debug, "�յ��Ͽ� TEARDOWN �������ִ��ɾ�� nClient = %llu ", nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
	}
	else if (memcmp(pRecvData, "GET_PARAMETER", 13) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		Rtsp_WriteLog(Log_Debug, "�յ� GET_PARAMETER ���ִ�лظ� nClient = %llu ", nClient);

		memset(szCSeq, 0x00, sizeof(szCSeq));
		GetFieldValue("CSeq", szCSeq);

		sprintf(szResponseBuffer, "RTSP/1.0 200 OK\r\nCSeq: %s\r\nPublic: %s\r\nx-Timeshift_Range: clock=20100318T021915.84Z-20100318T031915.84Z\r\nx-Timeshift_Current: clock=20100318T031915.84Z\r\n\r\n", szCSeq, RtspServerPublic);
		nSendRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
		if (nSendRet != 0)
		{
			Rtsp_WriteLog(Log_Debug, "�ظ� GET_PARAMETER �������ִ��ɾ�� nClient = %llu ", nClient);
 			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		}
	}
	else if (memcmp(pRecvData, "RTSP/1.0 200", 12) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		Rtsp_WriteLog(Log_Debug, "�յ���ȷ��Ӧ nClient = %llu \r\n%s", nClient, pRecvData);
	}
	else if (memcmp(pRecvData, "ANNOUNCE  RTSP/1.0",18) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//�յ���Ϊ��������������ͽ�����
        nRtspConnectStatus = RtspConnectStatus_ConnectFailed;
		
 		//�޸����ӳɹ�,�Ѵ�������״̬�޸�Ϊ�ɹ�
		boost::shared_ptr<CNetRevcBase>   pParent = GetNetRevcBaseClientNoLock(m_hParent);
		if (pParent != NULL)
		   pParent->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;
	
		Rtsp_WriteLog(Log_Debug, "�յ���Ϊ��������������ͽ����� nClient = %llu \r\n%s", nClient, pRecvData);
	}
	else
	{
         nRtspConnectStatus = RtspConnectStatus_ConnectFailed;
		 
 		//�޸����ӳɹ�,�Ѵ�������״̬�޸�Ϊ�ɹ�
		boost::shared_ptr<CNetRevcBase>   pParent = GetNetRevcBaseClientNoLock(m_hParent);
		if (pParent != NULL)
		   pParent->nRtspConnectStatus = RtspConnectStatus_ConnectFailed;
		
		bIsInvalidConnectFlag = true; //ȷ��Ϊ�Ƿ����� 
		Rtsp_WriteLog(Log_Debug, "�Ƿ���rtsp �������ִ��ɾ�� nClient = %llu ",nClient);
		DeleteNetRevcBaseClient(nClient);
	}
}

bool CNetClientRecvRtsp::getRealmAndNonce(char* szDigestString, char* szRealm, char* szNonce)
{
	string strDigestString = szDigestString;

	int nPos1, nPos2;
	nPos1 = strDigestString.find("realm=\"", 0);
	if (nPos1 <= 0)
		return false;
	nPos2 = strDigestString.find("\"", nPos1 + strlen("realm=\""));
	if (nPos2 <= 0)
		return false;

	memcpy(szRealm, szDigestString + nPos1 + 7, nPos2 - nPos1 - 7);

	nPos1 = strDigestString.find("nonce=\"", 0);
	if (nPos1 <= 0)
		return false;
	nPos2 = strDigestString.find("\"", nPos1 + strlen("nonce=\""));
	if (nPos2 <= 0)
		return false;

	memcpy(szNonce, szDigestString + nPos1 + 7, nPos2 - nPos1 - 7);

	return true;
}


//��ȡ��֤��ʽ
bool  CNetClientRecvRtsp::GetWWW_Authenticate()
{
	if (!GetFieldValue("WWW-Authenticate", szWww_authenticate))
	{
		Rtsp_WriteLog(Log_Debug, "��Describe�У���ȡ���� www-authenticate ��Ϣ��\r\n");
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return false;
	}

	//ȷ����������֤����
	string strWww_authenticate = szWww_authenticate;
	int nPos1 = strWww_authenticate.find("Digest ", 0);//ժҪ��֤
	int nPos2 = strWww_authenticate.find("Basic ", 0);//������֤
	if (nPos1 >= 0)
		AuthenticateType = WWW_Authenticate_MD5;
	else if (nPos2 >= 0)
		AuthenticateType = WWW_Authenticate_Basic;
	else
	{//����ʶ����֤��ʽ��
		Rtsp_WriteLog(Log_Debug, "��Describe�У��Ҳ�����ʶ����֤��ʽ ��\r\n");
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return false;
	}

	if (AuthenticateType == WWW_Authenticate_MD5)
	{//ժҪ��֤ ��ȡrealm ,nonce 
		if (getRealmAndNonce(szWww_authenticate, m_rtspStruct.szRealm, m_rtspStruct.szNonce) == false)
		{
			Rtsp_WriteLog(Log_Debug, "��Describe�У���ȡ���� realm,nonce ��Ϣ��\r\n");
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return false;
		}
	}
	return true;
}

//��sdp��Ϣ�л�ȡ��Ƶ����Ƶ��ʽ��Ϣ ,������Щ��Ϣ����rtp�����rtp��� 
bool   CNetClientRecvRtsp::GetMediaInfoFromRtspSDP()
{
	//��sdpװ�������
	string strSDPTemp = szRtspContentSDP;
	char   szTemp[128] = { 0 };
	int    nPos1 = strSDPTemp.find("m=video", 0);
	int    nPos2 = strSDPTemp.find("m=audio", 0);
	int    nPos3;
	memset(szVideoSDP, 0x00, sizeof(szVideoSDP));
	memset(szAudioSDP, 0x00, sizeof(szAudioSDP));
	memset(szVideoName, 0x00, sizeof(szVideoName));
	memset(szAudioName, 0x00, sizeof(szAudioName));
	nChannels = 0;

	if (nPos1 > 0 && nPos2 > 0 && nPos2 > nPos1)
	{//��ƵSDP ����ǰ�� ����ƵSDP���ں���
		memcpy(szVideoSDP, szRtspContentSDP + nPos1, nPos2 - nPos1);
		memcpy(szAudioSDP, szRtspContentSDP + nPos2, strlen(szRtspContentSDP) - nPos2);

		sipParseV.ParseSipString(szVideoSDP);
		sipParseA.ParseSipString(szAudioSDP);
	}
	else if (nPos1 > 0 && nPos2 > 0 && nPos2 < nPos1)
	{//��ƵSDP���ں��棬��ƵSDP����ǰ�棬ZLMediaKit �������������з�ʽ 
		memcpy(szVideoSDP, szRtspContentSDP + nPos1, strlen(szRtspContentSDP) - nPos1);
		memcpy(szAudioSDP, szRtspContentSDP + nPos2, nPos1 - nPos2);

		sipParseV.ParseSipString(szVideoSDP);
		sipParseA.ParseSipString(szAudioSDP);
	}
	else if (nPos1 > 0 && nPos2 < 0)
	{//ֻ����Ƶ
		memcpy(szVideoSDP, szRtspContentSDP + nPos1, strlen(szRtspContentSDP) - nPos1);
		sipParseV.ParseSipString(szVideoSDP);
	}

	//����¼��ʱ��ʱ��
	char szRangeValue[256] = { 0 };
	nPos1 = nPos2 = 0;
	nPos1 = strSDPTemp.find("a=range: npt=0-", 0);
	if (nPos1 > 0 )
		nPos2 = strSDPTemp.find("\r\n", nPos1);
	if (nPos1 > 0 && nPos2 > 0)
	{
		memcpy(szRangeValue, szRtspContentSDP + nPos1 + strlen("a=range: npt=0-"), nPos2 - nPos1 - strlen("a=range: npt=0-"));
		dRange = atof(szRangeValue);
	}

	//��ȡ��Ƶ��������
	string strVideoName;
	char   szTemp2[64] = { 0 };
	char   szTemp3[64] = { 0 };
	if (sipParseV.GetSize() > 0)
	{
		if (sipParseV.GetFieldValue("a=rtpmap", szTemp))
		{
			strVideoName = szTemp;
			nPos1 = strVideoName.find(" ", 0);
			if (nPos1 > 0)
			{
				memcpy(szTemp2, szTemp, nPos1);
				nVideoPayload = atoi(szTemp2);
			}
			nPos2 = strVideoName.find("/", 0);
			if (nPos1 > 0 && nPos2 > 0)
			{
				memcpy(szVideoName, szTemp + nPos1 + 1, nPos2 - nPos1 - 1);

				//תΪ��С
				strVideoName = szVideoName;
				to_upper(strVideoName);
				strcpy(szVideoName, strVideoName.c_str());
			}

			Rtsp_WriteLog(Log_Debug, "��SDP�У���ȡ������Ƶ��ʽΪ %s , payload = %d ��", szVideoName, nVideoPayload);
		}
	}

	  //��ȡ��Ƶ��Ϣ
	  if (sipParseA.GetSize() > 0)
	  {
		memset(szTemp2, 0x00, sizeof(szTemp2));
		memset(szTemp, 0x00, sizeof(szTemp));
		if (sipParseA.GetFieldValue("a=rtpmap", szTemp))
		{
			strVideoName = szTemp;
			nPos1 = strVideoName.find(" ", 0);
			if (nPos1 > 0)
			{
				memcpy(szTemp2, szTemp, nPos1);
				nAudioPayload = atoi(szTemp2);
			}
			nPos2 = strVideoName.find("/", 0);
			if (nPos1 > 0 && nPos2 > 0)
			{
				memcpy(szAudioName, szTemp + nPos1 + 1, nPos2 - nPos1 - 1);

				//תΪ��С
				string strName = szAudioName;
				to_upper(strName);
				strcpy(szAudioName, strName.c_str());

				//�ҳ�����Ƶ�ʡ�ͨ����
				nPos3 = strVideoName.find("/", nPos2 + 1);
				memset(szTemp2, 0x00, sizeof(szTemp2));
				if (nPos3 > 0)
				{
					memcpy(szTemp2, szTemp + nPos2 + 1, nPos3 - nPos2 - 1);
					nSampleRate = atoi(szTemp2);

					memcpy(szTemp3, szTemp + nPos3 + 1, strlen(szTemp) - nPos3 - 1);
					nChannels = atoi(szTemp3);//ͨ������
				}
				else
				{
					memcpy(szTemp2, szTemp + nPos2 + 1, strlen(szTemp) - nPos2 - 1);
					nSampleRate = atoi(szTemp2);
					nChannels = 1;
				}

				//��ֹ��Ƶͨ�������� 
				if (nChannels > 2)
					nChannels = 1;
			}
		}

		if (strcmp(szAudioName, "PCMA") == 0)
		{
			strcpy(szAudioName, "G711_A");
		}
		else if (strcmp(szAudioName, "PCMU") == 0)
		{
			strcpy(szAudioName, "G711_U");
		}
		else if (strcmp(szAudioName, "MPEG4-GENERIC") == 0)
		{
			strcpy(szAudioName, "AAC");
		}
		else if (strstr(szAudioName, "G726") != NULL)
		{
			strcpy(szAudioName, "G726LE");
		}
		else
			strcpy(szAudioName, "NONE");

		Rtsp_WriteLog(Log_Debug, "��SDP�У���ȡ������Ƶ��ʽΪ %s ,nChannels = %d ,SampleRate = %d , payload = %d ��", szAudioName, nChannels, nSampleRate, nAudioPayload);
	}

   if (strlen(szVideoName) == 0)
   {
		nMediaCount = 0;
		return false;
    }
   else
   {
	   if (nChannels >= 1)
		   nMediaCount = 2;
	   else
		   nMediaCount = 1;
      return true;
   }
}

//����rtp����־ 
bool CNetClientRecvRtsp::FindRtpPacketFlag()
{
	bool bFindFlag = false;

	unsigned char szRtpFlag1[2] = { 0x24, 0x00 };
	unsigned char szRtpFlag2[2] = { 0x24, 0x01 };
	unsigned char szRtpFlag3[2] = { 0x24, 0x02 };
	unsigned char szRtpFlag4[2] = { 0x24, 0x03 };
	int  nPos = 0;

	if (netDataCacheLength > 2)
	{
		for (int i = nNetStart; i < nNetEnd; i++)
		{
			if ((memcmp(netDataCache + i, szRtpFlag1, 2) == 0) || 
				(memcmp(netDataCache + i, szRtpFlag2, 2) == 0) || 
				(memcmp(netDataCache + i, szRtpFlag3, 2) == 0) || 
				(memcmp(netDataCache + i, szRtpFlag4, 2) == 0))
			{
				nPos = i;
				bFindFlag = true;
 				break;
			}
  		}
	}

	//�ҵ���־�����¼�����㣬������ 
	if (bFindFlag)
	{
		nNetStart = nPos;
		netDataCacheLength = nNetEnd - nNetStart;
		Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp = %X ,�ҵ�RTPλ�� ,nClient = %llu�� nNetStart = %d , nNetEnd = %d , netDataCacheLength = %d ", this,nClient, nNetStart, nNetEnd, netDataCacheLength);
	}

	return bFindFlag;
}

//������������
int CNetClientRecvRtsp::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	bExitProcessFlagArray[2] = false; 
	int nRet;
	uint32_t nReadLength = 4;
	 
	while (!bIsInvalidConnectFlag && bRunFlag && netDataCacheLength > 4)
	{
		data_Length = 0;
		memcpy((unsigned char*)&rtspHead, netDataCache + nNetStart, sizeof(rtspHead));
		if (true)
		{
			if (rtspHead.head == '$')
			{//rtp ����
				nNetStart += 4;
				netDataCacheLength -= 4;

				nRtpLength = nReadLength = ntohs(rtspHead.Length);

				if (nRtpLength > netDataCacheLength )
				{//ʣ��rtp���Ȳ�����ȡ����Ҫ�˳����ȴ���һ�ζ�ȡ
					bExitProcessFlagArray[2] = true;
					nNetStart -= 4;
					netDataCacheLength += 4;
 					return 0;
				}

				if (nRtpLength > 0 && nRtpLength < 65535 )
				{
					if (rtspHead.chan == 0x00)
					{
						if (nVideoSSRC == 0)
						{//��ȡSSRC 
							memcpy((char*)&rtpHead, netDataCache + nNetStart, sizeof(rtpHead));
							nVideoSSRC = rtpHead.ssrc;
						}
						rtp_depacket_input(hRtpHandle[0], netDataCache + nNetStart , nRtpLength );
 
						//if(nPrintCount % 200 == 0)
 						//  printf("this =%X, Video Length = %d \r\n",this, nReadLength);
					}
					else if (rtspHead.chan == 0x02)
					{
						rtp_depacket_input(hRtpHandle[1], netDataCache + nNetStart , nRtpLength );

						if (audioSSRC == 0)
						{//��ȡSSRC 
							memcpy((char*)&rtpHead, netDataCache + nNetStart, sizeof(rtpHead));
							audioSSRC = rtpHead.ssrc;
						}
					}
					else if (rtspHead.chan == 0x01)
					{//�յ�RTCP������Ҫ�ظ�rtcp�����
						//WriteLog(Log_Debug, "this =%X ,�յ� ��Ƶ ��RTCP������Ҫ�ظ�rtcp�������netBaseNetType = %d  �յ�RCP������ = %d ", this, netBaseNetType, nReadLength);
						SendRtcpReportDataRR(nVideoSSRC, 1);
					}
					else if (rtspHead.chan == 0x03)
					{//�յ�RTCP������Ҫ�ظ�rtcp�����
						//WriteLog(Log_Debug, "this =%X ,�յ� ��Ƶ ��RTCP������Ҫ�ظ�rtcp�������netBaseNetType = %d  �յ�RCP������ = %d ", this, netBaseNetType, nReadLength);
						SendRtcpReportDataRR(audioSSRC, 3);
					}

					bExitProcessFlagArray[2] = true;
					nNetStart           += nRtpLength;
					netDataCacheLength  -= nRtpLength;
				}
				else
				{
					Rtsp_WriteLog(Log_Debug, "ReadDataFunc() ,��δ��ȡ��rtp���� ! nClient = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
					bRunFlag = false;
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return -1;
				}
			}
			else
			{//rtsp ����
				if (FindRtpPacketFlag() == true)
				{//rtp ���� 
					bExitProcessFlagArray[2] = true;
					return 0;
				}

				if (!ReadRtspEnd())
				{
					Rtsp_WriteLog(Log_Debug, "ReadDataFunc() ,��δ��ȡ��rtsp (1)���� ! nClient = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
			        bRunFlag = false;
			       pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return  -1;
				}
				else
				{
					//���rtspͷ
					if (FindHttpHeadEndFlag() > 0)
						FillHttpHeadToStruct();

					if (nContentLength > 0)
					{
						nReadLength = nContentLength;
						nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
						if (nRet != 0 || nReadLength != nContentLength)
						{
							Rtsp_WriteLog(Log_Debug, "ReadDataFunc() ,��δ��ȡ��rtsp (2)���� ! nClient = %llu", nClient);
							bExitProcessFlagArray[2] = true;
			                bRunFlag = false;
			                pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
							return -1;
						}
						else
						{
							data_Length += nContentLength;
						}
					}

					data_[data_Length] = 0x00;
					InputRtspData(data_, data_Length);

					break; //rtsp ����һ һ�����ģ�ִ����������˳� 
				}
			}
		}
		else
		{
			Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp= %X  , ProcessNetData() ,��δ��ȡ������ ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			bRunFlag = false;
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}
	}

	bExitProcessFlagArray[2] = true;
	return 0;
}

//����rtcp �����
void  CNetClientRecvRtsp::SendRtcpReportData()
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
	rtcpSR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nVideoSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, 1);
}

//����rtcp ����� ���ն�
void  CNetClientRecvRtsp::SendRtcpReportDataRR(unsigned int nSSRC, int nChan)
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
	rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, nChan);
}

void  CNetClientRecvRtsp::ProcessRtcpData(char* szRtcpData, int nDataLength, int nChan)
{
	szRtcpDataOverTCP[0] = '$';
	szRtcpDataOverTCP[1] = nChan;
	unsigned short nRtpLen = htons(nDataLength);
	memcpy(szRtcpDataOverTCP + 2, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

	memcpy(szRtcpDataOverTCP + 4, szRtcpData, nDataLength);
	XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 4, 1);
}

//���͵�һ������
int CNetClientRecvRtsp::SendFirstRequst()
{
 	SendOptions(AuthenticateType,true);
 	return 0;
}

//����m3u8�ļ�
bool  CNetClientRecvRtsp::RequestM3u8File()
{
	return true;
}

int CNetClientRecvRtsp::sdp_h264_load(uint8_t* data, int bytes, const char* config)
{
	int n, len, off;
	const char* p, *next;
	const uint8_t startcode[] = { 0x00, 0x00, 0x00, 0x01 };

	off = 0;
	p = config;
	while (p)
	{
		next = strchr(p, ',');
		len = next ? (int)(next - p) : (int)strlen(p);
		if (off + (len + 3) / 4 * 3 + (int)sizeof(startcode) > bytes)
			return -1; // don't have enough space

		memcpy(data + off, startcode, sizeof(startcode));
		n = (int)base64_decode(data + off + sizeof(startcode), p, len);
		assert(n <= (len + 3) / 4 * 3);
		off += n + sizeof(startcode);

		p = next ? next + 1 : NULL;
	}

	return off;
}

//�� SDP�л�ȡ  SPS��PPS ��Ϣ
bool  CNetClientRecvRtsp::GetSPSPPSFromDescribeSDP()
{
	m_bHaveSPSPPSFlag = false;
	int  nPos1, nPos2;
	char  szSprop_Parameter_Sets[512] = { 0 };

	m_nSpsPPSLength = 0;
	string strSDPTemp = szRtspContentSDP;
	nPos1 = strSDPTemp.find("sprop-parameter-sets=", 0);
	memset(m_szSPSPPSBuffer, 0x00, sizeof(m_szSPSPPSBuffer));
	nPos2 = strSDPTemp.find("\r\n", nPos1 + 1);

	if (nPos1 > 0 && nPos2 > 0)
	{
		memcpy(szSprop_Parameter_Sets, szRtspContentSDP + nPos1, nPos2 - nPos1 + 2);
		strSDPTemp = szSprop_Parameter_Sets;
		nPos1 = strSDPTemp.find("sprop-parameter-sets=", 0);

		nPos2 = strSDPTemp.find(";", nPos1 + 1);
		if (nPos2 > nPos1)
		{//���滹�б����
			memcpy(m_szSPSPPSBuffer, szSprop_Parameter_Sets + nPos1 + strlen("sprop-parameter-sets="), nPos2 - nPos1 - strlen("sprop-parameter-sets="));
		}
		else
		{//����û�б����
			nPos2 = strSDPTemp.find("\r\n", nPos1 + 1);
			if (nPos2 > nPos1)
				memcpy(m_szSPSPPSBuffer, szSprop_Parameter_Sets + nPos1 + strlen("sprop-parameter-sets="), nPos2 - nPos1 - strlen("sprop-parameter-sets="));
		}
	}

	if (strlen(m_szSPSPPSBuffer) > 0)
	{
		m_nSpsPPSLength = sdp_h264_load((unsigned char*)m_pSpsPPSBuffer, sizeof(m_pSpsPPSBuffer), m_szSPSPPSBuffer);
		m_bHaveSPSPPSFlag = true;
	}

	return m_bHaveSPSPPSFlag;
}

void  CNetClientRecvRtsp::UserPasswordBase64(char* szUserPwdBase64)
{
	char szTemp[128] = { 0 };
	sprintf(szTemp, "%s:%s", m_rtspStruct.szUser, m_rtspStruct.szPwd);
	Base64Encode((unsigned char*)szUserPwdBase64, (unsigned char*)szTemp, strlen(szTemp));
}

//ȷ��SDP������Ƶ����Ƶ����ý������, ��Describe���ҵ� trackID���󻪵Ĵ�0��ʼ����������Ϊ�Ĵ�1��ʼ
void  CNetClientRecvRtsp::FindVideoAudioInSDP()
{
	char szTemp[1280] = { 0 };

	nMediaCount = 0;
	if (strlen(szRtspContentSDP) <= 0)
		return;

	strcpy(szTemp, szRtspContentSDP);
	to_lower(szTemp);
	string strSDP = szTemp;
	string strTraceID;
	char   szTempTraceID[512] = { 0 };
	int nPos, nPos2, nPos3, nPos4;

	nPos = strSDP.find("m=video");
	if (nPos >= 0)
	{
		nPos2 = strSDP.find("a=control:", nPos);
		if (nPos2 > 0)
		{
			nPos3 = strSDP.find("\r\n", nPos2);
			if (nPos3 > 0)
			{
				nMediaCount++;

				memcpy(szTempTraceID, szRtspContentSDP + nPos2 + 10, nPos3 - nPos2 - 10);
				strTraceID = szTempTraceID;
				nPos4 = strTraceID.rfind("/", strlen(szTempTraceID));
				if (nPos4 <= 0)
				{//û��rtsp���Ƶĵ�ַ������ trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//��rtsp���Ƶĵ�ַ�����纣��������ͷ a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
				}
				nTrackIDOrer++;
			}
		}
	}

	nPos = strSDP.find("m=audio");
	if (nPos >= 0)
	{
		nPos2 = strSDP.find("a=control:", nPos);
		if (nPos2 > 0)
		{
			nPos3 = strSDP.find("\r\n", nPos2);
			if (nPos3 > 0)
			{
				nMediaCount++;

				memcpy(szTempTraceID, szRtspContentSDP + nPos2 + 10, nPos3 - nPos2 - 10);
				strTraceID = szTempTraceID;
				nPos4 = strTraceID.rfind("/", strlen(szTempTraceID));
				if (nPos4 <= 0)
				{//û��rtsp���Ƶĵ�ַ������ trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//��rtsp���Ƶĵ�ַ�����纣��������ͷ a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
				}
				nTrackIDOrer++;
			}
		}
	}
}

void  CNetClientRecvRtsp::SendOptions(WWW_AuthenticateType wwwType, bool bRecordStatus)
{
	//ȷ������
 	nSendSetupCount = 0;
	nMediaCount = 0;
	nTrackIDOrer = 1;//��1��ʼ������0��ʼ
	CSeq = 1;

	if (wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("OPTIONS", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nAuthorization: Basic %s\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	unsigned int nRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	if (nRet != 0)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		Rtsp_WriteLog(Log_Debug, "�ظ� OPTIONS �������ִ��ɾ�� nClient = %llu ", nClient);
		return;
	}
	if (bRecordStatus)
	nRtspProcessStep = RtspProcessStep_OPTIONS;

#if PrintRtspLogFlag
	Rtsp_WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);
#endif
	CSeq++;
}

void  CNetClientRecvRtsp::SendDescribe(WWW_AuthenticateType wwwType)
{
	if (wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("DESCRIBE", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);

	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Basic %s\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	nRtspProcessStep = RtspProcessStep_DESCRIBE;

#ifdef PrintRtspLogFlag
	Rtsp_WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);
#endif
	CSeq++;
}

/*
Ҫ�Ż�����Щ����� {trackID=1 �� trackID=2} ����Щ������ǣ������ {trackID=0��trackID=1}
*/
void  CNetClientRecvRtsp::SendSetup(WWW_AuthenticateType wwwType)
{
	nSendSetupCount++;
	if (nSendSetupCount == 2)
	{
		string strSession = szSessionID;
		int    nPos2 = strSession.find(";", 0);
		if (nPos2 > 0)
		{
			szSessionID[nPos2] = 0x00;
			Rtsp_WriteLog(Log_Debug, "SendSetup() ��nClient = %llu ,strSessionID = %s , szSessionID = %s ", nClient, strSession.c_str(), szSessionID);
		}
	}
	if (wwwType == WWW_Authenticate_None)
	{
		if (nSendSetupCount == 1)
		{
			if (m_rtspStruct.szRtspURLTrim[strlen(m_rtspStruct.szRtspURLTrim) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_rtspStruct.szRtspURLTrim[strlen(m_rtspStruct.szRtspURLTrim) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
		}
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("SETUP", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		if (nSendSetupCount == 1)
		{
			if (m_rtspStruct.szRtspURLTrim[strlen(m_rtspStruct.szRtspURLTrim) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_rtspStruct.szRtspURLTrim[strlen(m_rtspStruct.szRtspURLTrim) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse, szSessionID);
		}

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);

		if (nSendSetupCount == 1)
		{
			if (m_rtspStruct.szRtspURLTrim[strlen(m_rtspStruct.szRtspURLTrim) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_rtspStruct.szRtspURLTrim[strlen(m_rtspStruct.szRtspURLTrim) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic, szSessionID);
		}
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);

	nRtspProcessStep = RtspProcessStep_SETUP;

#ifdef PrintRtspLogFlag
	Rtsp_WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);
#endif

	CSeq++;
}

void  CNetClientRecvRtsp::SendPlay(WWW_AuthenticateType wwwType)
{//\r\nScale: 255
 
 //�� session ����; �����ַ���ȥ��  
	string strSession = szSessionID;
	int    nPos2 = strSession.find(";", 0);
	if (nPos2 > 0)
	{
		szSessionID[nPos2] = 0x00;
		Rtsp_WriteLog(Log_Debug, "SendPlay() ��nClient = %llu ,strSessionID = %s , szSessionID = %s ", nClient, strSession.c_str(), szSessionID);
	}

	if (wwwType == WWW_Authenticate_None)
	{
		if (m_nXHRtspURLType == XHRtspURLType_RecordDownload)
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nScale: 255\r\nUser-Agent: %s\r\nSession: %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID);
		else 
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PLAY", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		if (m_nXHRtspURLType == XHRtspURLType_RecordDownload)
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nScale: 255\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
		else 
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);


		if (m_nXHRtspURLType == XHRtspURLType_RecordDownload)
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nScale: 255\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID, szBasic);
		else
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID, szBasic);
	}
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);

	nRtspProcessStep = RtspProcessStep_PLAY;

	//�޸����ӳɹ�,�Ѵ�������״̬�޸�Ϊ�ɹ�
	boost::shared_ptr<CNetRevcBase>   pParent = GetNetRevcBaseClientNoLock(m_hParent);
	if (pParent != NULL)
	   pParent->nRtspConnectStatus = RtspConnectStatus_ConnectSuccess;

#ifdef PrintRtspLogFlag
	Rtsp_WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);
#endif
	CSeq++;
}

//��ͣ
bool CNetClientRecvRtsp::RtspPause()
{
	if (bPauseFlag || nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
		return false;

	sprintf(szResponseBuffer, "PAUSE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	bPauseFlag = true;
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp = %X,nClient = %d ,RtspPause() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}

//����
bool CNetClientRecvRtsp::RtspResume()
{
	if (!bPauseFlag || nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
		return false;
	sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, CSeq, MediaServerVerson, szSessionID);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	bPauseFlag = false;
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp = %X,nClient = %d ,RtspResume() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}

//���ٲ���
bool  CNetClientRecvRtsp::RtspSpeed(int nSpeed)
{
	if (nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
		return false;

	sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nScale: %d\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, nSpeed, CSeq, MediaServerVerson, szSessionID);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	bPauseFlag = false;
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp = %X,nClient = %d ,RtspSpeed() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}

//�϶�����
bool CNetClientRecvRtsp::RtspSeek(int64_t nSeekTime)
{
	if (nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
		return false;

	sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nScale: %d\r\nCSeq: %d\r\nRange: npt=%d-0\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_rtspStruct.szRtspURLTrim, 1, CSeq, nSeekTime, MediaServerVerson, szSessionID);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	bPauseFlag = false;
	Rtsp_WriteLog(Log_Debug, "CNetClientRecvRtsp = %X, nClient = %d ,RtspSeek() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}
