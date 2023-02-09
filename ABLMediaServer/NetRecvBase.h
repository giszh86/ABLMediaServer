#ifndef _NetRecvBase_H
#define _NetRecvBase_H

#include "MediaFifo.h"
#include "H265_SPS_Nal.h"
#include "Unit_ReadSPS.h"
#include "MediaStreamSource.h"
#include "AACEncode.h"

#define CalcMaxVideoFrameSpeed         20  //计算视频帧速度次数
#define Send_ImageFile_MaxPacketCount  1024*32

class CNetRevcBase
{
public:
   CNetRevcBase();
   ~CNetRevcBase() ;
   
   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength,void* address) = 0;//接收网络数据
   virtual int ProcessNetData() = 0;//处理网络数据，比如进行解包、发送网络数据等等 

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength,char* szVideoCodec) = 0;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength,char* szAudioCodec,int nChannels,int SampleRate) = 0;//塞入音频数据

   virtual int SendVideo() = 0;//发送视频数据
   virtual int SendAudio() = 0;//发送音频数据

   virtual int   SendFirstRequst() = 0;//发送第一个请求
   virtual bool  RequestM3u8File() = 0 ;

   ABLRtspPlayerType      m_rtspPlayerType;//rtsp 播放类型 
   H265ConvertH264Struct  m_h265ConvertH264Struct;
   uint64_t               key;//媒体源关联的句柄号
    uint64_t              nCurrentVideoFrames;//当前视频帧数
   uint64_t               nTotalVideoFrames;//录像视频总帧数
   std::mutex             businessProcMutex;//业务处理锁
   int                    nTcp_Switch;
   volatile   bool        bSendFirstIDRFrameFlag;
   unsigned long          nSSRC;//rtp解包得到的ssrc 
   unsigned   char        psHeadFlag[4];//PS头字节

   int                    nRtspProcessStep;
   volatile bool          m_bSendMediaWaitForIFrame;//发送媒体流是否等待到I帧 
   volatile bool          m_bIsRtspRecordURL;//代理拉流时是录像回放的url 

   volatile bool          m_bPauseFlag;//在录像回放时，是否暂停状态
   int                    m_nScale; 
   bool                   ConvertDemainToIPAddress();
   char                   domainName[256]; //域名
   bool                   ifConvertFlag;//是否需要转换
   int64_t                tUpdateIPTime;//动态域名更新IP时间 

   char                   szContentType[512];
   getSnapStruct          m_getSnapStruct;
   int                    timeout_sec;
   volatile  bool         bSnapSuccessFlag ;//是否抓拍成功过

   bool                   bConnectSuccessFlag;
   char                   app[512];
   char                   stream[512];
   char                   szJson[2048] ;  //生成的json

   CAACEncode             aacEnc;
   char                   g711ToPCMCache[1024 * 16];
   int                    nG711ToPCMCacheLength;
   int                    nAACEncodeLength;
   int                    nRetunEncodeLength;
   char                   g711toPCM[1024];
   char                   g711toPCMResample[2048];

   _rtp_header             rtpHeader;
   _rtp_header*            rtpHeaderPtr;

#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource>   CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient);
#else
   std::shared_ptr<CMediaStreamSource>   CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient);
#endif


   bool                    QueryRecordFileIsExiting(char* szReplayRecordFileURL);

   char                    szRecordPath[1024];//录像保存的路径 D:\video\Media\Camera_000001
   char                    szPicturePath[1024];//图片保存的路径 D:\video\Media\Camera_000001

   volatile  uint64_t      nReConnectingCount;//重连次数 
   volatile  bool          bProxySuccessFlag;//各种代理是否成功

   unsigned  int           FindSpsPosition(unsigned char* szVideoBuffer, int nBufferLength, bool &bFind);
   bool                    ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params);
   bool                    GetWidthHeightFromSPS(unsigned char* szSPS, int nSPSLength, int& nWidth, int& nHeight);
   CSPSReader              _spsreader;
   bs_t                    s;

   int                     CalcVideoFrameSpeed(unsigned char* pRtpData,int nLength);//计算视频帧速度
   int                     CalcFlvVideoFrameSpeed(int nVideoPTS,int nMaxValue);
   uint32_t                oldVideoTimestamp;//上一个视频时间戳
   _rtp_header             rtp_header;//视频时间戳 
   int                     nVideoFrameSpeedOrder;//视频帧速度序号，要后面的速度才是平稳的帧速度
   volatile bool           bUpdateVideoFrameSpeedFlag;//是否跟新视频帧速度 

   int64_t                 nPrintTime;
   void                    SyncVideoAudioTimestamp(); //同步音视频，针对 rtmp ,flv ,ws-flv 
   int32_t                 flvPS, flvAACDts;
   int32_t                 nNewAddAudioTimeStamp;
   int                     nUseNewAddAudioTimeStamp;//使用新的音频时间戳次数
   bool                    bUserNewAudioTimeStamp;

   char                    m_szShareMediaURL[1024];//分享出去的地址，比如 /Media/Camera_00001  /live/test_00001 等等 
   int                     nVideoStampAdd;//视频时间戳增量
   int                     nAsyncAudioStamp;//同步的时间点

   volatile bool           bPushMediaSuccessFlag; //是否成功推流，成功推流了，才能从媒体库中删除
  
   volatile bool           bPushSPSPPSFrameFlag; //是否增加SPS、PPS帧

   bool                  ParseRtspRtmpHttpURL(char* szURL);

   //检测是否是I帧
   bool                CheckVideoIsIFrame(char* szVideoName, unsigned char* szPVideoData, int nPVideoLength);
   unsigned char       szVideoFrameHead[4];

   NETHANDLE   nServer;
   NETHANDLE   nClient;
   NETHANDLE   nClientRtcp; //用在国标udp方式接收码流时，记录rtcp 
   CMediaFifo  NetDataFifo;

   CMediaFifo           m_videoFifo; //存储视频缓存 
   CMediaFifo           m_audioFifo; //存储音频缓存 

   queryPictureListStruct m_queryPictureListStruct;
   queryRecordListStruct  m_queryRecordListStruct;
   startStopRecordStruct  m_startStopRecordStruct;
   controlStreamProxy     m_controlStreamProxy;
   SetConfigParamValue    m_setConfigParamValue;

   //媒体格式 
   MediaCodecInfo       mediaCodecInfo;

   //所有链接的网络类型 
   int                  netBaseNetType;

   //记录客户端的IP ，Port
   char                szClientIP[512]; //连接上来的客户端IP 
   unsigned short      nClientPort; //连接上来的客户端端口 

   //主动拉流
   RtspURLParseStruct   m_rtspStruct;

   volatile  int        nRecvDataTimerBySecond;//多少秒没有进行数据交换 

   addStreamProxyStruct m_addStreamProxyStruct;//请求代理拉流
   delRequestStruct     m_delRequestStruct ;//删除代理拉流
   NETHANDLE            nMediaClient;//真正拉流的句柄
   NETHANDLE            nMediaClient2;//真正拉流的句柄

   addPushProxyStruct   m_addPushProxyStruct ; //请求代理推流
 
   openRtpServerStruct  m_openRtpServerStruct;//创建gb28181 接收 rtp 参数解构
   startSendRtpStruct   m_startSendRtpStruct; //创建gb28181 发送 rtp 参数结构
 
   int64_t              nCreateDateTime;//创建时间

   int                  nNetGB28181ProxyType;//国标代理类型 
   vector<uint64_t>     vGB28181ClientVector;//存储国标28181收流的socket连接 

   int64_t              nProxyDisconnectTime;//代理断开时间 
   volatile bool        bRecordProxyDisconnectTimeFlag;//是否记录断裂时间

   getMediaListStruct     m_getMediaListStruct;//获取媒体源列表 
   getOutListStruct       m_getOutListStruct;  //获取往外发送的列表
   getServerConfigStruct  m_getServerConfigStruct;//获取系统配置

   int                    m_gbPayload;            //国标payload 
   uint32_t               hRtpHandle;             //国标rtp打包、或者解包
   uint32_t               psDeMuxHandle;          //ps 解包

   uint64_t               hParent;//国标代理的句柄号
   volatile bool          bRunFlag;
   char                   szMediaSourceURL[512];//媒体流地址，比如 /Media/Camera_00001 

   bool                   SplitterAppStream(char* szMediaSoureFile);
   
   bool                   DecodeUrl(char *Src, char  *url, int  MaxLen) ;
   bool                   ResponseHttp(uint64_t nHttpClient,char* szSuccessInfo,bool bClose);
   bool                   ResponseHttp2(uint64_t nHttpClient, char* szSuccessInfo, bool bClose);
   bool                   ResponseImage(uint64_t nHttpClient, HttpImageType imageType,unsigned char* pImageBuffer,int nImageLength, bool bClose);
   std::mutex             httpResponseLock;
   char                   szResponseBody[1024 * 512];
   char                   szResponseHttpHead[1024 * 128];
   char                   szRtspURLTemp[1024];
   uint64_t               nClient_http ; //http 请求连接 
   bool                   bResponseHttpFlag;

   closeStreamsStruct     m_closeStreamsStruct;

   unsigned short         nReturnPort;//gb28181 返回的端口

   _rtsp_header           rtspHead;

   int                    nGB28181ConnectCount;//国标TCP接收码流时，被连接次数
   char                   szRequestReplayRecordFile[512];//请求播放文件
   char                   szSplliterShareURL[512];//录像点播时切割的url 
   char                   szReplayRecordFile[256];//录像点播切割的录像文件名字 
   char                   szSplliterApp[256];
   char                   szSplliterStream[256];
   uint64_t               nReplayClient; //录像回放ID

   int                    nVideoFrameSpeedArray[CalcMaxVideoFrameSpeed];//视频帧速度数组
   int                    nCalcVideoFrameCount; //计算次数
   int                    m_nVideoFrameSpeed;

   volatile   uint32_t   nReadVideoFrameCount;//读取到视频帧总数 ，用于8、16倍速的延时 
   volatile   uint32_t   nReadAudioFrameCount;//读取到音频频帧总数 ，
   MediaSourceType       nMediaSourceType;//媒体源类型，实况播放，录像点播
   uint64_t              duration;//录像回放时读取到录像文件长度

   int                   nRtspRtpPayloadType;//rtp负载方式 0 未知 ，1 ES，2 PS ,用在rtsp代理拉流时使用，
   char                  szReponseTemp[512];

};

#endif