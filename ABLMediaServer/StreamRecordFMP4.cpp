/*
���ܣ�
    �������������Ϊmp4(fmp4��ʽ) 
	 
����    2022-01-09
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "StreamRecordFMP4.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSourceNoLock(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern boost::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

static int StreamRecordFMP4_hls_segment(void* param, const void* data, size_t bytes, int64_t pts, int64_t dts, int64_t duration)
{
	CStreamRecordFMP4* pNetServerHttpMp4 = (CStreamRecordFMP4*)param;
	if (pNetServerHttpMp4 == NULL)
		return 0;
	
	if(!pNetServerHttpMp4->bCheckHttpMP4Flag || !pNetServerHttpMp4->bRunFlag)
		return -1 ;

	if (bytes > 0)
	{
		pNetServerHttpMp4->writeTSBufferToMP4File((unsigned char*)data, bytes);
	}

	return 0;
}

CStreamRecordFMP4::CStreamRecordFMP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	memset((char*)&avc, 0x00, sizeof(avc));
	memset((char*)&hevc, 0x00, sizeof(hevc));

	nCurrentVideoFrames = 0;//��ǰ��Ƶ֡��
	nTotalVideoFrames = 0;//¼����Ƶ��֡��

	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_RecordFile_FMP4;
	nMediaClient = 0;
	bAddSendThreadToolFlag = false;
	bWaitIFrameSuccessFlag = false;
	fWriteMP4 = NULL;

	nClient = hClient;
	hls_init_segmentFlag = false;
	audioDts = 0;
	videoDts = 0;
	track_aac = -1;
	track_video = -1;
	hlsFMP4 = NULL;
 	strcpy(szClientIP, szIP);
	nClientPort = nPort;

	memset(netDataCache ,0x00,sizeof(netDataCache));
	MaxNetDataCacheCount = sizeof(netDataCache);
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//�������ݻ����С
	bFindMP4NameFlag = false;
	memset(szMP4Name, 0x00, sizeof(szMP4Name));
	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

	bCheckHttpMP4Flag = true;
	nCreateDateTime = GetTickCount64();

	WriteLog(Log_Debug, "CStreamRecordFMP4 ���� = %X nClient = %llu ", this, nClient);
}

CStreamRecordFMP4::~CStreamRecordFMP4()
{
	bCheckHttpMP4Flag = bRunFlag = false ;
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	//ɾ��fmp4��Ƭ���
	if (hlsFMP4 != NULL)
	{
		hls_fmp4_destroy(hlsFMP4);
		hlsFMP4 = NULL;
    }
 
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
 
	if (fWriteMP4)
	{
 		fclose(fWriteMP4);
		fWriteMP4 = NULL;

		//���һ��fmp4��Ƭ�ļ�֪ͨ 
		if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.nClientRecordMp4 > 0)
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = ABL_MediaServerPort.nClientRecordMp4;
			sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate));
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}
	}

	WriteLog(Log_Debug, "CStreamRecordFMP4 ���� = %X nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CStreamRecordFMP4::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag)
		return -1;
	nRecvDataTimerBySecond = 0;
	nCurrentVideoFrames ++;//��ǰ��Ƶ֡��
	nTotalVideoFrames ++ ;//¼����Ƶ��֡��

	m_videoFifo.push(pVideoData, nDataLength);

	if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.nClientRecordProgress > 0 && (GetTickCount64() - nCreateDateTime ) >= 1000 )
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = ABL_MediaServerPort.nClientRecordProgress;
		sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%llu,\"key\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"TotalVideoDuration\":%llu}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType,key, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), (nTotalVideoFrames / mediaCodecInfo.nVideoFrameRate));
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		nCreateDateTime = GetTickCount64();
	}
	return 0;
}

int CStreamRecordFMP4::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag)
		return -1;

	if (strcmp(mediaCodecInfo.szAudioName, "AAC") != 0)
		return 0;

	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

int CStreamRecordFMP4::SendVideo()
{
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	if (!bCheckHttpMP4Flag)
		return -1;

	nRecvDataTimerBySecond = 0;

	if (!bCheckHttpMP4Flag)
		return -1;

	if (nVideoStampAdd == 0)
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	if (hlsFMP4 == NULL)
		hlsFMP4 = hls_fmp4_create((1) * 1000, StreamRecordFMP4_hls_segment, this);

	videoDts += nVideoStampAdd;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (hlsFMP4)
			VideoFrameToFMP4File(pData, nLength);

		m_videoFifo.pop_front();
	}
}

int CStreamRecordFMP4::SendAudio()
{
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	if (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "AAC") != 0 || !bCheckHttpMP4Flag)
		return 0 ;
   
 	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (ABL_MediaServerPort.nEnableAudio == 1 && strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		{
			if (nAsyncAudioStamp == -1)
				nAsyncAudioStamp = GetTickCount();

			avtype = PSI_STREAM_AAC;

			if (hlsFMP4 != NULL && track_video >= 0)
			{
				if (track_aac == -1)
				{
					nAACLength = mpeg4_aac_adts_frame_length(pData, nLength);
					if (nAACLength < 0)
					{
						m_audioFifo.pop_front();
						return false;
					}

					mpeg4_aac_adts_load(pData, nLength, &aacHandle);
					nExtenAudioDataLength = mpeg4_aac_audio_specific_config_save(&aacHandle, szExtenAudioData, sizeof(szExtenAudioData));
					if (nExtenAudioDataLength > 0)
					{
						track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_AAC, mediaCodecInfo.nChannels, 16, mediaCodecInfo.nSampleRate, szExtenAudioData, nExtenAudioDataLength);
					}
				}

				//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
				if (track_aac >= 0 && hls_init_segmentFlag)
				{
					hls_fmp4_input(hlsFMP4, track_aac, pData + 7, nLength - 7, audioDts, audioDts, 0);
				}
			}

			audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;

			//500����ͬ��һ�� 
			if (GetTickCount() - nAsyncAudioStamp >= 500)
			{
				if (videoDts < audioDts)
				{
					nVideoStampAdd = (1000 / mediaCodecInfo.nVideoFrameRate) + 5;
				}
				else if (videoDts > audioDts)
				{
					nVideoStampAdd = (1000 / mediaCodecInfo.nVideoFrameRate) - 5;
				}
				nAsyncAudioStamp = GetTickCount();
			}
		}
		m_audioFifo.pop_front();
	}
	return 0;
}

int CStreamRecordFMP4::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	
	return 0;
}

int CStreamRecordFMP4::ProcessNetData()
{
	return 0;
}

//���͵�һ������
int CStreamRecordFMP4::SendFirstRequst()
{

	 return 0;
}

//����m3u8�ļ�
bool  CStreamRecordFMP4::RequestM3u8File()
{
	return true;
}

static int fmp4_hls_init_segment(hls_fmp4_t* hls, void* param)
{
	CStreamRecordFMP4* pNetServerHttpMp4 = (CStreamRecordFMP4*)param;
	if (pNetServerHttpMp4 == NULL)
		return 0;

	int bytes = hls_fmp4_init_segment(hls, pNetServerHttpMp4->s_packet, sizeof(pNetServerHttpMp4->s_packet));

	pNetServerHttpMp4->fTSFileWriteByteCount = pNetServerHttpMp4->nFmp4SPSPPSLength = bytes;
	pNetServerHttpMp4->s_packetLength = bytes;
	bool  bUpdateFlag = false;

	if (pNetServerHttpMp4->fWriteMP4 == NULL)
	{
#ifdef OS_System_Windows
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(pNetServerHttpMp4->szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", pNetServerHttpMp4->szRecordPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		sprintf(pNetServerHttpMp4->szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
		time_t now;
		time(&now);
		struct tm *local;
		local = localtime(&now);
		sprintf(pNetServerHttpMp4->szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", pNetServerHttpMp4->szRecordPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
		sprintf(pNetServerHttpMp4->szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
#endif
			boost::shared_ptr<CRecordFileSource> pRecord = GetRecordFileSource(pNetServerHttpMp4->m_szShareMediaURL);
			if (pRecord)
			{
				bUpdateFlag = pRecord->UpdateExpireRecordFile(pNetServerHttpMp4->szFileName);
				if (bUpdateFlag)
				{
					pNetServerHttpMp4->fWriteMP4 = fopen(pNetServerHttpMp4->szFileName, "r+b");
					if (pNetServerHttpMp4->fWriteMP4)
						fseek(pNetServerHttpMp4->fWriteMP4, 0, SEEK_SET);
				}
				else
				   pNetServerHttpMp4->fWriteMP4 = fopen(pNetServerHttpMp4->szFileName, "wb");

				if (pNetServerHttpMp4->fWriteMP4)
				{
				   pRecord->AddRecordFile(pNetServerHttpMp4->szFileNameOrder);
				   WriteLog(Log_Debug, "CStreamRecordFMP4 = %X %s ����¼���ļ� nClient = %llu ,nMediaClient = %llu szFileNameOrder %s ", pNetServerHttpMp4, pNetServerHttpMp4->m_szShareMediaURL, pNetServerHttpMp4->nClient, pNetServerHttpMp4->nMediaClient, pNetServerHttpMp4->szFileNameOrder);
 		         }
  			}
 	}

	if (pNetServerHttpMp4->fWriteMP4)
	{
		fwrite(pNetServerHttpMp4->s_packet, 1, bytes, pNetServerHttpMp4->fWriteMP4);
		fflush(pNetServerHttpMp4->fWriteMP4);
	}
	//����hls_init_segment ��ʼ����ɲ���д��Ƶ����Ƶ�Σ��ڻص�������������־
	pNetServerHttpMp4->hls_init_segmentFlag = true;

	return 0;
}

bool  CStreamRecordFMP4::VideoFrameToFMP4File(unsigned char* szVideoData, int nLength)
{
	if (track_video < 0 )
	{
		bool bFind = false;
		int  nPos = -1;
		int  nWidth = 0 , nHeight = 0;
		nPos = FindSpsPosition(szVideoData, nLength, bFind);
		if (!bFind)
			return false;

		int n;
		//vcl �� update ��Ҫ��ֵΪ 0 ���������ױ��� 
		vcl = 0;
		update = 0;        
		if (memcmp(mediaCodecInfo.szVideoName, "H264", 4) == 0)
		{
			if (bFind && nPos >= 0)
				GetWidthHeightFromSPS(szVideoData + nPos, nLength - nPos, nWidth, nHeight);
			if (nWidth <= 0 || nHeight <= 0)
				return false;
			n = h264_annexbtomp4(&avc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		}
		else if (memcmp(mediaCodecInfo.szVideoName, "H265", 4) == 0)
		{
			if (bFind && nPos >= 0)
				ParseSequenceParameterSet(szVideoData + nPos, nLength - nPos, H265Params);
			if (H265Params.width <= 0 || H265Params.height <= 0)
				return false;
			n = h265_annexbtomp4(&hevc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		}
		else
			return false;

		if (track_video < 0)
		{
			memset(szExtenVideoData, 0x00, sizeof(szExtenVideoData));
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			{//H264 �ȴ� SPS��PPS �ķ��� 
				if (avc.nb_sps < 1 || avc.nb_pps < 1)
				{
 					return false  ; // waiting for sps/pps
				}
				extra_data_size = mpeg4_avc_decoder_configuration_record_save(&avc, szExtenVideoData, sizeof(szExtenVideoData));
			}
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			{//H265 �ȴ�SPS��PPS�ķ��� 
				if (hevc.numOfArrays < 1)
				{
 					return false ; // waiting for vps/sps/pps
				}
			    extra_data_size = mpeg4_hevc_decoder_configuration_record_save(&hevc, szExtenVideoData, sizeof(szExtenVideoData));
			}
			else
				return false;

			if (extra_data_size <= 0)
			{
				return false;
			}

			if (extra_data_size > 0)
			{
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				  track_video = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_H264, nWidth, nHeight, szExtenVideoData, extra_data_size);
				else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				  track_video = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_HEVC, H265Params.width, H265Params.height, szExtenVideoData, extra_data_size);
			}
		}
	}

	if (track_video >= 0 && hlsFMP4 != NULL )
	{
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, szVideoData, nLength) == true)
		{
			if (!bWaitIFrameSuccessFlag)
				bWaitIFrameSuccessFlag = true;
			flags = 1;
		}
		else
			flags = 0;

		if (!bWaitIFrameSuccessFlag)
			return false ;

		vcl = 0;
		update = 0;        

		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			nMp4BufferLength = h264_annexbtomp4(&avc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			nMp4BufferLength = h265_annexbtomp4(&hevc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		else
			return false;

		//����Ƶ��� ������ �ȴ���Ƶ����30֡ʱ����û������Ƶ���֤��������û����Ƶ 
		if (nMp4BufferLength > 0 && (track_aac >= 0 || (videoDts / 40 > 30)))
		{
			if (hls_init_segmentFlag == false)
			{
				fmp4_hls_init_segment(hlsFMP4, this);
			}

			//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
			if (hls_init_segmentFlag == true)
				hls_fmp4_input(hlsFMP4, track_video, pH265Buffer, nMp4BufferLength, videoDts, videoDts, (flags == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
	}

	return true;
}

bool CStreamRecordFMP4::writeTSBufferToMP4File(unsigned char* pTSData, int nLength)
{
	bool bUpdateFlag = false;
	if (fWriteMP4 && pTSData != NULL && nLength > 0)
	{
		fwrite(pTSData, 1, nLength, fWriteMP4);
		fflush(fWriteMP4);

		if ((nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate) >= ABL_MediaServerPort.fileSecond)
		{
			fclose(fWriteMP4);

			//���һ��fmp4��Ƭ�ļ�֪ͨ 
			if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.nClientRecordMp4 > 0 )
			{
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = ABL_MediaServerPort.nClientRecordMp4;
				sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate));
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}
			nCurrentVideoFrames = 0;

#ifdef OS_System_Windows
			SYSTEMTIME st;
			GetLocalTime(&st);
			sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", szRecordPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		    sprintf(szFileNameOrder,"%04d%02d%02d%02d%02d%02d.mp4", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
			time_t now;
			time(&now);
			struct tm *local;
			local = localtime(&now);
			sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", szRecordPath, local->tm_year + 1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
		    sprintf(szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
#endif
  				
			boost::shared_ptr<CRecordFileSource> pRecord = GetRecordFileSource(m_szShareMediaURL);
			if (pRecord)
			{
 				bUpdateFlag = pRecord->UpdateExpireRecordFile(szFileName);
				if (bUpdateFlag)
				{
					fWriteMP4 = fopen(szFileName, "r+b");
					if (fWriteMP4)
						fseek(fWriteMP4, 0, SEEK_SET);
				}
				else
					fWriteMP4 = fopen(szFileName, "wb");


				if (fWriteMP4 != NULL)
				{
					fwrite(s_packet, 1, s_packetLength, fWriteMP4);
					fflush(fWriteMP4);
				}
				pRecord->AddRecordFile(szFileNameOrder);
				WriteLog(Log_Debug, "CStreamRecordFMP4 = %X %s ����¼���ļ� nClient = %llu ,nMediaClient = %llu szFileNameOrder %s ", this, m_szShareMediaURL, nClient, nMediaClient, szFileNameOrder);
 			}				
 
			nCreateDateTime = GetTickCount64();
		}

		return true;
	}
	else
		return false;
}