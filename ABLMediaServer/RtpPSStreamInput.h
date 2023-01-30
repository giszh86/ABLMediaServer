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

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   void                                  GetAACAudioInfo(unsigned char* nAudioData, int nLength);
   std::mutex                            psRecvLock;
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
   _rtp_header                           rtpHead;

   int                    m_gbPayload;            //国标payload 
   uint32_t               hRtpHandle;             //国标rtp打包、或者解包

};

#endif