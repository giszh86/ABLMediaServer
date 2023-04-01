#ifndef _NetClientRecvRtsp_H
#define _NetClientRecvRtsp_H

#include "./rtspMD5/DigestAuthentication.hh"

#include "MediaStreamSource.h"
#include "rtp_packet.h"
#include "RtcpPacket.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#else

#endif


#include "mpeg4-avc.h"
#include "rtp-payload.h"
#include "rtp-profile.h"

#define  MaxRtspValueCount          64 
#define  MaxRtspProtectCount        24 
#define   MaxRtpHandleCount         2

//url类型，是实况，还是录像 ,还是下载
enum XHRtspURLType
{
	XHRtspURLType_Liveing = 0,  //实况播放
	XHRtspURLType_RecordPlay = 1,   //录像播放
	XHRtspURLType_RecordDownload = 2    //录像下载
};

//#define    WriteHIKPsPacketData       1 //是否写海康PS流

#define    RtspServerRecvDataLength             1024*32      //用于切割缓存区的大小 
#define    MaxRtpSendVideoMediaBufferLength     1024*64      //用于拼接RTP包，准备发送 
#define    MaxRtpSendAudioMediaBufferLength     1024*8       //用于拼接RTP包，准备发送 
#define    VideoStartTimestampFlag              0xEFEFEFEF   //视频开始时间戳 

class CNetClientRecvRtsp : public CNetRevcBase
{
public:
	CNetClientRecvRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientRecvRtsp() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据

   virtual int SendVideo() ;//发送视频数据
   virtual int SendAudio() ;//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   uint32_t     cbVideoTimestamp;//回调时间戳
   uint32_t     cbVideoLength;//回调视频累计
   bool         RtspPause();
   bool         RtspResume();
   bool         RtspSpeed(char* nSpeed);
   bool         RtspSeek(char* szSeekTime);
   WWW_AuthenticateType       m_wwwType;
   unsigned char              szCallBackAudio[2048];
   unsigned char*             szCallBackVideo;
   int                        m_nXHRtspURLType;

   _rtp_header                rtpHead;

   bool                       FindRtpPacketFlag();
   bool                       StartRtpPsDemux();

#ifdef           WriteHIKPsPacketData
   FILE*          fWritePS;
#endif

   uint32_t                   psHandle; 

   int                        nRtspProcessStep;
   int                        nTrackIDOrer;
   int                        CSeq;
   unsigned int               nMediaCount;
   unsigned int               nSendSetupCount;
   char                       szWww_authenticate[384];//摘要认证参数，由服务器发送过来的
   WWW_AuthenticateType       AuthenticateType;//rtsp是什么类型验证
   char                       szBasic[512];//用于rtsp基础验证
   char                       szSessionID[512];//sessionID 
   char                       szTrackIDArray[16][512];

   bool  GetWWW_Authenticate();
   bool  getRealmAndNonce(char* szDigestString, char* szRealm, char* szNonce);
   void  SendPlay(WWW_AuthenticateType wwwType);
   void  SendSetup(WWW_AuthenticateType wwwType);
   void  SendDescribe(WWW_AuthenticateType wwwType);
   void  SendOptions(WWW_AuthenticateType wwwType);
   void  UserPasswordBase64(char* szUserPwdBase64);
   void  FindVideoAudioInSDP();

   unsigned char           s_extra_data[512];
   int                     extra_data_size;
   struct mpeg4_avc_t      avc;
   int                     sdp_h264_load(uint8_t* data, int bytes, const char* config);
   bool                     bStartWriteFlag ;

   int                     nSendRtpFailCount;//累计发送rtp包失败次数 

   bool                    GetSPSPPSFromDescribeSDP();
   bool                    m_bHaveSPSPPSFlag;
   char                    m_szSPSPPSBuffer[512];
   char                    m_pSpsPPSBuffer[512];
   unsigned int            m_nSpsPPSLength;

   int64_t                 nCurrentTime;
   int                     videoSSRC;
   bool                    bSendRRReportFlag;
   int                     audioSSRC;
   CRtcpPacketSR           rtcpSR;
   CRtcpPacketRR           rtcpRR;
   unsigned char           szRtcpSRBuffer[512];
   unsigned int            rtcpSRBufferLength;
   unsigned char           szRtcpDataOverTCP[1500];
   void                    SendRtcpReportData();//发送rtcp 报告包,发送端
   void                    SendRtcpReportDataRR(unsigned int nSSRC, int nChan);//发送rtcp 报告包,接收端
   void                    ProcessRtcpData(char* szRtpData, int nDataLength, int nChan);

   int                     GetRtspPathCount(char* szRtspURL);//统计rtsp URL 路径数量

   volatile                uint64_t tRtspProcessStartTime; //开始时间

   std::mutex              MediaSumRtpMutex;
   unsigned short          nVideoRtpLen, nAudioRtpLen;

   unsigned char            szRtpDataOverTCP[1500];
   unsigned char            szAudioRtpDataOverTCP[1500];

   uint32_t                hRtpVideo, hRtpAudio;
   int                     nVideoSSRC;

   char                    szRtspSDPContent[512];
   char                    szRtspAudioSDP[512];

   bool                    GetMediaInfoFromRtspSDP();
   void                    SplitterRtpAACData(unsigned char* rtpAAC, int nLength);
   int32_t                 XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain);
   bool                    ReadRtspEnd();

   int                     au_header_length;
   uint8_t                 *ptr, *pau, *pend;
   int                     au_size ; // only AU-size
   int                     au_numbers ;
   int                     SplitterSize[16];

   std::mutex              netDataLock;
   unsigned char           netDataCache[MaxNetDataCacheBufferLength]; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   int                     MaxNetDataCacheCount;

   unsigned char           data_[RtspServerRecvDataLength];//每一帧rtsp数据，包括rtsp 、 rtp 包 
   unsigned int            data_Length;
   unsigned short          nRtpLength;
   int                     nContentLength;

   RtspProtect      RtspProtectArray[MaxRtspProtectCount];
   int              RtspProtectArrayOrder;
 
   int             FindHttpHeadEndFlag();
   int             FindKeyValueFlag(char* szData);
   void            GetHttpModemHttpURL(char* szMedomHttpURL);
   int             FillHttpHeadToStruct();
   bool            GetFieldValue(char* szFieldName, char* szFieldValue);

   bool            bReadHeadCompleteFlag; //是否读取完毕HTTP头
   int             nRecvLength;           //已经读取完毕的长度
   unsigned char   szHttpHeadEndFlag[8];  //Http头结束标志
   int             nHttpHeadEndLength;    //Http头结束标志点的长度 
   char            szResponseHttpHead[512];
   char            szCSeq[128];
   char            szTransport[256];

   char            szResponseBuffer[2048];
   int             nSendRet;
  static   uint64_t Session ;
  uint64_t         currentSession;
  char             szCurRtspURL[512];
  int64_t           nPrintCount;

   //只处理rtsp命令，比如 OPTIONS,DESCRIBE,SETUP,PALY 
   void            InputRtspData(unsigned char* pRecvData, int nDataLength);

   void           AddADTSHeadToAAC(unsigned char* szData, int nAACLength);
   unsigned char  aacData[2048];
   int            timeValue;
   struct rtp_payload_t   hRtpHandle[MaxRtpHandleCount];
   void*                  rtpDecoder[MaxRtpHandleCount];
   char           szSdpAudioName[64];
   char           szVideoName[64];
   char           szAudioName[64];
   int            nVideoPayload;
   int            nAudioPayload;
   int            sample_index;//采样频率所对应的序号 
   int            nChannels; //音频通道数
   int            nSampleRate; //音频采样频率
   char           szRtspContentSDP[1024];
   char           szVideoSDP[512];
   char           szAudioSDP[512];
   CABLSipParse   sipParseV, sipParseA;   //sdp 信息分析
#ifdef USE_BOOST
  boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
 


   volatile bool  bIsInvalidConnectFlag; //是否为非法连接 
   volatile bool  bExitProcessFlagArray[3];

   FILE*          fWriteRtpVideo;
   FILE*          fWriteRtpAudio;
   FILE*          fWriteESStream;
};

#endif