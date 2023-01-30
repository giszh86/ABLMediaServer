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

//视频，音频
enum ABLAVType
{
	AVType_Video = 0, //视频
	AVType_Audio = 1, //音频 
};

class CReadRecordFileInput : public CNetRevcBase
{
public:
	CReadRecordFileInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
    ~CReadRecordFileInput() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

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

   volatile  ABLAVType   nAVType;//上一帧媒体类型 
   uint64_t              mov_readerTime;//读取媒体时刻的时间 
   uint64_t              nOldPTS;
   volatile   int        nVidepSpeedTime;//视频帧速度 
   volatile   double     dBaseSpeed ;
   volatile   double     m_dScaleValue;//当前速度
   volatile   bool       m_bPauseFlag;
   volatile   uint64_t   m_nStartTimestamp;//开始的时间戳
   uint64_t              nVideoFirstPTS;
   uint64_t              nAudioFirstPTS;

   volatile   bool       bRestoreVideoFrameFlag;//是否需要恢复视频帧总数
   volatile   bool       bRestoreAudioFrameFlag;//是否需要恢复音频帧总数
};

#endif