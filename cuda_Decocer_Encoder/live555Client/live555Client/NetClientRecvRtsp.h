#ifndef _NetClientRecvRtsp_H
#define _NetClientRecvRtsp_H

#include "DigestAuthentication.hh"

#include "rtp_depacket.h"
#include "RtcpPacket.h"

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

#define  MaxRtspValueCount          64 
#define  MaxRtspProtectCount        24 
#define   MaxRtpHandleCount         2

//#define  WriteRtpDepacketFileFlag   1 //是否写rtp解包文件
#define    PrintRtspLogFlag           1 //是否打印rtsp交换过程

#define    RtspServerRecvDataLength             1024*128      //用于切割缓存区的大小 
#define    MaxRtpSendVideoMediaBufferLength     1024*64      //用于拼接RTP包，准备发送 
#define    MaxRtpSendAudioMediaBufferLength     1024*8       //用于拼接RTP包，准备发送 
#define    VideoStartTimestampFlag              0xEFEFEFEF   //视频开始时间戳 

//HTTP头结构
struct HttpHeadStruct
{
	char szKey[128];
	char szValue[512];
};

struct RtspFieldValue
{
	char szKey[64];
	char szValue[384];
};

//rtsp 协议数据
struct RtspProtect
{
	char  szRtspCmdString[512];//  OPTIONS rtsp://190.15.240.36:554/Camera_00001.sdp RTSP/1.0
	char  szRtspCommand[64];//  rtsp命令名字 ,OPTIONS ANNOUNCE SETUP  RECORD   
	char  szRtspURL[512];// rtsp://190.15.240.36:554/Camera_00001.sdp

	RtspFieldValue rtspField[MaxRtspValueCount];
	char  szRtspContentSDP[1024]; //  媒体描述内容
	int   nRtspSDPLength;
};

class CNetClientRecvRtsp : public CNetRevcBase
{
public:
	CNetClientRecvRtsp(NETHANDLE hServer, NETHANDLE hClient2, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType);
   ~CNetClientRecvRtsp() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据

   virtual int SendVideo() ;//发送视频数据
   virtual int SendAudio() ;//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   bool         RtspPause();
   bool         RtspResume();
   bool         RtspSpeed(int nSpeed);
   bool         RtspSeek(int64_t nSeekTime);

   double                     dRange;
   _rtp_header                rtpHead;
   bool                       bPauseFlag;
   char                       szMediaCodecName[256];
   uint32_t                   psHandle; 
   volatile  bool             cbMediaCodecNameFlag;//是否回调媒体信息


   bool                       FindRtpPacketFlag();

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
   void  SendOptions(WWW_AuthenticateType wwwType,bool bRecordStatus);
   void  UserPasswordBase64(char* szUserPwdBase64);
   void  FindVideoAudioInSDP();

   unsigned char           s_extra_data[512];
   int                     extra_data_size;
   int                     sdp_h264_load(uint8_t* data, int bytes, const char* config);
#ifdef WriteRtpDepacketFileFlag
   bool                     bStartWriteFlag ;
#endif 

   int                     nSendRtpFailCount;//累计发送rtp包失败次数 

   bool                    GetSPSPPSFromDescribeSDP();
   bool                    m_bHaveSPSPPSFlag;
   char                    m_szSPSPPSBuffer[512];
   char                    m_pSpsPPSBuffer[512];
   unsigned int            m_nSpsPPSLength;
   volatile bool           m_bCallBackSPSFlag;//是否回调SPS、PPS帧

   time_t                  nCurrentTime;
   int                     videoSSRC;
   int                     audioSSRC;
   bool                    bSendRRReportFlag;
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

   unsigned short          nVideoRtpLen, nAudioRtpLen;

   unsigned char            szRtpDataOverTCP[1500];
   unsigned char            szAudioRtpDataOverTCP[1500];

   uint32_t                hRtpVideo, hRtpAudio;
   int                     nVideoSSRC;

   char                    szRtspSDPContent[512];
   char                    szRtspAudioSDP[512];

   bool                    GetMediaInfoFromRtspSDP();
   void                    SplitterRtpAACData(unsigned char* rtpAAC, int nLength, uint32_t nTimestamp);
   int32_t                 XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain);
   bool                    ReadRtspEnd();

   int                     au_header_length;
   uint8_t                 *ptr, *pau, *pend;
   int                     au_size ; // only AU-size
   int                     au_numbers ;
   int                     SplitterSize[16];

   volatile bool           bRunFlag;

   std::mutex              netDataLock;
   unsigned char*          netDataCache; //网络数据缓存
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
   uint32_t       hRtpHandle[MaxRtpHandleCount];
   char           szVideoName[64];
   char           szAudioName[64];
   int            nVideoPayload;
   int            nAudioPayload;
   int            sample_index;//采样频率所对应的序号 
   int            nChannels; //音频通道数
   int            nSampleRate; //音频采样频率
   char           szMediaSourceURL[512];//媒体流地址，比如 /Media/Camera_00001 
   char           szRtspContentSDP[1024];
   char           szVideoSDP[512];
   char           szAudioSDP[512];
   CRtspABLSipParse   sipParseV, sipParseA;   //sdp 信息分析
  
   volatile bool  bIsInvalidConnectFlag; //是否为非法连接 
   volatile bool  bExitProcessFlagArray[3];

#ifdef WriteRtpDepacketFileFlag
   FILE*          fWriteRtpVideo;
   FILE*          fWriteRtpAudio;
#endif
};

#endif