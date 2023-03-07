#ifndef _NetRecvBase_H
#define _NetRecvBase_H

#include "MediaFifo.h"
#include "live555Client.h"

class CNetRevcBase
{
public:
   CNetRevcBase();
   ~CNetRevcBase() ;
   
   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) = 0;//������������
   virtual int ProcessNetData() = 0;//�����������ݣ�������н���������������ݵȵ� 

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength,char* szVideoCodec) = 0;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength,char* szAudioCodec,int nChannels,int SampleRate) = 0;//������Ƶ����

   virtual int SendVideo() = 0;//������Ƶ����
   virtual int SendAudio() = 0;//������Ƶ����

   virtual int   SendFirstRequst() = 0;//���͵�һ������
   virtual bool  RequestM3u8File() = 0 ;
   bool                             DecodeUrl(char *Src, char  *url, int  MaxLen);

   int64_t                          nSendOptionsCount;
   int                              m_nXHRtspURLType;

   int                              nRtspConnectStatus;
   char                             szCBErrorMessage[256];//�ص�������Ϣ
   bool                             bReConnectFlag;//�Ƿ���Ҫ���� 
   int                              nReconnctTimeCount;//��������
   void*                            m_pCustomerPtr;
   volatile LIVE555RTSP_AudioVideo  m_callbackFunc;

   char                    m_szShareMediaURL[512];//�����ȥ�ĵ�ַ������ /Media/Camera_00001  /live/test_00001 �ȵ� 
   int                     nVideoStampAdd;//��Ƶʱ�������
   int                     nAsyncAudioStamp;//ͬ����ʱ���

   volatile bool           bPushMediaSuccessFlag; //�Ƿ�ɹ��������ɹ������ˣ����ܴ�ý�����ɾ��
  
   volatile bool           bPushSPSPPSFrameFlag; //�Ƿ�����SPS��PPS֡

   //������Ƶ֡�ٶ�
   void                   CalcVideoFrameSpeed();

   bool                  ParseRtspRtmpHttpURL(char* szURL);

   //����Ƿ���I֡
   bool                CheckVideoIsIFrame(char* szVideoName, unsigned char* szPVideoData, int nPVideoLength);
   unsigned char       szVideoFrameHead[4];

   NETHANDLE   nServer;
   NETHANDLE   nClient;
   CMediaFifo  NetDataFifo;

   CMediaFifo           m_videoFifo; //�洢��Ƶ���� 
   CMediaFifo           m_audioFifo; //�洢��Ƶ���� 

   //ý���ʽ 
   MediaCodecInfo       mediaCodecInfo;

   volatile int         VideoFrameSpeed,TempVideoFrameSpeed;
   volatile double      nPushVideoFrameCount;//��λʱ���ڼ�����Ƶ֡���� 
   volatile double      nCalcFrameSpeedStartTime; //����֡�ٶȿ�ʼʱ��
   volatile double      nCalcFrameSpeedEndTime ;  //����֡�ٶȽ���ʱ��
   volatile int         nCalcFrameSpeedCount;     //�Ѿ�������Ƶ֡�ٶȴ��� ������Ƶ֡�ٶ��ȶ��󣬲��ټ�����Ƶ֡�ٶ� 

   //�������ӵ��������� 
   int                  netBaseNetType;

   //��¼�ͻ��˵�IP ��Port
   char                szClientIP[512]; //���������Ŀͻ���IP 
   unsigned short      nClientPort; //���������Ŀͻ��˶˿� 

   //��������
   RtspURLParseStruct   m_rtspStruct;

   volatile  int        nRecvDataTimerBySecond;//������û�н������ݽ��� 

   addStreamProxyStruct m_addStreamProxyStruct;//�����������
   NETHANDLE            nMediaClient;//���������ľ��

   time_t               nCreateDateTime;//����ʱ��

   vector<uint64_t>     vGB28181ClientVector;//�洢����28181������socket���� 

   int64_t              nProxyDisconnectTime;//����Ͽ�ʱ�� 
   volatile bool        bRecordProxyDisconnectTimeFlag;//�Ƿ��¼����ʱ��

   int                  m_gbPayload;            //����payload 
   uint32_t             hRtpHandle;             //����rtp��������߽��
   uint32_t             psDeMuxHandle;          //ps ���

   uint64_t             m_hParent;              //�������ľ����
   _rtsp_header         rtspHead;
};

#endif