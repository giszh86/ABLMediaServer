#ifndef _NetGB28181RtpServer_H
#define _NetGB28181RtpServer_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif


class CNetGB28181RtpServer : public CNetRevcBase
{
public:
	CNetGB28181RtpServer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetGB28181RtpServer() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   void         ProcessRtcpData(unsigned char* szRtcpData, int nDataLength, int nChan);
   void         GetAACAudioInfo(unsigned char* nAudioData, int nLength);

#ifdef  WriteRtpFileFlag
   FILE*                  fWriteRtpFile;
#endif
   _rtp_header*            rtpHeadPtr;
   ps_demuxer_t*           psBeiJingLaoChen;

   int                     nRtpRtcpPacketType;//是否是RTP包，0 未知，2 rtp 包
   unsigned char           szRtcpDataOverTCP[1024];
   sockaddr_in*            pRtpAddress;
   int64_t                 nSendRtcpTime;
   CRtcpPacketRR           rtcpRR;//接收者报告
   unsigned char           szRtcpSRBuffer[512];
   unsigned int            rtcpSRBufferLength;

   //rtp 解包
   bool   RtpDepacket(unsigned char* pData, int nDataLength);
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   volatile  bool bInitFifoFlag;

   std::mutex              netDataLock;
   unsigned char*          netDataCache; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;

   FILE*  fWritePsFile;
   FILE*  pWriteRtpFile;
};

#endif