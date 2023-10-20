#ifndef _NetRecvBase_H
#define _NetRecvBase_H

#include "MediaFifo.h"
#include "MediaStreamSource.h"
#include "AACEncode.h"

#define CalcMaxVideoFrameSpeed         25  //������Ƶ֡�ٶȴ���
#define Send_ImageFile_MaxPacketCount  1024*32

class CNetRevcBase
{
public:
   CNetRevcBase();
   ~CNetRevcBase() ;
   
   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength,void* address) = 0;//������������
   virtual int ProcessNetData() = 0;//�����������ݣ�������н���������������ݵȵ� 

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength,char* szVideoCodec) = 0;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength,char* szAudioCodec,int nChannels,int SampleRate) = 0;//������Ƶ����

   virtual int SendVideo() = 0;//������Ƶ����
   virtual int SendAudio() = 0;//������Ƶ����

   virtual int   SendFirstRequst() = 0;//���͵�һ������
   virtual bool  RequestM3u8File() = 0 ;

   WebRtcCallStruct       webRtcCallStruct;

   void                   GetAACAudioInfo(unsigned char* nAudioData, int nLength);

   bool                   InsertUUIDtoJson(char* szSrcJSON, char* szUUID);
   char                   szTemp2[string_length_512];
   char                   request_uuid[string_length_256];

   int                    m_nXHRtspURLType;
   char*                  getAACConfig(int nChanels, int nSampleRate);
   char                   szConfigStr[64];

   char                   szPlayParams[string_length_1024];//����url�еĲ������������ź�����ַ���
   volatile  bool         bOn_playFlag;//����֪ͨ�Ƿ��͹�
   ABLRtspPlayerType      m_rtspPlayerType;//rtsp �������� 
   H265ConvertH264Struct  m_h265ConvertH264Struct;
   uint64_t               key;//ý��Դ�����ľ����
    uint64_t              nCurrentVideoFrames;//��ǰ��Ƶ֡��
   uint64_t               nTotalVideoFrames;//¼����Ƶ��֡��
   std::mutex             businessProcMutex;//ҵ������
   int                    nTcp_Switch;
   volatile   bool        bSendFirstIDRFrameFlag;
   unsigned long          nSSRC;//rtp����õ���ssrc 
   unsigned   char        psHeadFlag[4];//PSͷ�ֽ�

   int                    nRtspProcessStep;
   volatile bool          m_bSendMediaWaitForIFrame;//����ý�����Ƿ�ȴ���I֡ 
   volatile uint32_t      m_bWaitIFrameCount;//�ȴ�I֡��֡��
   volatile bool          m_bIsRtspRecordURL;//��������ʱ��¼��طŵ�url 

   volatile bool          m_bPauseFlag;//��¼��ط�ʱ���Ƿ���ͣ״̬
   int                    m_nScale; 
   bool                   ConvertDemainToIPAddress();
   char                   domainName[256]; //����
   bool                   ifConvertFlag;//�Ƿ���Ҫת��
   int64_t                tUpdateIPTime;//��̬��������IPʱ�� 

   char                   szContentType[512];
   getSnapStruct          m_getSnapStruct;
   int                    timeout_sec;
   volatile  bool         bSnapSuccessFlag ;//�Ƿ�ץ�ĳɹ���

   bool                   bConnectSuccessFlag;
   char                   app[string_length_256];
   char                   stream[string_length_512];
   char                   szJson[string_length_4096] ;  //���ɵ�json

   _rtp_header             rtpHeader;
   _rtp_header*            rtpHeaderPtr;
   _rtp_header*            rtpHeaderXHB;
   unsigned short          rtpExDataLength;
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource>   CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient);
#else
   std::shared_ptr<CMediaStreamSource>   CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient);
#endif
  
   bool                    QueryRecordFileIsExiting(char* szReplayRecordFileURL);

   char                    szRecordPath[string_length_2048];//¼�񱣴��·�� D:\video\Media\Camera_000001
   char                    szPicturePath[string_length_2048];//ͼƬ�����·�� D:\video\Media\Camera_000001

   volatile  uint64_t      nReConnectingCount;//�������� 
   volatile  bool          bProxySuccessFlag;//���ִ����Ƿ�ɹ�

   int                     CalcVideoFrameSpeed(unsigned char* pRtpData,int nLength);//������Ƶ֡�ٶ�
   int                     CalcFlvVideoFrameSpeed(int nVideoPTS,int nMaxValue);
   uint32_t                oldVideoTimestamp;//��һ����Ƶʱ���
   _rtp_header             rtp_header;//��Ƶʱ��� 
   int                     nVideoFrameSpeedOrder;//��Ƶ֡�ٶ���ţ�Ҫ������ٶȲ���ƽ�ȵ�֡�ٶ�
   volatile bool           bUpdateVideoFrameSpeedFlag;//�Ƿ������Ƶ֡�ٶ� 

   int64_t                 nPrintTime;
   void                    SyncVideoAudioTimestamp(); //ͬ������Ƶ����� rtmp ,flv ,ws-flv 
   int32_t                 flvPS, flvAACDts;
   int32_t                 nNewAddAudioTimeStamp;
   int                     nUseNewAddAudioTimeStamp;//ʹ���µ���Ƶʱ�������
   bool                    bUserNewAudioTimeStamp;

   char                    m_szShareMediaURL[string_length_2048];//�����ȥ�ĵ�ַ������ /Media/Camera_00001  /live/test_00001 �ȵ� 
   int                     nVideoStampAdd;//��Ƶʱ�������
   int                     nAsyncAudioStamp;//ͬ����ʱ���

   volatile bool           bPushMediaSuccessFlag; //�Ƿ�ɹ��������ɹ������ˣ����ܴ�ý�����ɾ��
  
   volatile bool           bPushSPSPPSFrameFlag; //�Ƿ�����SPS��PPS֡

   bool                  ParseRtspRtmpHttpURL(char* szURL);

   //����Ƿ���I֡
   bool                CheckVideoIsIFrame(char* szVideoName, unsigned char* szPVideoData, int nPVideoLength);
   unsigned char       szVideoFrameHead[4];

   NETHANDLE   nServer;
   NETHANDLE   nClient;
   NETHANDLE   nClientRtcp; //���ڹ���udp��ʽ��������ʱ����¼rtcp 
   CMediaFifo  NetDataFifo;

   CMediaFifo           m_videoFifo; //�洢��Ƶ���� 
   CMediaFifo           m_audioFifo; //�洢��Ƶ���� 

   queryPictureListStruct m_queryPictureListStruct;
   queryRecordListStruct  m_queryRecordListStruct;
   startStopRecordStruct  m_startStopRecordStruct;
   controlStreamProxy     m_controlStreamProxy;
   SetConfigParamValue    m_setConfigParamValue;
   ListServerPortStruct   m_listServerPortStruct;

   //ý���ʽ 
   MediaCodecInfo       mediaCodecInfo;

   //�������ӵ��������� 
   int                  netBaseNetType;

   //��¼�ͻ��˵�IP ��Port
   char                szClientIP[512]; //���������Ŀͻ���IP 
   unsigned short      nClientPort; //���������Ŀͻ��˶˿� 

   //��������
   RtspURLParseStruct   m_rtspStruct;

   volatile  int        nRecvDataTimerBySecond;//������û�н������ݽ��� 

   addStreamProxyStruct m_addStreamProxyStruct;//�����������
   delRequestStruct     m_delRequestStruct ;//ɾ����������
   NETHANDLE            nMediaClient;//���������ľ��
   NETHANDLE            nMediaClient2;//���������ľ��

   addPushProxyStruct   m_addPushProxyStruct ; //�����������
 
   openRtpServerStruct  m_openRtpServerStruct;//����gb28181 ���� rtp �����⹹
   startSendRtpStruct   m_startSendRtpStruct; //����gb28181 ���� rtp �����ṹ
 
   int64_t              nCreateDateTime;//����ʱ��

   int                  nNetGB28181ProxyType;//����������� 
   vector<uint64_t>     vGB28181ClientVector;//�洢����28181������socket���� 

   int64_t              nProxyDisconnectTime;//����Ͽ�ʱ�� 
   volatile bool        bRecordProxyDisconnectTimeFlag;//�Ƿ��¼����ʱ��

   getMediaListStruct     m_getMediaListStruct;//��ȡý��Դ�б� 
   getOutListStruct       m_getOutListStruct;  //��ȡ���ⷢ�͵��б�
   getServerConfigStruct  m_getServerConfigStruct;//��ȡϵͳ����

   int                    m_gbPayload;            //����payload 
   uint32_t               hRtpHandle;             //����rtp��������߽��
   uint32_t               psDeMuxHandle;          //ps ���

   uint64_t               hParent;//�������ľ����
   volatile bool          bRunFlag;
   char                   szMediaSourceURL[string_length_1024];//ý������ַ������ /Media/Camera_00001 

   bool                   SplitterAppStream(char* szMediaSoureFile);
   
   bool                   DecodeUrl(char *Src, char  *url, int  MaxLen) ;
   bool                   ResponseHttp(uint64_t nHttpClient,char* szSuccessInfo,bool bClose);
   bool                   ResponseHttp2(uint64_t nHttpClient, char* szSuccessInfo, bool bClose);
   bool                   ResponseImage(uint64_t nHttpClient, HttpImageType imageType,unsigned char* pImageBuffer,int nImageLength, bool bClose);
   std::mutex             httpResponseLock;
   char                   szResponseBody[1024 * 512];
   char                   szResponseHttpHead[1024 * 128];
   char                   szRtspURLTemp[string_length_2048];
   uint64_t               nClient_http ; //http �������� 
   bool                   bResponseHttpFlag;

   closeStreamsStruct     m_closeStreamsStruct;

   unsigned short         nReturnPort;//gb28181 ���صĶ˿�

   _rtsp_header           rtspHead;

   int                    nGB28181ConnectCount;//����TCP��������ʱ�������Ӵ���
   char                   szRequestReplayRecordFile[string_length_1024];//���󲥷��ļ�
   char                   szSplliterShareURL[string_length_1024];//¼��㲥ʱ�и��url 
   char                   szReplayRecordFile[string_length_1024];//¼��㲥�и��¼���ļ����� 
   char                   szSplliterApp[string_length_256];
   char                   szSplliterStream[string_length_512];
   uint64_t               nReplayClient; //¼��ط�ID

   int                    nVideoFrameSpeedArray[CalcMaxVideoFrameSpeed];//��Ƶ֡�ٶ�����
   int                    nCalcVideoFrameCount; //�������
   int                    m_nVideoFrameSpeed;

   volatile   uint32_t   nReadVideoFrameCount;//��ȡ����Ƶ֡���� ������8��16���ٵ���ʱ 
   volatile   uint32_t   nReadAudioFrameCount;//��ȡ����ƵƵ֡���� ��
   MediaSourceType       nMediaSourceType;//ý��Դ���ͣ�ʵ�����ţ�¼��㲥
   uint64_t              duration;//¼��ط�ʱ��ȡ��¼���ļ�����

   int                   nRtspRtpPayloadType;//rtp���ط�ʽ 0 δ֪ ��1 ES��2 PS ,����rtsp��������ʱʹ�ã�
   char                  szReponseTemp[string_length_1024];

   bool                    m_bHaveSPSPPSFlag;
   char                    m_szSPSPPSBuffer[512];
   char                    m_pSpsPPSBuffer[512];
   unsigned int            m_nSpsPPSLength;
   int                     sdp_h264_load(uint8_t* data, int bytes, const char* config);
   int                     GetSubFromString(char* szString, char* szStringFlag1, char* szStringFlag2, char* szOutString);
   bool                    GetH265VPSSPSPPS(char* szSDPString, int  nVideoPayload);
};
#endif