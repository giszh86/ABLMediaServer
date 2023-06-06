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
#define  GB28181VideoStartTimestampFlag           0xEFEFEFEF   //��Ƶ��ʼʱ��� 

//#define  WriteGB28181PSFileFlag   1 //����ps ����
//#define   WriteRecvPSDataFlag   1 //����PS����־

class CNetGB28181RtpClient : public CNetRevcBase
{
public:
	CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetGB28181RtpClient() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

#ifdef WriteRecvPSDataFlag
   FILE* fWritePSDataFile;
#endif

   void                     GetAACAudioInfo(unsigned char* nAudioData, int nLength);
   char                     m_recvMediaSource[512];
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pRecvMediaSource;//����ý��Դ
#else
   std::shared_ptr<CMediaStreamSource> pRecvMediaSource;//����ý��Դ
#endif
 
   ps_demuxer_t*           psBeiJingLaoChenDemuxer;
   bool                    RtpDepacket(unsigned char* pData, int nDataLength);
   uint32_t                hRtpHandle;             //����rt���

   void    GB28181PsToRtPacket(unsigned char* pPsData, int nLength);
   void    SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength);
   void    GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength);
 
   unsigned char*          netDataCache; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;
   int                     nRtpRtcpPacketType;//�Ƿ���RTP����0 δ֪��2 rtp ��
   _rtp_header*            rtpHeadPtr;

   void                    CreateRtpHandle();
   int                     nMaxRtpSendVideoMediaBufferLength;//ʵ�����ۼ���Ƶ����Ƶ���� ��������귢����Ƶʱ������Ϊ64K�����ֻ������Ƶ������640���� 
   unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //�Ѿ����۵ĳ���  ��Ƶ
   uint32_t                nStartVideoTimestamp; //��һ֡��Ƶ��ʼʱ��� ��
   uint32_t                nCurrentVideoTimestamp;// ��ǰ֡ʱ���
   unsigned short          nVideoRtpLen;
   uint32_t                nSendRet;

   _ps_mux_init  init; 
   _ps_mux_input input;
   uint32_t      psMuxHandle;
   
   //�����ϳ�
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