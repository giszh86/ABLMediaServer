#ifndef _NetGB28181RtpClient_H
#define _NetGB28181RtpClient_H

//#include <boost/unordered/unordered_map.hpp>
//#include <boost/smart_ptr/shared_ptr.hpp>
//#include <boost/unordered/unordered_map.hpp>
//#include <boost/make_shared.hpp>
//#include <boost/algorithm/string.hpp>
//
//using namespace boost;

#define  MaxGB28181RtpSendVideoMediaBufferLength  1024*64 
#define  GB28181VideoStartTimestampFlag           0xEFEFEFEF   //��Ƶ��ʼʱ��� 

//#define  WriteGB28181PSFileFlag   1 //����ps ����

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

   void    GB28181PsToRtPacket(unsigned char* pPsData, int nLength);
   void    SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength);
   void    GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength);

   unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //�Ѿ����۵ĳ���  ��Ƶ
   uint32_t                nStartVideoTimestamp; //��һ֡��Ƶ��ʼʱ��� ��
   uint32_t                nCurrentVideoTimestamp;// ��ǰ֡ʱ���
   unsigned short          nVideoRtpLen;
   uint32_t                nSendRet;

   _ps_mux_init  init; 
   _ps_mux_input input;
   uint32_t      psMuxHandle;

   _rtp_packet_sessionopt  optionPS;
   _rtp_packet_input       inputPS;
   uint32_t                hRtpPS ;
   sockaddr_in             gbDstAddr;

#ifdef  WriteGB28181PSFileFlag
   FILE*   writePsFile;
#endif
};

#endif