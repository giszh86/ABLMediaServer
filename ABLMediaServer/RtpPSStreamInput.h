#ifndef _RtpPSStreamInput_H
#define _RtpPSStreamInput_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;

class CRtpPSStreamInput : public CNetRevcBase
{
public:
	CRtpPSStreamInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CRtpPSStreamInput() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   void                                  GetAACAudioInfo(unsigned char* nAudioData, int nLength);
   std::mutex                            psRecvLock;
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
   _rtp_header                           rtpHead;

   int                    m_gbPayload;            //����payload 
   uint32_t               hRtpHandle;             //����rtp��������߽��

};

#endif