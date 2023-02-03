// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef  _Stdafx_H
#define  _Stdafx_H

//���嵱ǰ����ϵͳΪWindows 
#if (defined _WIN32 || defined _WIN64)
 #define      OS_System_Windows        1
#endif

#ifdef OS_System_Windows
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <WinSock2.h>
#include <Windows.h>
#include <objbase.h>  
#include <iphlpapi.h>

#include <thread>
#include <mutex>
#include <map>
#include <list>
#include <vector>
#include <string.h>
#include <malloc.h>

#include <d3d9.h>
#include <d3dx9tex.h>

#include "cudaCodecDLL.h"
#include "ABLString.h"
#include <cctype>
#else 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>

#include<sys/types.h> 
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h> 
#include <ifaddrs.h>
#include <netdb.h>

#include <pthread.h>
#include <signal.h>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <vector>
#include <math.h>
#include <iconv.h>
#include <malloc.h>
#include <dlfcn.h> 
#include "cudaCodecDLL_Linux.h"
#include "cudaEncodeDLL.h"

#include <limits.h>
#include <sys/resource.h>

#define      BYTE     unsigned char 

#endif

uint64_t GetCurrentSecond();
uint64_t GetCurrentSecondByTime(char* szDateTime);
bool     QureyRecordFileFromRecordSource(char* szShareURL, char* szFileName);

//rtsp ��������
enum ABLRtspPlayerType
{
	RtspPlayerType_Unknow = -1 ,  //δ֪ 
	RtspPlayerType_Liveing = 0 , //ʵʱ����
	RtspPlayerType_RecordReplay = 1 ,//¼��ط�
	RtspPlayerType_RecordDownload = 2,//¼������
};

//ý���ʽ��Ϣ
struct MediaCodecInfo
{
	char szVideoName[64]; //H264��H265 
	int  nVideoFrameRate; //��Ƶ֡�ٶ� 
	int  nWidth;          //��Ƶ��
	int  nHeight;         //��Ƶ��
	int  nVideoBitrate;   //��Ƶ����

	char szAudioName[64]; //AAC�� G711A�� G711U
	int  nChannels;       //ͨ������ 1 ��2 
	int  nSampleRate;     //����Ƶ�� 8000��16000��22050 ��32000�� 44100 �� 48000 
	int  nBaseAddAudioTimeStamp;//ÿ����Ƶ����
	int  nAudioBitrate;   //��Ƶ����

	MediaCodecInfo()
	{
		memset(szVideoName, 0x00, sizeof(szVideoName));
		nVideoFrameRate = 25 ;
		memset(szAudioName, 0x00, sizeof(szAudioName));
		nChannels = 0;
		nSampleRate = 0;
		nBaseAddAudioTimeStamp = 64;
		nWidth = 0;
		nHeight = 0;
		nVideoBitrate = 0;
		nAudioBitrate = 0;
	};
};

//ý����������ж˿ڽṹ
struct MediaServerPort
{
	char secret[256];  //api��������
	uint64_t nServerStartTime;//����������ʱ��
	bool     bNoticeStartEvent;//�Ƿ��Ѿ�֪ͨ���� 
	int  nHttpServerPort; //http����˿�
	int  nRtspPort;     //rtsp
	int  nRtmpPort;     //rtmp
	int  nHttpFlvPort;  //http-flv
	int  nWSFlvPort;    //ws-flv
	int  nHttpMp4Port;  //http-mp4
	int  ps_tsRecvPort; //���굥�˿�

	int  nHlsPort;     //Hls �˿� 
	int  nHlsEnable;   //HLS �Ƿ��� 
	int  nHLSCutType;  //HLS��Ƭ��ʽ  1 Ӳ�̣�2 �ڴ� 
	int  nH265CutType; //H265��Ƭ��ʽ 1  ��ƬΪTS ��2 ��ƬΪ mp4  
	int  hlsCutTime; //��Ƭʱ��
	int  nMaxTsFileCount;//�������TS��Ƭ�ļ�����
	char wwwPath[256];//hls��Ƭ·��

	int  nRecvThreadCount;//�����������ݽ����߳����� 
	int  nSendThreadCount;//�����������ݷ����߳�����
	int  nRecordReplayThread;//����¼��طţ���ȡ�ļ����߳�����
	int  nGBRtpTCPHeadType;  //GB28181 TCP ��ʽ����rtp(����PS)����ʱ����ͷ����ѡ��1�� 4���ֽڷ�ʽ��2��2���ֽڷ�ʽ��
	int  nEnableAudio;//�Ƿ�������Ƶ

	int  nIOContentNumber; //ioContent����
	int  nThreadCountOfIOContent;//ÿ��iocontent�ϴ������߳�����
	int  nReConnectingCount;//��������

	char recordPath[256];//¼�񱣴�·��
	int  pushEnable_mp4;//���������Ƿ���¼��
	int  fileSecond;//fmp4�и�ʱ��
	int  videoFileFormat;//¼���ļ���ʽ 1 Ϊ fmp4, 2 Ϊ mp4 
	int  fileKeepMaxTime;//¼���ļ������ʱ������λСʱ
	int  httpDownloadSpeed;//http¼�������ٶ��趨
	int  fileRepeat;//MP4�㲥(rtsp/rtmp/http-flv/ws-flv)�Ƿ�ѭ�������ļ�

	char picturePath[256];//ͼƬץ�ı���·��
	int  pictureMaxCount; //ÿ·ý��Դ���ץ�ı�������
	int  snapOutPictureWidth;//ץ�������
	int  snapOutPictureHeight;//ץ�������
	int  captureReplayType; //ץ�ķ�������
	int  snapObjectDestroy;//ץ�Ķ����Ƿ�����
	int  snapObjectDuration;//ץ�Ķ��������ʱ������λ��
	int  maxSameTimeSnap;//ץ����󲢷�����
	int  maxTimeNoOneWatch;//���˹ۿ����ʱ�� 
	int  nG711ConvertAAC; //�Ƿ�ת��ΪAAC 
	char ABL_szLocalIP[128];
	char mediaServerID[256];

	int  H265ConvertH264_enable;
	int  H265DecodeCpuGpuType;
	int  convertOutWidth;
	int	 convertOutHeight;
	int  convertMaxObject;
	int  convertOutBitrate;
	int  H264DecodeEncode_enable;
	int  filterVideo_enable;
	char filterVideoText[1280];
	int  nFilterFontSize;
	char  nFilterFontColor[64];
	float nFilterFontAlpha;//͸����
	int  nFilterFontLeft;//x����
	int  nFilterFontTop;//y����

	//�¼�֪ͨģ��
	int  hook_enable;//�Ƿ����¼�֪ͨ
	int  noneReaderDuration;//���˹ۿ�ʱ�䳤
	char on_stream_arrive[256];
	char on_stream_not_arrive[256]; //����δ���� ���������������������֧�� 
	char on_stream_none_reader[256];
	char on_stream_disconnect[256];
	char on_stream_not_found[256];
	char on_record_mp4[256];
	char on_record_progress[256];//¼�����
	char on_record_ts[256];
	char on_server_started[256];
	char on_server_keepalive[256];
	char on_delete_record_mp4[256];

	uint64_t    nClientNoneReader;
	uint64_t    nClientNotFound;
	uint64_t    nClientRecordMp4;
	uint64_t    nClientDeleteRecordMp4;
	uint64_t    nClientRecordProgress;
	uint64_t    nClientArrive;
	uint64_t    nClientNotArrive;
	uint64_t    nClientDisconnect;
	uint64_t    nClientRecordTS;
	uint64_t    nServerStarted;//����������
	uint64_t    nServerKeepalive;//������������Ϣ 
	int         MaxDiconnectTimeoutSecond;//�����߳�ʱ���
	int         ForceSendingIFrame;//ǿ�Ʒ���I֡ 
	uint64_t    nServerKeepaliveTime;//����������ʱ��

	char       debugPath[256];//�����ļ�
	int        nSaveProxyRtspRtp;//�Ƿ񱣴������������0 �����棬1 ����
	int        nSaveGB28181Rtp;//�Ƿ񱣴�GB28181���ݣ�0 δ���棬1 ���� 

	MediaServerPort()
	{
		memset(wwwPath, 0x00, sizeof(wwwPath));
		nServerStartTime = 0;
		bNoticeStartEvent = false;
		memset(on_server_started, 0x00, sizeof(on_server_started));
		memset(on_server_keepalive, 0x00, sizeof(on_server_keepalive));
		memset(on_delete_record_mp4, 0x00, sizeof(on_delete_record_mp4));
		memset(secret, 0x00, sizeof(secret));
		nRtspPort    = 554;
		nRtmpPort    = 1935;
		nHttpFlvPort = 8088;
		nHttpMp4Port = 8089;
		ps_tsRecvPort = 10000;

		nHlsPort     = 9088;
		nHlsEnable   = 0;
		nHLSCutType  = 1;
		nH265CutType = 1;
		hlsCutTime = 3;
		nMaxTsFileCount = 20;

		nRecvThreadCount = 64;
		nSendThreadCount = 64;
		nRecordReplayThread = 64;
		nGBRtpTCPHeadType = 1;
		nEnableAudio = 0 ;

		nIOContentNumber = 16;
		nThreadCountOfIOContent = 16;

		memset(recordPath, 0x00, sizeof(recordPath));
		fileSecond = 180;
		videoFileFormat = 1;
		pushEnable_mp4 = 0;
		fileKeepMaxTime = 12;
		httpDownloadSpeed = 6;
		fileRepeat = 0;
		maxTimeNoOneWatch = 1;
		nG711ConvertAAC = 0;
		memset(mediaServerID, 0x00, sizeof(mediaServerID));
		memset(picturePath, 0x00, sizeof(picturePath));
		pictureMaxCount = 10;
		captureReplayType = 1;
		snapObjectDestroy = 1;
		snapObjectDuration = 120;
		memset(ABL_szLocalIP, 0x00, sizeof(ABL_szLocalIP));

		hook_enable = 0;
		noneReaderDuration = 30;
		memset(on_stream_none_reader, 0x00, sizeof(on_stream_none_reader));
		memset(on_stream_not_found, 0x00, sizeof(on_stream_not_found));
		memset(on_record_mp4, 0x00, sizeof(on_record_mp4));
		memset(on_record_progress, 0x00, sizeof(on_record_progress));
		memset(on_stream_arrive, 0x00, sizeof(on_stream_arrive));
		memset(on_stream_not_arrive, 0x00, sizeof(on_stream_not_arrive));
 		memset(on_record_ts, 0x00, sizeof(on_record_ts));
		memset(on_stream_disconnect, 0x00, sizeof(on_stream_disconnect));
		nClientNoneReader = 0 ;
		nClientNotFound = 0;
		nClientRecordMp4 = 0;
		nClientDeleteRecordMp4 = 0;
		nClientRecordProgress = 0;
		nClientArrive = 0;
		nClientNotArrive = 0;
		nClientDisconnect = 0;
		nClientRecordTS = 0;
		nServerStarted = 0;
		nServerKeepalive = 0;

		maxSameTimeSnap = 16;
		snapOutPictureWidth; 
		snapOutPictureHeight; 

		H265ConvertH264_enable = 0;
		H265DecodeCpuGpuType = 0;
		convertOutWidth = 720;
		convertOutHeight = 576;
		convertMaxObject = 24;
		convertOutBitrate = 512;
		H264DecodeEncode_enable=0;
		filterVideo_enable=0;
		memset(filterVideoText,0x00,sizeof(filterVideoText));
		nFilterFontSize=12;
		memset(nFilterFontColor,0x00,sizeof(nFilterFontColor));
		nFilterFontAlpha = 0.6;//͸����
		nFilterFontLeft = 10;//x����
		nFilterFontTop = 10 ;//y����
		MaxDiconnectTimeoutSecond = 16;
		ForceSendingIFrame = 0;

		nSaveProxyRtspRtp = 0; 
		nSaveGB28181Rtp = 0 ; 
		memset(debugPath, 0x00, sizeof(debugPath));
 	}
};

//��Ե���ĳһ·��Ƶת��ṹ
struct H265ConvertH264Struct
{
	int  H265ConvertH264_enable;//H265�Ƿ�ת��
	int  H264DecodeEncode_enable;//h264�Ƿ����½��룬�ٱ��� 
 	int  convertOutWidth;
	int	 convertOutHeight;
 	int  convertOutBitrate;

	H265ConvertH264Struct()
	{
	  H265ConvertH264_enable = 0;
	  H264DecodeEncode_enable = 0;
 	  convertOutWidth = 0;
	  convertOutHeight = 0;
 	  convertOutBitrate = 512;
 	}
};

//�����������
enum NetBaseNetType
{
	NetBaseNetType_Unknown                 = 20 ,  //δ�������������
	NetBaseNetType_RtmpServerRecvPush      = 21,//RTMP �����������տͻ��˵����� 
	NetBaseNetType_RtmpServerSendPush      = 22,//RTMP ��������ת���ͻ��˵�������������
	NetBaseNetType_RtspServerRecvPush      = 23,//RTSP �����������տͻ��˵����� 
	NetBaseNetType_RtspServerSendPush      = 24,//RTSP ��������ת���ͻ��˵�������������
	NetBaseNetType_HttpFLVServerSendPush   = 25,//Http-FLV ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_HttpHLSServerSendPush   = 26,//Http-HLS ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_WsFLVServerSendPush     = 27,//WS-FLV ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_HttpMP4ServerSendPush   = 28,//http-mp4 ��������ת�� rtsp ��rtmp ��GB28181�ȵ�����������������mp4���ͳ�ȥ
	NetBaseNetType_WebRtcServerSendPush    = 29,//WebRtc ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������

	//������������
	NetBaseNetType_RtspClientRecv          = 30 ,//rtsp������������ 
	NetBaseNetType_RtmpClientRecv          = 31 ,//rtmp������������ 
	NetBaseNetType_HttpFlvClientRecv       = 32 ,//http-flv������������ 
	NetBaseNetType_HttpHLSClientRecv       = 33 ,//http-hls������������ 
	NetBaseNetType_HttClientRecvJTT1078    = 34, //���ս�ͨ��JTT1078 

	//������������
	NetBaseNetType_RtspClientPush          = 40,//rtsp������������ s
	NetBaseNetType_RtmpClientPush          = 41,//rtmp������������ 
 	NetBaseNetType_GB28181ClientPushTCP    = 42,//GB28181������������ 
	NetBaseNetType_GB28181ClientPushUDP    = 43,//GB28181������������ 

	NetBaseNetType_addStreamProxyControl   = 50,//���ƴ���rtsp\rtmp\flv\hsl ����
	NetBaseNetType_addPushProxyControl     = 51,//���ƴ���rtsp\rtmp  ���� ����

	NetBaseNetType_NetGB28181RtpServerListen      = 56,//����TCP��ʽ����Listen��
	NetBaseNetType_NetGB28181RtpServerUDP         = 60,//����28181 UDP��ʽ ��������
	NetBaseNetType_NetGB28181RtpServerTCP_Server  = 61,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181RtpServerTCP_Client  = 62,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181RtpServerRTCP        = 63,//����28181 UDP��ʽ �������� �е� rtcp ��
	NetBaseNetType_NetGB28181SendRtpUDP           = 65,//����28181 UDP��ʽ ��������
	NetBaseNetType_NetGB28181SendRtpTCP_Connect   = 66,//����28181 TCP��ʽ ��������,�������ӷ�ʽ ��������
	NetBaseNetType_NetGB28181SendRtpTCP_Server    = 67,//����28181 TCP��ʽ ��������,�������ӷ�ʽ ��������
	NetBaseNetType_NetGB28181RecvRtpPS_TS         = 68,//����28181 ���˿ڽ���PS��TS����
	NetBaseNetType_NetGB28181UDPTSStreamInput     = 69,//TS��������
	NetBaseNetType_NetGB28181UDPPSStreamInput     = 64,//PS����������굥�˿���������

	NetBaseNetType_RecordFile_FMP4                = 70,//¼��洢Ϊfmp4��ʽ
	NetBaseNetType_RecordFile_TS                  = 71,//¼��洢ΪTS��ʽ
	NetBaseNetType_RecordFile_PS                  = 72,//¼��洢ΪPS��ʽ
	NetBaseNetType_RecordFile_FLV                 = 73,//¼��洢Ϊflv��ʽ
	NetBaseNetType_RecordFile_MP4                 = 74,//¼��洢Ϊmp4��ʽ

 	ReadRecordFileInput_ReadFMP4File              = 80,//�Զ�ȡfmp4�ļ���ʽ 
	ReadRecordFileInput_ReadTSFile                = 81,//�Զ�ȡTS�ļ���ʽ 
	ReadRecordFileInput_ReadPSFile                = 82,//�Զ�ȡPS�ļ���ʽ 
	ReadRecordFileInput_ReadFLVFile               = 83,//�Զ�ȡFLV�ļ���ʽ 

	NetBaseNetType_HttpClient_None_reader           = 90,//���˹ۿ�
	NetBaseNetType_HttpClient_Not_found             = 91,//��û���ҵ�
	NetBaseNetType_HttpClient_Record_mp4            = 92,//���һ��¼��
	NetBaseNetType_HttpClient_on_stream_arrive      = 93,//��������
	NetBaseNetType_HttpClient_on_stream_disconnect  = 94,//�����ѶϿ�
	NetBaseNetType_HttpClient_on_record_ts          = 95,//TS��Ƭ���
	NetBaseNetType_HttpClient_on_stream_not_arrive  = 96,//����û�е���
	NetBaseNetType_HttpClient_Record_Progress       = 97,//¼�����ؽ���
 
	NetBaseNetType_SnapPicture_JPEG               =100,//ץ��ΪJPG 
	NetBaseNetType_SnapPicture_PNG                =101,//ץ��ΪPNG

	NetBaseNetType_NetServerHTTP                  =110,//http �������� 

	NetBaseNetType_HttpClient_ServerStarted       = 120,//����������
	NetBaseNetType_HttpClient_ServerKeepalive     = 121,//����������
	NetBaseNetType_HttpClient_DeleteRecordMp4     = 122,//����¼���ļ�

};

#define   MediaServerVerson                 "ABLMediaServer-6.3.5(2022-11-30)"
#define   RtspServerPublic                  "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD��GET_PARAMETER"
#define   RecordFileReplaySplitter          "__ReplayFMP4RecordFile__"  //ʵ����¼�����ֵı�־�ַ�������������ʵ����������url�С�

#define  MaxNetDataCacheBufferLength        1024*1024*3   //���绺�������ֽڴ�С
#define  MaxLiveingVideoFifoBufferLength    1024*1024*3   //������Ƶ���� 
#define  MaxLiveingAudioFifoBufferLength    1024*512      //������Ƶ���� 
#define  BaseRecvRtpSSRCNumber              0xFFFFFFFFFF  //���ڽ���TS����ʱ ���� ssrc ��ֵ��Ϊ�ؼ��� Key
#define  IDRFrameMaxBufferLength            1024*1024*2   //IDR֡��󻺴������ֽڴ�С
#define  MaxClientConnectTimerout           12*1000       //���ӷ��������ʱʱ�� 10 �� 

//rtsp url ��ַ�ֽ�
//rtsp://admin:szga2019@190.15.240.189:554
//rtsp ://190.16.37.52:554/03067970000000000102?DstCode=01&ServiceType=1&ClientType=1&StreamID=1&SrcTP=2&DstTP=2&SrcPP=1&DstPP=1&MediaTransMode=0&BroadcastType=0&Token=jCqM1pVyGb6stUfpLZDvgBG92nGzNBbP&DomainCode=49b5dca295cf42b283ca1d5dd2a0f398&UserId=8&
struct RtspURLParseStruct
{
	char szSrcRtspPullUrl[512]; //ԭʼURL
	char szDstRtspUrl[512];//����ַ���RTSP url 
	char szRequestFile[512];//������ļ� ���� http://admin:szga2019@190.15.240.189:9088/Media/Camera_00001/hls.m3u8 �е� /Media/Camera_00001/hls.m3u8
	                                  // ���� http://admin:szga2019@190.15.240.189:8088/Media/Camera_00001.flv �е� /Media/Camera_00001.flv
	char szRtspURLTrim[512];//ȥ����������ַ����õ���rtsp url 

	bool bHavePassword; //�Ƿ�������
	char szUser[32]; //�û���
	char szPwd[32];//����
	char szIP[32]; //IP
	char szPort[16];//�˿�

	char szRealm[64];//������֤����
	char szNonce[64];//������֤����

	RtspURLParseStruct()
	{
		memset(szSrcRtspPullUrl, 0x00, sizeof(szSrcRtspPullUrl));
		memset(szDstRtspUrl, 0x00, sizeof(szDstRtspUrl));
		memset(szRequestFile, 0x00, sizeof(szRequestFile));
		memset(szRtspURLTrim, 0x00, sizeof(szRtspURLTrim));

		memset(szUser, 0x00, sizeof(szUser));
		memset(szPwd, 0x00, sizeof(szPwd));
		memset(szIP, 0x00, sizeof(szIP));
		memset(szPort, 0x00, sizeof(szPort));
		memset(szRealm, 0x00, sizeof(szRealm));
		memset(szNonce, 0x00, sizeof(szNonce));
		bHavePassword = false;
	}
};

//��������ת�������ṹ
struct addStreamProxyStruct
{
	char  secret[256];//api�������� 
	char  vhost[64];//���������������
	char  app[128];//�������Ӧ����
	char  stream[128];//�������id 
	char  url[512];//������ַ ��֧�� rtsp\rtmp\http-flv \ hls 
	char  isRtspRecordURL[128];//�Ƿ�rtsp¼��ط� 
	char  enable_mp4[64];//�Ƿ�¼��
	char  enable_hls[64];//�Ƿ���hls
	char  convertOutWidth[64];//ת�������
	char  convertOutHeight[64];//ת������� 
	char  H264DecodeEncode_enable[64];//H264�Ƿ�����ٱ��� 

	addStreamProxyStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(url, 0x00, sizeof(url));
		memset(isRtspRecordURL, 0x00, sizeof(isRtspRecordURL));
		memset(enable_mp4, 0x00, sizeof(enable_mp4));
		memset(enable_hls, 0x00, sizeof(enable_hls));
		memset(convertOutWidth, 0x00, sizeof(convertOutWidth));
		memset(convertOutHeight, 0x00, sizeof(convertOutHeight));
		memset(H264DecodeEncode_enable, 0x00, sizeof(H264DecodeEncode_enable));
	}
};

//����ɾ������ṹ
struct delRequestStruct
{
	char  secret[256];//api�������� 
	char  key[128];//key
 
	delRequestStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
	}
};

//��������ת�������ṹ
struct addPushProxyStruct
{
	char  secret[256];//api�������� 
	char  vhost[64];//���������������
	char  app[128];//�������Ӧ����
	char  stream[128];//�������id 
	char  url[384];//������ַ ��֧�� rtsp\rtmp\http-flv \ hls 

	addPushProxyStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(url, 0x00, sizeof(url));
	}
};

//����GB28181��������
struct openRtpServerStruct
{
	char   secret[256];//api�������� 
	char   vhost[64];//���������������
	char   app[128];//�������Ӧ����
	char   stream_id[128];//�������id 
	char   port[64] ;//GB2818�˿�
	char   enable_tcp[16]; //0 UDP��1 TCP 
	char   payload[64]; //payload rtp �����payload 
	char   enable_mp4[64];//�Ƿ�¼��
	char   enable_hls[64];//�Ƿ���hls
	char  convertOutWidth[64];//ת�������
	char  convertOutHeight[64];//ת������� 
	char  H264DecodeEncode_enable[64];//H264�Ƿ�����ٱ��� 

	openRtpServerStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream_id, 0x00, sizeof(stream_id));
		memset(port, 0x00, sizeof(port));
		memset(enable_tcp, 0x00, sizeof(enable_tcp));
		memset(payload, 0x00, sizeof(payload));
		memset(enable_mp4, 0x00, sizeof(enable_mp4));
		memset(enable_hls, 0x00, sizeof(enable_hls));
		memset(convertOutWidth, 0x00, sizeof(convertOutWidth));
		memset(convertOutHeight, 0x00, sizeof(convertOutHeight));
		memset(H264DecodeEncode_enable, 0x00, sizeof(H264DecodeEncode_enable));
	}
};

//����GB28181��������
struct startSendRtpStruct
{
	char   secret[256];//api�������� 
	char   vhost[64];//���������������
	char   app[128];//�������Ӧ����
	char   stream[128];//�������id 
	char   ssrc[128];//ssrc
	char   src_port[64];//����Դ�󶨵Ķ˿ںţ�0 �Զ�����һ���˿ڣ�����0 ����û�ָ���Ķ˿�
	char   dst_url[512];//
	char   dst_port[64];//GB2818�˿�
	char   is_udp[16]; //0 UDP��1 TCP 
	char   payload[24]; //payload rtp �����payload 

	startSendRtpStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(ssrc, 0x00, sizeof(ssrc));
		memset(dst_url, 0x00, sizeof(dst_url));
		memset(dst_port, 0x00, sizeof(dst_port));
		memset(is_udp, 0x00, sizeof(is_udp));
		memset(payload, 0x00, sizeof(payload));
	}
};

//��ȡ�б�ṹ
struct getMediaListStruct 
{
	char   secret[256];//api�������� 
	char   vhost[256];//���������������
	char   app[256];//�������Ӧ����
	char   stream[256];//�������id 

	getMediaListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
	}
};

//��ȡ���ⷢ��ý���б�ṹ
struct getOutListStruct
{
	char   secret[256];//api�������� 
	char   outType[128];//ý��Դ���� rtsp ,rtmp ,flv ,hls ,gb28181 ,webrtc  
	getOutListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(outType, 0x00, sizeof(outType));
	}
};

//��ȡϵͳ����
struct getServerConfigStruct
{
	char   secret[256];//api�������� 
	getServerConfigStruct()
	{
		memset(secret, 0x00, sizeof(secret));
 	}
};

//�ر�Դ���ṹ
struct closeStreamsStruct
{
	char    secret[256];//api��������
	char    schema[256];
	char   	vhost[256];
	char   	app[256];
	char   	stream[256];
	int	    force ; //1 ǿ�ƹرգ������Ƿ����˹ۿ� ��0 �����ܹۿ�ʱ�����ر�
	closeStreamsStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(schema, 0x00, sizeof(schema));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		force = 1; 
	}
};

//��ʼ��ֹͣ¼��
struct startStopRecordStruct
{
	char  secret[256];//api�������� 
	char  vhost[64];//���������������
	char  app[128];//�������Ӧ����
	char  stream[128];//�������id 

	startStopRecordStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
 	}
};

//��ѯ¼���ļ��б�
struct queryRecordListStruct
{
	char  secret[256];//api�������� 
	char  vhost[64];//���������������
	char  app[128];//�������Ӧ����
	char  stream[128];//�������id 
	char  starttime[128];//��ʼʱ��
	char  endtime[128];//��ʼʱ��

	queryRecordListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(starttime, 0x00, sizeof(starttime));
		memset(endtime, 0x00, sizeof(endtime));
	}
};

//����ץ��
struct getSnapStruct
{
	char  secret[256];//api�������� 
	char  vhost[64];//���������������
	char  app[128];//�������Ӧ����
	char  stream[128];//�������id 
	char  timeout_sec[128];//ץ��ͼƬ��ʱ ����λ ��

	getSnapStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(timeout_sec, 0x00, sizeof(timeout_sec));
	}
};

//��ѯͼƬ�ļ��б�
struct queryPictureListStruct
{
	char  secret[256];//api�������� 
	char  vhost[64];//���������������
	char  app[128];//�������Ӧ����
	char  stream[128];//�������id 
	char  starttime[128];//��ʼʱ�� 
	char  endtime[128];//��ʼʱ��  

	queryPictureListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(starttime, 0x00, sizeof(starttime));
		memset(endtime, 0x00, sizeof(endtime));
	}
};

//��ѯͼƬ�ļ��б�
struct controlStreamProxy
{
	char  secret[256];//api�������� 
	char  key[64];//key
 	char  command[128];//���� 
	char  value[128];//ֵ

	controlStreamProxy()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
		memset(command, 0x00, sizeof(command));
		memset(value, 0x00, sizeof(value));
 	}
};

//�������ò���ֵ
struct SetConfigParamValue
{
	char  secret[256];//api�������� 
	char  key[128];//key
	char  value[128];//ֵ

	SetConfigParamValue()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
 		memset(value, 0x00, sizeof(value));
	}
};

enum NetRevcBaseClientType
{
	NetRevcBaseClient_ServerAccept               = 1, //�������˿ڽ��� ������ 554,8080,8088,8089,1935 �ȵȶ˿�accept������
	NetRevcBaseClient_addStreamProxy             = 2, //����������ʽ
	NetRevcBaseClient_addPushStreamProxy         = 3, //����������ʽ
	NetRevcBaseClient_addStreamProxyControl      = 4, //���ƴ�������
	NetRevcBaseClient_addPushProxyControl        = 5, //���ƴ�������
	NetRevcBaseClient__NetGB28181Proxy           = 6, //GB28181���� 
};

//http������� 
struct RequestKeyValue
{
	char key[512];
	char value[1280];
	RequestKeyValue()
	{
		memset(key, 0x00, sizeof(key));
		memset(value, 0x00, sizeof(value));
	}
};

//http ����������
enum HttpReponseIndexApiCode
{
	IndexApiCode_OK           = 0 ,     //��������
	IndexApiCode_ErrorRequest = -100,   //�Ƿ�����
	IndexApiCode_secretError  = -200,   //secret����
	IndexApiCode_ParamError   = -300,   //��������
	IndexApiCode_KeyNotFound  = -400,   //Key û���ҵ�
	IndexApiCode_SqlError     = -500,   //Sql����
	IndexApiCode_ConnectFail  = -600,   //����ʧ��
	IndexApiCode_RtspSDPError = -700,   //rtsp����ʧ��
	IndexApiCode_RtmpPushError = -800,  //rtmp����ʧ��
	IndexApiCode_BindPortError = -900,  //�����ʧ��
	IndexApiCode_ConnectTimeout = -1000, //�������ӳ�ʱ
	IndexApiCode_HttpJsonError = -1001, //http ���� json�����Ƿ�
	IndexApiCode_HttpProtocolError = -1002, //http ���� Э�����
	IndexApiCode_MediaSourceNotFound = -1003, //����ý��Դû���ҵ� 
	IndexApiCode_RequestProcessFailed  = -1004 ,//ִ��ʧ�� 
	IndexApiCode_RequestFileNotFound = -1005,//�ļ�û���ҵ�
	IndexApiCode_ContentTypeNotSupported = -1006,//Content-Type ���Ͳ�֧��
	IndexApiCode_OverMaxSameTimeSnap = -1007,//�������ץ������
	IndexApiCode_AppStreamAlreadyUsed = -1008,//app/sream �Ѿ�ʹ����
	IndexApiCode_PortAlreadyUsed      = -1009,//port �Ѿ�ʹ����
	IndexApiCode_SSRClreadyUsed       = -1010,//SSRC �Ѿ�ʹ����
	IndexApiCode_TranscodingVideoFilterNotEnable = -1011,//ת�롢ˮӡ������δ����
	IndexApiCode_TranscodingVideoFilterTakeEffect = -1012,//ת�롢ˮӡ������δ��Ч
	IndexApiCode_RecvRtmpFailed = -1100, //��ȡrtmp����ʧ��
	IndexApiCode_AppStreamHaveUsing = -1200, //app,stream ����ʹ�ã�����������δ���� 
};

//rtsp�������� DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD
enum RtspProcessStep
{
	RtspProcessStep_Initing = 0,//rtsp �ոճ�ʼ��
	RtspProcessStep_OPTIONS = 1, //������� OPTIONS
	RtspProcessStep_ANNOUNCE = 2,//������� ANNOUNCE
	RtspProcessStep_DESCRIBE = 3,//������� DESCRIBE
	RtspProcessStep_SETUP = 4,//������� SETUP
	RtspProcessStep_RECORD = 5,//������� RECORD 
	RtspProcessStep_PLAY = 6,//ֻ�Ƿ�����Play���� 
	RtspProcessStep_PLAYSucess = 7,//������� PLAY
	RtspProcessStep_PAUSE = 8,//������� PAUSE
	RtspProcessStep_TEARDOWN = 9 //������� TEARDOWN
};
//rtsp��֤��ʽ 
enum  WWW_AuthenticateType
{
	WWW_Authenticate_UnKnow = -1, //��δȷ����ʽ
	WWW_Authenticate_None = 0,  //����֤
	WWW_Authenticate_MD5 = 1,  //MD5��֤��ժҪ��֤
	WWW_Authenticate_Basic = 2    //base 64 ������֤
};

//����������� 
enum NetGB28181ProxyType
{
	NetGB28181ProxyType_RecvStream = 1, //��������
	NetGB28181ProxyType_PushStream = 2 ,//��������
};

//accept ��������������
enum NetServerHandleType
{
	NetServerHandleType_GB28181RecvStream = 10, //tcp��ʽ���չ�������
};

//����
struct NetServerHandleParam
{
	int      nNetServerHandleType;
	uint64_t hParent; //�����Ķ��� 
	char     szMediaSource[256];
	int      nAcceptNumber; //���Ӵ��� 
	NetServerHandleParam()
	{
		nNetServerHandleType = 0;
		hParent = 0;
		memset(szMediaSource, 0x00, sizeof(szMediaSource));
		nAcceptNumber = 0 ;
	}
};

//rtp��ͷ
struct _rtp_header
{
	uint8_t cc : 4;
	uint8_t x : 1;
	uint8_t p : 1;
	uint8_t v : 2;
	uint8_t payload : 7;
	uint8_t mark : 1;
	uint16_t seq;
	uint32_t timestamp;
	uint32_t ssrc;

	_rtp_header()
		: v(2), p(0), x(0), cc(0)
		, mark(0), payload(0)
		, seq(0)
		, timestamp(0)
		, ssrc(0)
	{
	}
};

struct _rtsp_header
{
	unsigned char  head;
	unsigned char  chan;
	unsigned short Length;
};

//ý��Դ����
enum MediaSourceType
{
	MediaSourceType_LiveMedia = 0,  //ʵ������
	MediaSourceType_ReplayMedia = 1, //¼��㲥
};

//rtsp ����ʱ rtp ���ص��������� 
enum RtspRtpPayloadType
{
	RtspRtpPayloadType_Unknow = 0,  //δ֪
	RtspRtpPayloadType_ES     = 1, //rtp����ES 
	RtspRtpPayloadType_PS     = 2, //rtp����PS 
};

//ͼƬ����
enum HttpImageType
{
	HttpImageType_jpeg = 1,  //jpeg
    HttpImageType_png = 2, //png
};
bool          ABLDeleteFile(char* szFileName);

//��Ϣ֪ͨ�ṹ 
struct MessageNoticeStruct
{
	uint64_t nClient;
	char     szMsg[2048];
	int      nSendCount;
	MessageNoticeStruct()
	{
		nClient = 0;
		nSendCount = 0;
		memset(szMsg, 0x00, sizeof(szMsg));
	}
};

#ifndef OS_System_Windows
unsigned long GetTickCount();
unsigned long GetTickCount64();

#endif

#include "XHNetSDK.h"
#include "ABLSipParse.h"

#ifdef OS_System_Windows 
#include "ABLogSDK.h"
#include "ConfigFile.h"

void malloc_trim(int n);

#else 
#include "ABLogFile.h"
#include "Ini.h"
#endif

//#include <boost/unordered/unordered_map.hpp>
//#include <boost/smart_ptr/shared_ptr.hpp>
//#include <boost/unordered/unordered_map.hpp>
//#include <boost/make_shared.hpp>
//#include <boost/algorithm/string.hpp>
//#include <boost/algorithm/string/replace.hpp>
//
//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_io.hpp>
//#include <boost/uuid/uuid_generators.hpp>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/base64.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>	
#include <libswresample/swresample.h>
}
using namespace std;
//using namespace boost;
using namespace rapidjson;

typedef list<int> LogFileVector;

#define SAFE_ARRAY_DELETE(x) if( x != NULL ) { delete[] x; x = NULL; }
#define SAFE_RELEASE(x)  if( x != NULL ) { x->Release(); x = NULL; }
#define SAFE_DELETE(x)   if( x != NULL ) { delete x; x = NULL; }

#include "NetRecvBase.h"
#include "NetServerHTTP.h"
#include "NetRtspServer.h"
#include "NetRtmpServerRecv.h"
#include "NetServerHTTP_FLV.h"
#include "NetServerWS_FLV.h"
#include "NetServerHLS.h"
#include "NetClientHttp.h"
#include "NetClientSnap.h"

#include "ps_demux.h"
#include "ps_mux.h"
#include "MediaFifo.h"
#include "NetBaseThreadPool.h"
#include "FFVideoDecode.h"
#include "FFVideoEncode.h"

#include "MediaStreamSource.h"
#include "MediaSendThreadPool.h"
#include "rtp_depacket.h"
#include "NetServerRecvRtpTS_PS.h"
#include "RtpTSStreamInput.h"
#include "RtpPSStreamInput.h"

#include "NetClientRecvHttpHLS.h"
#include "NetClientRecvRtmp.h"
#include "NetClientRecvFLV.h"
#include "NetClientRecvRtsp.h"
#include "NetClientRecvJTT1078.h"
#include "NetClientSendRtsp.h"
#include "NetClientSendRtmp.h"
#include "NetClientAddStreamProxy.h"
#include "NetClientAddPushProxy.h"
#include "NetGB28181Listen.h"
#include "NetGB28181RtpServer.h"
#include "NetGB28181RtpClient.h"
#include "NetServerHTTP_MP4.h"
#include "StreamRecordFMP4.h"
#include "StreamRecordMP4.h"
#include "RecordFileSource.h"
#include "PictureFileSource.h"
#include "ReadRecordFileInput.h"
#include "LCbase64.h"
#include "SHA1.h"

#endif
