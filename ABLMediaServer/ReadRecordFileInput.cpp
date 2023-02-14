/*
���ܣ�
        ʵ�ֶ�ȡ¼���ļ�����ý��Դ������Ƶ����Ƶ���ݣ��γ�ý��Դ
����    2022-01-18
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/
#include "stdafx.h"
#include "ReadRecordFileInput.h"
#ifdef USE_BOOST
extern CNetBaseThreadPool* RecordReplayThreadPool;//¼��ط��̳߳�
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);

extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern MediaServerPort                       ABL_MediaServerPort;
#else
extern CNetBaseThreadPool* RecordReplayThreadPool;//¼��ط��̳߳�
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);

extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern MediaServerPort                       ABL_MediaServerPort;
#endif


#if defined(_WIN32) || defined(_WIN64)
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#elif defined(OS_LINUX)
#define fseek64 fseeko64
#define ftell64 ftello64
#else
#define fseek64 fseek
#define ftell64 ftell
#endif

static int mov_file_read(void* fp, void* data, uint64_t bytes)
{
	if (bytes == fread(data, 1, bytes, (FILE*)fp))
		return 0;
	return 0 != ferror((FILE*)fp) ? ferror((FILE*)fp) : -1 /*EOF*/;
}

static int mov_file_write(void* fp, const void* data, uint64_t bytes)
{
	return bytes == fwrite(data, 1, bytes, (FILE*)fp) ? 0 : ferror((FILE*)fp);
}

static int mov_file_seek(void* fp, int64_t offset)
{
	return fseek64((FILE*)fp, offset, SEEK_SET);
}

static int64_t mov_file_tell(void* fp)
{
	return ftell64((FILE*)fp);
}

const struct mov_buffer_t* mov_file_buffer(void)
{
	static struct mov_buffer_t s_io = {
		mov_file_read,
		mov_file_write,
		mov_file_seek,
		mov_file_tell,
	};
	return &s_io;
}

static void ReadRecordFileInput_onread(void* param, uint32_t track, const void* buffer, size_t bytes, int64_t pts, int64_t dts, int flags)
{
	CReadRecordFileInput* pThis = (CReadRecordFileInput*)param;
	if (pThis == NULL )
		return;

 	if (pThis->s_avc_track == track || pThis->s_hevc_track == track)
	{
		pThis->nAVType = AVType_Video;
		if(pThis->nVideoFirstPTS == 0)
			pThis->nVideoFirstPTS = pts;
		if (pThis->nVideoFirstPTS != 0)
		{
			if (pThis->bRestoreVideoFrameFlag)
			{//���¼����Ѿ�������Ƶ֡��������Ϊ�Ѿ��϶�����
				pThis->nReadVideoFrameCount = ((pts - pThis->nVideoFirstPTS) / (1000 / pThis->mediaCodecInfo.nVideoFrameRate));
				pThis->bRestoreVideoFrameFlag = false;
			}
			else
				pThis->nReadVideoFrameCount ++;
		}

		if(pThis->s_avc_track == track)
		  pThis->nRetLength = h264_mp4toannexb(&pThis->s_avc, buffer, bytes, pThis->s_packet+4, sizeof(pThis->s_packet));
		else 
		  pThis->nRetLength = h265_mp4toannexb(&pThis->s_hevc, buffer, bytes, pThis->s_packet+4, sizeof(pThis->s_packet));

 		if (pThis->nRetLength > 0)
		{
			memcpy(pThis->s_packet, (unsigned char*)&pThis->nReadVideoFrameCount, sizeof(pThis->nReadVideoFrameCount));
			if (abs(pThis->m_dScaleValue - 8.0) <= 0.01 || abs(pThis->m_dScaleValue - 16.0) <= 0.01)
			{//��֡
				if (pThis->m_rtspPlayerType == RtspPlayerType_RecordReplay)
				{//¼��ط�
				  if (pThis->CheckVideoIsIFrame(pThis->mediaCodecInfo.szVideoName, pThis->s_packet+4, pThis->nRetLength))
					  pThis->pMediaSource->PushVideo(pThis->s_packet, pThis->nRetLength+4, pThis->mediaCodecInfo.szVideoName);
 				}else //¼������
					pThis->pMediaSource->PushVideo(pThis->s_packet, pThis->nRetLength + 4, pThis->mediaCodecInfo.szVideoName);
			}
			else
				pThis->pMediaSource->PushVideo(pThis->s_packet, pThis->nRetLength+4, pThis->mediaCodecInfo.szVideoName);
		}
		//������ƵԴ��֡�ٶ�
		if ( pThis->pMediaSource != NULL)
		{
			if (pThis->pMediaSource->bUpdateVideoSpeed == false)
			{
				//������Ƶ֡�ٶ�
				pThis->nVidepSpeedTime = pts - pThis->nOldPTS;
				pThis->nOldPTS = pts;
				if (pThis->nVidepSpeedTime <= 5)
					pThis->nVidepSpeedTime = 40;
				else if (pThis->nVidepSpeedTime > 1000)
					pThis->nVidepSpeedTime = 40;
				pThis->dBaseSpeed = pThis->nVidepSpeedTime;
				pThis->m_nStartTimestamp = pts;//��ʼ��ʱ���,���϶�����ʱʹ�õ�
											   
		       int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(pts, 1000);
			   if (nVideoSpeed > 0)
			   {
			     WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
			     pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
 			   }
 			}
  		}
  	}
	else if (pThis->s_av1_track == track)
	{
 
	}
	else if (pThis->s_vpx_track == track)
	{
 
	}
	else if (pThis->s_aac_track == track)
	{
		pThis->nAVType = AVType_Audio;

		if (pThis->nAudioFirstPTS == 0)
 		  pThis->nAudioFirstPTS = pts;

		if (pThis->bRestoreAudioFrameFlag)
		{//���¼�����Ƶ֡���� ����Ϊ�Ѿ��ϵ�����
		    pThis->nReadAudioFrameCount = ((pts - pThis->nAudioFirstPTS) / pThis->mediaCodecInfo.nBaseAddAudioTimeStamp);
			pThis->bRestoreAudioFrameFlag = false;
		}
		else
			pThis->nReadAudioFrameCount++;

		pThis->nRetLength = mpeg4_aac_adts_save(&pThis->s_aac, bytes, pThis->audioBuffer+4, sizeof(pThis->audioBuffer));
		if (pThis->nRetLength > 0)
		{
			memcpy(pThis->audioBuffer, (unsigned char*)&pThis->nReadAudioFrameCount, sizeof(pThis->nReadAudioFrameCount));
			memcpy(pThis->audioBuffer + (4 + pThis->nRetLength), buffer, bytes);
			pThis->pMediaSource->PushAudio(pThis->audioBuffer, 4 + pThis->nRetLength + bytes,pThis->mediaCodecInfo.szAudioName, pThis->mediaCodecInfo.nChannels, pThis->mediaCodecInfo.nSampleRate);
		}
 	}
	else if (pThis->s_opus_track == track)
	{
 
	}
	else if (pThis->s_mp3_track == track)
	{
 
	}
	else if (pThis->s_subtitle_track == track)
	{
 
	}
	else
	{
 
	}
 }

static void ReadRecordFileInput_mov_video_info(void* param, uint32_t track, uint8_t object, int /*width*/, int /*height*/, const void* extra, size_t bytes)
{
	CReadRecordFileInput* pThis = (CReadRecordFileInput*)param;
	if (pThis == NULL)
		return;

	if (MOV_OBJECT_H264 == object)
	{
		strcpy(pThis->mediaCodecInfo.szVideoName, "H264");
		pThis->s_avc_track = track;
		mpeg4_avc_decoder_configuration_record_load((const uint8_t*)extra, bytes, &pThis->s_avc);
	}
	else if (MOV_OBJECT_HEVC == object)
	{
		strcpy(pThis->mediaCodecInfo.szVideoName, "H265");
		pThis->s_hevc_track = track;
		mpeg4_hevc_decoder_configuration_record_load((const uint8_t*)extra, bytes, &pThis->s_hevc);
	}
	else if (MOV_OBJECT_AV1 == object)
	{
		pThis->s_av1_track = track;
	}
	else if (MOV_OBJECT_VP9 == object)
	{
		pThis->s_vpx_track = track;
	}
	else
	{
 	}
}

static void ReadRecordFileInput_mov_audio_info(void*  param, uint32_t track, uint8_t object, int channel_count, int bit_per_sample, int sample_rate, const void* extra, size_t bytes)
{
	CReadRecordFileInput* pThis = (CReadRecordFileInput*)param;
	if (pThis == NULL)
		return;

	if (MOV_OBJECT_AAC == object)// MOV_OBJECT_AAC
	{
		//��¼��Ƶ��Ϣ
		strcpy(pThis->mediaCodecInfo.szAudioName, "AAC");
		pThis->mediaCodecInfo.nChannels = channel_count;
		pThis->mediaCodecInfo.nSampleRate = sample_rate;

		pThis->s_aac_track = track;
		mpeg4_aac_audio_specific_config_load((const uint8_t*)extra, bytes, &pThis->s_aac);
	}
	else if (MOV_OBJECT_OPUS == object)
	{
		pThis->s_opus_track = track;
	}
	else if (MOV_OBJECT_MP3 == object || MOV_OBJECT_MP1A == object)
	{
		pThis->s_mp3_track = track;
	}
	else
	{
		if (object == MOV_OBJECT_G711a || object == MOV_OBJECT_G711u)
		{
			//s_aac_track = track;
			//s_aac.channel_configuration = channel_count;
		}

		//s_aac.sampling_frequency_index = mpeg4_aac_audio_frequency_from(sample_rate);
	}
}

static void ReadRecordFileInput_mov_subtitle_info(void* /*param*/, uint32_t track, uint8_t object, const void* /*extra*/, size_t /*bytes*/)
{
	 
}

//�ӻطŵ�¼�����ֻ�ȡ�㲥����url 
bool  CReadRecordFileInput::GetMediaShareURLFromFileName(char* szRecordFileName,char* szMediaURL)
{
	if (szRecordFileName == NULL || strlen(szRecordFileName) == 0 || szMediaURL == NULL || strlen(szMediaURL) == 0)
		return false;

	string strRecordFileName = szRecordFileName;
#ifdef OS_System_Windows
#ifdef USE_BOOST
	replace_all(strRecordFileName, "\\", "/");
#else
	ABL::replace_all(strRecordFileName, "\\", "/");
#endif

#endif
	int   nPos;
	char  szTempFileName[512] = { 0 };
	nPos = strRecordFileName.rfind("/", strlen(szRecordFileName));
	if (nPos > 0)
	{
		memcpy(szTempFileName, szRecordFileName + nPos+1, strlen(szRecordFileName) - nPos);
		szTempFileName[strlen(szTempFileName) - 4] = 0x00;
		sprintf(m_szShareMediaURL, "%s%s%s", szMediaURL, RecordFileReplaySplitter, szTempFileName);
		return true;
	}else 
 	  return false;
}

CReadRecordFileInput::CReadRecordFileInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	nDownloadFrameCount = 0;
	if (strlen(szIP) <= 4 || memcmp(szIP+(strlen(szIP) - 4),".mp4",4) != 0)
	{
		WriteLog(Log_Debug, "CReadRecordFileInput ����ý��Դʧ�� = %X ,¼���ļ��������� szIP = %s ", this, szIP);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}

	WriteLog(Log_Debug, "CReadRecordFileInput ���캯�� = %X ,nClient = %llu ,��ȡ¼���ļ� %s", this, hClient, szIP);

	m_rtspPlayerType = RtspPlayerType_RecordReplay;
	pMediaSource = NULL ;
	nClient    = hClient;
	if (GetMediaShareURLFromFileName(szIP, szShareMediaURL) == false)
	{
		WriteLog(Log_Debug, "CReadRecordFileInput ����ý��Դʧ�� = %X ,��װ¼��ط�urlʧ�� ¼���ļ��� = %s ,szShareMediaURL = %s ", this, szIP, szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
    
	memset((char*)&s_avc, 0x00, sizeof(s_avc));
	memset((char*)&s_hevc, 0x00, sizeof(s_hevc));

	s_aac_track  = 0xFFFFFFFF;
	s_avc_track  = 0xFFFFFFFF;
	s_av1_track  = 0xFFFFFFFF;
	s_vpx_track  = 0xFFFFFFFF;
	s_hevc_track = 0xFFFFFFFF;
	s_opus_track = 0xFFFFFFFF;
	s_mp3_track  = 0xFFFFFFFF;
	s_subtitle_track = 0xFFFFFFFF;

	WriteLog(Log_Debug, "CReadRecordFileInput =  %X ,nClient = %llu ��ʼ��ȡ¼���ļ� %s ", this, nClient, szIP);

	fp = fopen(szIP, "rb");
	if (fp == NULL)
	{
		WriteLog(Log_Debug, "CReadRecordFileInput ��ȡ�ļ�ʧ�� =  %X ,nClient = %llu RecordFile %s ", this, nClient, szIP);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	mov = mov_reader_create(mov_file_buffer(), fp);
	if (mov == NULL)
	{
		WriteLog(Log_Debug, "CReadRecordFileInput ��ȡ�ļ�ʧ�� =  %X ,nClient = %llu RecordFile %s ", this, nClient, szIP);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
 	}

	duration =  mov_reader_getduration(mov);
	struct mov_reader_trackinfo_t info = { ReadRecordFileInput_mov_video_info, ReadRecordFileInput_mov_audio_info, ReadRecordFileInput_mov_subtitle_info };
	mov_reader_getinfo(mov, &info, this);

	//����¼��㲥ý��Դ 
	pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hClient, MediaSourceType_ReplayMedia, duration / 1000, m_h265ConvertH264Struct);
	if (pMediaSource == NULL)
	{
		WriteLog(Log_Debug, "CReadRecordFileInput ����ý��Դʧ�� =  %X ,nClient = %llu m_szShareMediaURL %s ", this, hClient, m_szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}

    netBaseNetType = ReadRecordFileInput_ReadFMP4File;
	strcpy(m_addStreamProxyStruct.app, pMediaSource->app);
	strcpy(m_addStreamProxyStruct.stream, pMediaSource->stream);
	strcpy(m_addStreamProxyStruct.url, szIP);

	nAVType = AVType_Audio;
	mov_readerTime = GetTickCount64();
	nOldPTS = 0;
	nVidepSpeedTime = 40;
	dBaseSpeed = 40.00;
	m_dScaleValue = 1.00;
	m_bPauseFlag = false;
	m_nStartTimestamp = 0;
	nReadVideoFrameCount = nReadAudioFrameCount = 0;
	nVideoFirstPTS = 0 ;
	nAudioFirstPTS = 0;
	 
	bRestoreVideoFrameFlag = false ;//�Ƿ���Ҫ�ָ���Ƶ֡����
	bRestoreAudioFrameFlag = false ;//�Ƿ���Ҫ�ָ���Ƶ֡����

 	RecordReplayThreadPool->InsertIntoTask(nClient);
	WriteLog(Log_Debug, "CReadRecordFileInput ���캯�� = %X ,nClient = %llu , m_szShareMediaURL = %s , ¼���ļ� %s ʱ�� %llu �� ", this, hClient, m_szShareMediaURL, szIP, duration / 1000 );
}

CReadRecordFileInput::~CReadRecordFileInput() 
{
 	WriteLog(Log_Debug, "CReadRecordFileInput �������� = %X ,nClient = %llu ", this, nClient);
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);

	if (mov)
	{
	  mov_reader_destroy(mov);
	  mov = NULL;
	}
	if(fp)
 	   fclose(fp);

	//ɾ���ַ�Դ
	if (strlen(m_szShareMediaURL) > 0)
	   DeleteMediaStreamSource(m_szShareMediaURL);

   malloc_trim(0);
}

int CReadRecordFileInput::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{

  return 0 ;	
}

int CReadRecordFileInput::ProcessNetData() 
{
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);
	nRecvDataTimerBySecond = 0;

	if (mov == NULL || m_bPauseFlag == true )
	{
		//Sleep(2);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		mov_readerTime = GetTickCount64();
		RecordReplayThreadPool->InsertIntoTask(nClient);
		return -1;
	}

	nReadRet = mov_reader_read(mov, s_buffer, sizeof(s_buffer), ReadRecordFileInput_onread, this);

	if (nAVType == AVType_Video)
	{//��ȡ��Ƶ
		mov_readerTime = GetTickCount64();

		if ((abs(m_dScaleValue - 8.0) <= 0.01 || abs(m_dScaleValue - 16.0) <= 0.01))
		{//8��16���ٲ���Ҫ�ȴ� 
			if (m_rtspPlayerType == RtspPlayerType_RecordReplay)
			{//¼��ط�
				if (nReadVideoFrameCount % 25 == 0)
				{
					if (abs(m_dScaleValue - 8.0) <= 0.01)
						std::this_thread::sleep_for(std::chrono::milliseconds(60));
						//Sleep(60);
					else
						std::this_thread::sleep_for(std::chrono::milliseconds(30));
						//Sleep(30);
				}
			}
			else//¼������
			{
				nDownloadFrameCount++;
				if (nDownloadFrameCount % 10 == 0)
				{
				  if (abs(m_dScaleValue - 8.0) <= 0.01)
					  std::this_thread::sleep_for(std::chrono::milliseconds(50));
					 //Sleep(50);
				  else if (abs(m_dScaleValue - 16.0) <= 0.01)
					  std::this_thread::sleep_for(std::chrono::milliseconds(40));
					 //Sleep(40);
 				}
			}
		}
 		else if (abs(m_dScaleValue - 255.0) <= 0.01 )
		{//rtsp¼������
			nDownloadFrameCount ++;
			if(nDownloadFrameCount % 10 == 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(35));
				//Sleep(35);
 		}
		else if (abs(m_dScaleValue - 1.0) <= 0.01)
		{//��ͨ�ĵ㲥�ط�
			if (((1000 / mediaCodecInfo.nVideoFrameRate) - 10) > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds((1000 / mediaCodecInfo.nVideoFrameRate) - 10));
				//Sleep((1000 / mediaCodecInfo.nVideoFrameRate) - 10);
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				//Sleep(10);
		}
		else 
		{//��ȡ��Ƶ��ʱ����δ������ҪSleep(2) ,����CPU�����
			  if ( !(abs(m_dScaleValue - 8.0) <= 0.01 || abs(m_dScaleValue - 16.0) <= 0.01) )
				  std::this_thread::sleep_for(std::chrono::milliseconds(2));
			    //Sleep(2); //8���١�16���٣�����ҪSleeep
 		}
 	}
	else if (nAVType == AVType_Audio)  
	{//��Ƶֱ�Ӷ�ȡ

  	}

	if (nReadRet == 0)
	{//¼���ļ���ȡ��� 
		if (ABL_MediaServerPort.fileRepeat == 1 && !(abs(m_dScaleValue - 255.0) <= 0.01) )
		{//���ļ���ȡ��ϣ����Ҳ���rtsp����ʱ�������¶�ȡ�ļ�
			int64_t nStartTime = 0;
			int nRet = mov_reader_seek(mov, &nStartTime);
		    WriteLog(Log_Debug, "ProcessNetData �ļ���ȡ��� ,nClient = %llu ,���������ٴβ��� ,nRet = %d ", nClient, nRet);
  		}
		else
		{
		   WriteLog(Log_Debug, "ProcessNetData �ļ���ȡ��� ,nClient = %llu ", nClient);
 		   DeleteNetRevcBaseClient(nClient);
		   return -1;
		}
	}
	else if (nReadRet < 0)
	{//�ļ���ȡ���� 
		WriteLog(Log_Debug, "ProcessNetData �ļ���ȡ���� ,nClient = %llu ",  nClient);
		DeleteNetRevcBaseClient(nClient);
		return -1;
	}
 
	RecordReplayThreadPool->InsertIntoTask(nClient);

    return 0 ;	
}

//����¼��ط��ٶ�
bool CReadRecordFileInput::UpdateReplaySpeed(double dScaleValue, ABLRtspPlayerType rtspPlayerType)
{
	double dCalcSpeed = 40.00;
	dCalcSpeed = (dBaseSpeed / dScaleValue);
	nVidepSpeedTime = (int)dCalcSpeed;
	m_dScaleValue = dScaleValue;
	m_rtspPlayerType = rtspPlayerType;
	WriteLog(Log_Debug, "UpdateReplaySpeed ����¼��ط��ٶ� dScaleValue = %.2f ,nClient = %llu ,dCalcSpeed = %.2f, nVidepSpeedTime = %d , m_rtspPlayerType = %d ", dScaleValue, nClient, dCalcSpeed, nVidepSpeedTime, m_rtspPlayerType);

	return true;
}

bool CReadRecordFileInput::UpdatePauseFlag(bool bFlag)
{
	m_bPauseFlag = bFlag;
	WriteLog(Log_Debug, "UpdatePauseFlag ������ͣ���ű�־ ,nClient = %llu ,m_bPauseFlag = %d  ", nClient, m_bPauseFlag);
	return true;
}

bool  CReadRecordFileInput::ReaplyFileSeek(uint64_t nTimestamp)
{
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);
	if (mov == NULL || m_bPauseFlag == true)
 		return false;
	if (nTimestamp > (duration / 1000))
	{
		WriteLog(Log_Debug, "ReaplyFileSeek �϶�ʱ��������ļ����ʱ�� ,nClient = %llu ,nTimestamp = %llu ,duration = %d ", nClient, nTimestamp, duration / 1000 );
		return false; 
	}
	int64_t nSeekToTime = m_nStartTimestamp + (nTimestamp * 1000) ;////��ʼ��ʱ��� ���� �϶�����ʱ��� ��������ȷ
	int nRet = mov_reader_seek(mov, &nSeekToTime);

	bRestoreVideoFrameFlag = bRestoreAudioFrameFlag = true; //��Ϊ���ϵ����ţ���Ҫ���¼����Ѿ�������Ƶ����Ƶ֡���� 
	WriteLog(Log_Debug, "ReaplyFileSeek �϶����� ,nClient = %llu ,nTimestamp = %llu ,nRet = %d ", nClient, nTimestamp, nRet);
}

int CReadRecordFileInput::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)  
{

  return 0 ;	
}

int CReadRecordFileInput::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)  
{

  return 0 ;	
}

int CReadRecordFileInput::SendVideo() 
{

  return 0 ;	
}

int CReadRecordFileInput::SendAudio() 
{

  return 0 ;	
}

int CReadRecordFileInput::SendFirstRequst() 
{

  return 0 ;	
}

bool CReadRecordFileInput::RequestM3u8File() 
{
 
  return true ;	
}

 