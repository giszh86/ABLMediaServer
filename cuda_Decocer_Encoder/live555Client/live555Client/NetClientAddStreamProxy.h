#ifndef _NetClientAddStreamProxy_H
#define _NetClientAddStreamProxy_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

#include "live555Client.h"

using namespace boost;

class CNetClientAddStreamProxy : public CNetRevcBase
{
public:
	CNetClientAddStreamProxy(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, uint64_t hParent, int nXHRtspURLType);
   ~CNetClientAddStreamProxy() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

};

#endif