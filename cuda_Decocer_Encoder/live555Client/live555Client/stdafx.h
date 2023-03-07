// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef  _Stdafx_H
#define  _Stdafx_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <float.h>

#include<sys/types.h> 
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h> 

#include <pthread.h>
#include <signal.h>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <math.h>
#include <iconv.h>

//ý���ʽ��Ϣ
struct MediaCodecInfo
{
	char szVideoName[64]; //H264��H265 
	int  nVideoFrameRate; //��Ƶ֡�ٶ� 

	char szAudioName[64]; //AAC�� G711A�� G711U
	int  nChannels;       //ͨ������ 1 ��2 
	int  nSampleRate;     //����Ƶ�� 8000��16000��22050 ��32000�� 44100 �� 48000 

	MediaCodecInfo()
	{
		memset(szVideoName, 0x00, sizeof(szVideoName));
		nVideoFrameRate = 25;
		memset(szAudioName, 0x00, sizeof(szAudioName));
		nChannels = 0;
		nSampleRate = 0;
	};
};

//ý����������ж˿ڽṹ
struct MediaServerPort
{
	char secret[256];  //api��������
	int  nHttpServerPort; //http����˿�
	int  nRtspPort;     //rtsp
	int  nRtmpPort;     //rtmp
	int  nHttpFlvPort;  //http-flv

	int  nHlsPort;     //Hls �˿� 
	int  nHlsEnable;   //HLS �Ƿ��� 
	int  nHLSCutType;  //HLS��Ƭ��ʽ  1 Ӳ�̣�2 �ڴ� 
	int  nH265CutType; //H265��Ƭ��ʽ 1  ��ƬΪTS ��2 ��ƬΪ mp4  

	int  nRecvThreadCount;//�����������ݽ����߳����� 
	int  nSendThreadCount;//�����������ݷ����߳����� 
	int  nRtpPacketOfRtsp;//rtp���ط�ʽ 1 ES��2 PS 
	int  nGBRtpTCPHeadType;  //GB28181 TCP ��ʽ����rtp(����PS)����ʱ����ͷ����ѡ��1�� 4���ֽڷ�ʽ��2��2���ֽڷ�ʽ��
	MediaServerPort()
	{
		memset(secret, 0x00, sizeof(secret));
		nRtspPort = 554;
		nRtmpPort = 1935;
		nHttpFlvPort = 8088;

		nHlsPort = 9088;
		nHlsEnable = 0;
		nHLSCutType = 1;
		nH265CutType = 1;

		nRecvThreadCount = 64;
		nSendThreadCount = 64;
		nRtpPacketOfRtsp = 1;
		nGBRtpTCPHeadType = 1;
	}
};

//�����������
enum NetBaseNetType
{
	NetBaseNetType_Unknown = 20,  //δ�������������
	NetBaseNetType_RtmpServerRecvPush = 21,//RTMP �����������տͻ��˵����� 
	NetBaseNetType_RtmpServerSendPush = 22,//RTMP ��������ת���ͻ��˵�������������
	NetBaseNetType_RtspServerRecvPush = 23,//RTSP �����������տͻ��˵����� 
	NetBaseNetType_RtspServerSendPush = 24,//RTSP ��������ת���ͻ��˵�������������
	NetBaseNetType_HttpFLVServerSendPush = 25,//Http-FLV ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_HttpHLSServerSendPush = 26,//Http-HLS ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_WebRtcServerRecvPush = 27,//WebRtc �����������տͻ��˵����� 
	NetBaseNetType_WebRtcServerSendPush = 28,//WebRtc ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������

	//������������
	NetBaseNetType_RtspClientRecv = 30,//rtsp������������ 
	NetBaseNetType_RtmpClientRecv = 31,//rtmp������������ 
	NetBaseNetType_HttpFlvClientRecv = 32,//http-flv������������ 
	NetBaseNetType_HttpHLSClientRecv = 33,//http-hls������������ 

	//������������
	NetBaseNetType_RtspClientPush = 40,//rtsp������������ 
	NetBaseNetType_RtmpClientPush = 41,//rtmp������������ 
	NetBaseNetType_GB28181ClientPushTCP = 42,//GB28181������������ 
	NetBaseNetType_GB28181ClientPushUDP = 43,//GB28181������������ 

	NetBaseNetType_addStreamProxyControl = 50,//���ƴ���rtsp\rtmp\flv\hsl ����
	NetBaseNetType_addPushProxyControl = 51,//���ƴ���rtsp\rtmp  ���� ����

	NetBaseNetType_NetGB28181RtpServerUDP = 60,//����28181 UDP��ʽ ��������
	NetBaseNetType_NetGB28181RtpServerTCP_Server = 61,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181RtpServerTCP_Client = 62,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181SendRtpUDP = 65,//����28181 UDP��ʽ ��������
	NetBaseNetType_NetGB28181SendRtpTCP_Connect = 66,//����28181 TCP��ʽ ��������,�������ӷ�ʽ ��������
	NetBaseNetType_NetGB28181SendRtpTCP_Server = 67,//����28181 TCP��ʽ ��������,�������ӷ�ʽ ��������

};

#define   MediaServerVerson                 "ABLRtspClient-3.3.9(2021-10-08)"
#define   RtspServerPublic                  "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD��GET_PARAMETER"

#define  MaxNetDataCacheBufferLength        1024*1024*4   //���绺�������ֽڴ�С
#define  MaxLiveingVideoFifoBufferLength    1024*1024*4   //������Ƶ���� 
#define  MaxLiveingAudioFifoBufferLength    1024*512      //������Ƶ���� 
#define  MaxRecvDataTimerBySecondDiconnect  60            //���30��û���ݣ���ִ��ɾ��
#define  MaxReconnctTimeCount               20            //����������� 
#define  OneMicroSecondTime                 1000          //1����

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

	addStreamProxyStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(url, 0x00, sizeof(url));
	}
};

enum NetRevcBaseClientType
{
	NetRevcBaseClient_ServerAccept = 1, //�������˿ڽ��� ������ 554,8080,8088,8089,1935 �ȵȶ˿�accept������
	NetRevcBaseClient_addStreamProxy = 2, //����������ʽ
	NetRevcBaseClient_addPushStreamProxy = 3, //����������ʽ
	NetRevcBaseClient_addStreamProxyControl = 4, //���ƴ�������
	NetRevcBaseClient_addPushProxyControl = 5, //���ƴ�������
	NetRevcBaseClient__NetGB28181Proxy = 6, //GB28181���� 
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

//rtsp����״̬
enum RtspConnectStatus
{
	RtspConnectStatus_NoConnect = 0,
	RtspConnectStatus_AtConnecting = 1,//�������� 
	RtspConnectStatus_ConnectSuccess = 2,//���ӳɹ�
	RtspConnectStatus_ConnectFailed = 3 //����ʧ��
};

//rtspͷ
struct _rtsp_header
{
	unsigned char  head;
	unsigned char  chan;
	unsigned short Length;

	_rtsp_header()
	{
		head = 0;
		chan = 0;
		Length = 0;
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
using namespace std;

typedef list<int> LogFileVector;

#include "XHNetSDK.h"
#include "RtspABLogFile.h"
#include "RtspABLSipParse.h"

#include "NetRecvBase.h"
#include "NetBaseThreadPool.h"
#include "NetClientAddStreamProxy.h"
#include "NetClientRecvRtsp.h"

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace boost;

#define SAFE_ARRAY_DELETE(x) if( x != NULL ) { delete[] x; x = NULL; }
#define SAFE_RELEASE(x)  if( x != NULL ) { x->Release(); x = NULL; }
#define SAFE_DELETE(x)   if( x != NULL ) { delete x; x = NULL; }

#endif
