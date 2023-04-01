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

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   void         ProcessRtcpData(unsigned char* szRtcpData, int nDataLength, int nChan);
   void         GetAACAudioInfo(unsigned char* nAudioData, int nLength);

#ifdef  WriteRtpFileFlag
   FILE*                  fWriteRtpFile;
#endif
   _rtp_header*            rtpHeadPtr;
   ps_demuxer_t*           psBeiJingLaoChen;

   int                     nRtpRtcpPacketType;//�Ƿ���RTP����0 δ֪��2 rtp ��
   unsigned char           szRtcpDataOverTCP[1024];
   sockaddr_in*            pRtpAddress;
   int64_t                 nSendRtcpTime;
   CRtcpPacketRR           rtcpRR;//�����߱���
   unsigned char           szRtcpSRBuffer[512];
   unsigned int            rtcpSRBufferLength;

   //rtp ���
   bool   RtpDepacket(unsigned char* pData, int nDataLength);
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   volatile  bool bInitFifoFlag;

   std::mutex              netDataLock;
   unsigned char*          netDataCache; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;

   FILE*  fWritePsFile;
   FILE*  pWriteRtpFile;
};

#endif