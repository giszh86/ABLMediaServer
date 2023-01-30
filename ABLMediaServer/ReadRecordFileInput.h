#ifndef _ReadRecordFileInput_H
#define _ReadRecordFileInput_H

#include "mov-reader.h"
#include "mov-format.h"
#include "mpeg4-hevc.h"
#include "mpeg4-avc.h"
#include "mpeg4-aac.h"
#include "opus-head.h"
#include "webm-vpx.h"
#include "aom-av1.h"

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;

#define     ReadRecordFileInput_MaxPacketCount     1024*1024*3 

//��Ƶ����Ƶ
enum ABLAVType
{
	AVType_Video = 0, //��Ƶ
	AVType_Audio = 1, //��Ƶ 
};

class CReadRecordFileInput : public CNetRevcBase
{
public:
	CReadRecordFileInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
    ~CReadRecordFileInput() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   bool                 ReaplyFileSeek(uint64_t nTimestamp);
   bool                 GetMediaShareURLFromFileName(char* szRecordFileName, char* szMediaURL);
   bool                 UpdateReplaySpeed(double dScaleValue, ABLRtspPlayerType rtspPlayerType);
   bool                 UpdatePauseFlag(bool bFlag);

   uint64_t              nDownloadFrameCount;
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
   int                   nRetLength;
   std::mutex            readRecordFileInputLock;
   unsigned char         s_buffer[ReadRecordFileInput_MaxPacketCount];
   unsigned char         s_packet[ReadRecordFileInput_MaxPacketCount];
   unsigned char         audioBuffer[4096];
   struct mpeg4_avc_t    s_avc;
   struct mpeg4_hevc_t   s_hevc;
   struct mpeg4_aac_t    s_aac;

   uint32_t             s_aac_track ;
   uint32_t             s_avc_track;
   uint32_t             s_av1_track ;
   uint32_t             s_vpx_track;
   uint32_t             s_hevc_track;
   uint32_t             s_opus_track;
   uint32_t             s_mp3_track;
   uint32_t             s_subtitle_track;

   FILE*                 fp;
   mov_reader_t*         mov;
   int                   nReadRet;

   volatile  ABLAVType   nAVType;//��һ֡ý������ 
   uint64_t              mov_readerTime;//��ȡý��ʱ�̵�ʱ�� 
   uint64_t              nOldPTS;
   volatile   int        nVidepSpeedTime;//��Ƶ֡�ٶ� 
   volatile   double     dBaseSpeed ;
   volatile   double     m_dScaleValue;//��ǰ�ٶ�
   volatile   bool       m_bPauseFlag;
   volatile   uint64_t   m_nStartTimestamp;//��ʼ��ʱ���
   uint64_t              nVideoFirstPTS;
   uint64_t              nAudioFirstPTS;

   volatile   bool       bRestoreVideoFrameFlag;//�Ƿ���Ҫ�ָ���Ƶ֡����
   volatile   bool       bRestoreAudioFrameFlag;//�Ƿ���Ҫ�ָ���Ƶ֡����
};

#endif