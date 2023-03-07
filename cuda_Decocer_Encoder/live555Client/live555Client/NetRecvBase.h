#ifndef _NetRecvBase_H
#define _NetRecvBase_H

#include "MediaFifo.h"
#include "live555Client.h"

class CNetRevcBase
{
public:
   CNetRevcBase();
   ~CNetRevcBase() ;
   
   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) = 0;//接收网络数据
   virtual int ProcessNetData() = 0;//处理网络数据，比如进行解包、发送网络数据等等 

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength,char* szVideoCodec) = 0;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength,char* szAudioCodec,int nChannels,int SampleRate) = 0;//塞入音频数据

   virtual int SendVideo() = 0;//发送视频数据
   virtual int SendAudio() = 0;//发送音频数据

   virtual int   SendFirstRequst() = 0;//发送第一个请求
   virtual bool  RequestM3u8File() = 0 ;
   bool                             DecodeUrl(char *Src, char  *url, int  MaxLen);

   int64_t                          nSendOptionsCount;
   int                              m_nXHRtspURLType;

   int                              nRtspConnectStatus;
   char                             szCBErrorMessage[256];//回调错误信息
   bool                             bReConnectFlag;//是否需要重连 
   int                              nReconnctTimeCount;//重连次数
   void*                            m_pCustomerPtr;
   volatile LIVE555RTSP_AudioVideo  m_callbackFunc;

   char                    m_szShareMediaURL[512];//分享出去的地址，比如 /Media/Camera_00001  /live/test_00001 等等 
   int                     nVideoStampAdd;//视频时间戳增量
   int                     nAsyncAudioStamp;//同步的时间点

   volatile bool           bPushMediaSuccessFlag; //是否成功推流，成功推流了，才能从媒体库中删除
  
   volatile bool           bPushSPSPPSFrameFlag; //是否增加SPS、PPS帧

   //计算视频帧速度
   void                   CalcVideoFrameSpeed();

   bool                  ParseRtspRtmpHttpURL(char* szURL);

   //检测是否是I帧
   bool                CheckVideoIsIFrame(char* szVideoName, unsigned char* szPVideoData, int nPVideoLength);
   unsigned char       szVideoFrameHead[4];

   NETHANDLE   nServer;
   NETHANDLE   nClient;
   CMediaFifo  NetDataFifo;

   CMediaFifo           m_videoFifo; //存储视频缓存 
   CMediaFifo           m_audioFifo; //存储音频缓存 

   //媒体格式 
   MediaCodecInfo       mediaCodecInfo;

   volatile int         VideoFrameSpeed,TempVideoFrameSpeed;
   volatile double      nPushVideoFrameCount;//单位时间内加入视频帧总数 
   volatile double      nCalcFrameSpeedStartTime; //计算帧速度开始时间
   volatile double      nCalcFrameSpeedEndTime ;  //计算帧速度结束时间
   volatile int         nCalcFrameSpeedCount;     //已经计算视频帧速度次数 ，当视频帧速度稳定后，不再计算视频帧速度 

   //所有链接的网络类型 
   int                  netBaseNetType;

   //记录客户端的IP ，Port
   char                szClientIP[512]; //连接上来的客户端IP 
   unsigned short      nClientPort; //连接上来的客户端端口 

   //主动拉流
   RtspURLParseStruct   m_rtspStruct;

   volatile  int        nRecvDataTimerBySecond;//多少秒没有进行数据交换 

   addStreamProxyStruct m_addStreamProxyStruct;//请求代理拉流
   NETHANDLE            nMediaClient;//真正拉流的句柄

   time_t               nCreateDateTime;//创建时间

   vector<uint64_t>     vGB28181ClientVector;//存储国标28181收流的socket连接 

   int64_t              nProxyDisconnectTime;//代理断开时间 
   volatile bool        bRecordProxyDisconnectTimeFlag;//是否记录断裂时间

   int                  m_gbPayload;            //国标payload 
   uint32_t             hRtpHandle;             //国标rtp打包、或者解包
   uint32_t             psDeMuxHandle;          //ps 解包

   uint64_t             m_hParent;              //国标代理的句柄号
   _rtsp_header         rtspHead;
};

#endif