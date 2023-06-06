#ifndef _NetGB28181RtpClient_H
#define _NetGB28181RtpClient_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;
#else

#endif


#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"


#define  MaxGB28181RtpSendVideoMediaBufferLength  1024*64 
#define  GB28181VideoStartTimestampFlag           0xEFEFEFEF   //视频开始时间戳 

//#define  WriteGB28181PSFileFlag   1 //保存ps 码流
//#define   WriteRecvPSDataFlag   1 //保存PS流标志

class CNetGB28181RtpClient : public CNetRevcBase
{
public:
	CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetGB28181RtpClient() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

#ifdef WriteRecvPSDataFlag
   FILE* fWritePSDataFile;
#endif

   void                     GetAACAudioInfo(unsigned char* nAudioData, int nLength);
   char                     m_recvMediaSource[512];
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pRecvMediaSource;//接入媒体源
#else
   std::shared_ptr<CMediaStreamSource> pRecvMediaSource;//接入媒体源
#endif
 
   ps_demuxer_t*           psBeiJingLaoChenDemuxer;
   bool                    RtpDepacket(unsigned char* pData, int nDataLength);
   uint32_t                hRtpHandle;             //国标rt解包

   void    GB28181PsToRtPacket(unsigned char* pPsData, int nLength);
   void    SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength);
   void    GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength);
 
   unsigned char*          netDataCache; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;
   int                     nRtpRtcpPacketType;//是否是RTP包，0 未知，2 rtp 包
   _rtp_header*            rtpHeadPtr;

   void                    CreateRtpHandle();
   int                     nMaxRtpSendVideoMediaBufferLength;//实际上累计视频，音频总量 ，如果国标发送视频时，可以为64K，如果只发送音频就设置640即可 
   unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //已经积累的长度  视频
   uint32_t                nStartVideoTimestamp; //上一帧视频初始时间戳 ，
   uint32_t                nCurrentVideoTimestamp;// 当前帧时间戳
   unsigned short          nVideoRtpLen;
   uint32_t                nSendRet;

   _ps_mux_init  init; 
   _ps_mux_input input;
   uint32_t      psMuxHandle;
   
   //北京老陈
   char*   s_buffer;
   int     nVideoStreamID , nAudioStreamID ;
   ps_muxer_t* psBeiJingLaoChen ;
   struct ps_muxer_func_t handler;
   uint64_t videoPTS , audioPTS  ;
   int      nflags;

   _rtp_packet_sessionopt  optionPS;
   _rtp_packet_input       inputPS;
   uint32_t                hRtpPS ;
   sockaddr_in             gbDstAddr;

#ifdef  WriteGB28181PSFileFlag
   FILE*   writePsFile;
#endif
};

#endif