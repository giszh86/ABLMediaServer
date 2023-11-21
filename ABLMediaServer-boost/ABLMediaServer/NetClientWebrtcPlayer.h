#ifndef _NetClientWebrtcPlayer_H
#define _NetClientWebrtcPlayer_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;

//#define  WebRtcVideoFileFlag     1 //д��webrtc��Ƶ����

class CNetClientWebrtcPlayer : public CNetRevcBase
{
public:
	CNetClientWebrtcPlayer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientWebrtcPlayer() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   boost::shared_ptr<CMediaStreamSource> pMediaSource;
   int                                   nSpsPositionPos;

#ifdef WebRtcVideoFileFlag
   FILE*     fWriteVideoFile;
   int64_t   nWriteFileCount;
   FILE*     fWriteFrameLengthFile;
#endif
};

#endif