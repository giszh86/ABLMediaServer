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

//媒体格式信息
struct MediaCodecInfo
{
	char szVideoName[64]; //H264、H265 
	int  nVideoFrameRate; //视频帧速度 

	char szAudioName[64]; //AAC、 G711A、 G711U
	int  nChannels;       //通道数量 1 、2 
	int  nSampleRate;     //采样频率 8000、16000、22050 、32000、 44100 、 48000 

	MediaCodecInfo()
	{
		memset(szVideoName, 0x00, sizeof(szVideoName));
		nVideoFrameRate = 25;
		memset(szAudioName, 0x00, sizeof(szAudioName));
		nChannels = 0;
		nSampleRate = 0;
	};
};

//媒体服务器运行端口结构
struct MediaServerPort
{
	char secret[256];  //api操作密码
	int  nHttpServerPort; //http服务端口
	int  nRtspPort;     //rtsp
	int  nRtmpPort;     //rtmp
	int  nHttpFlvPort;  //http-flv

	int  nHlsPort;     //Hls 端口 
	int  nHlsEnable;   //HLS 是否开启 
	int  nHLSCutType;  //HLS切片方式  1 硬盘，2 内存 
	int  nH265CutType; //H265切片方式 1  切片为TS ，2 切片为 mp4  

	int  nRecvThreadCount;//用于网络数据接收线程数量 
	int  nSendThreadCount;//用于网络数据发送线程数量 
	int  nRtpPacketOfRtsp;//rtp负载方式 1 ES，2 PS 
	int  nGBRtpTCPHeadType;  //GB28181 TCP 方式发送rtp(负载PS)码流时，包头长度选择（1： 4个字节方式，2：2个字节方式）
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

//网络基本类型
enum NetBaseNetType
{
	NetBaseNetType_Unknown = 20,  //未定义的网络类型
	NetBaseNetType_RtmpServerRecvPush = 21,//RTMP 服务器，接收客户端的推流 
	NetBaseNetType_RtmpServerSendPush = 22,//RTMP 服务器，转发客户端的推上来的码流
	NetBaseNetType_RtspServerRecvPush = 23,//RTSP 服务器，接收客户端的推流 
	NetBaseNetType_RtspServerSendPush = 24,//RTSP 服务器，转发客户端的推上来的码流
	NetBaseNetType_HttpFLVServerSendPush = 25,//Http-FLV 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流
	NetBaseNetType_HttpHLSServerSendPush = 26,//Http-HLS 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流
	NetBaseNetType_WebRtcServerRecvPush = 27,//WebRtc 服务器，接收客户端的推流 
	NetBaseNetType_WebRtcServerSendPush = 28,//WebRtc 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流

	//主动拉流对象
	NetBaseNetType_RtspClientRecv = 30,//rtsp主动拉流对象 
	NetBaseNetType_RtmpClientRecv = 31,//rtmp主动拉流对象 
	NetBaseNetType_HttpFlvClientRecv = 32,//http-flv主动拉流对象 
	NetBaseNetType_HttpHLSClientRecv = 33,//http-hls主动拉流对象 

	//主动推流对象
	NetBaseNetType_RtspClientPush = 40,//rtsp主动推流对象 
	NetBaseNetType_RtmpClientPush = 41,//rtmp主动推流对象 
	NetBaseNetType_GB28181ClientPushTCP = 42,//GB28181主动推流对象 
	NetBaseNetType_GB28181ClientPushUDP = 43,//GB28181主动推流对象 

	NetBaseNetType_addStreamProxyControl = 50,//控制代理rtsp\rtmp\flv\hsl 拉流
	NetBaseNetType_addPushProxyControl = 51,//控制代理rtsp\rtmp  推流 代理

	NetBaseNetType_NetGB28181RtpServerUDP = 60,//国标28181 UDP方式 接收码流
	NetBaseNetType_NetGB28181RtpServerTCP_Server = 61,//国标28181 TCP方式 接收码流,被动连接方式 
	NetBaseNetType_NetGB28181RtpServerTCP_Client = 62,//国标28181 TCP方式 接收码流,被动连接方式 
	NetBaseNetType_NetGB28181SendRtpUDP = 65,//国标28181 UDP方式 推送码流
	NetBaseNetType_NetGB28181SendRtpTCP_Connect = 66,//国标28181 TCP方式 接收码流,主动连接方式 推送码流
	NetBaseNetType_NetGB28181SendRtpTCP_Server = 67,//国标28181 TCP方式 接收码流,被动连接方式 推送码流

};

#define   MediaServerVerson                 "ABLRtspClient-3.3.9(2021-10-08)"
#define   RtspServerPublic                  "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD，GET_PARAMETER"

#define  MaxNetDataCacheBufferLength        1024*1024*4   //网络缓存区域字节大小
#define  MaxLiveingVideoFifoBufferLength    1024*1024*4   //最大的视频缓存 
#define  MaxLiveingAudioFifoBufferLength    1024*512      //最大的音频缓存 
#define  MaxRecvDataTimerBySecondDiconnect  60            //最大30秒没数据，即执行删除
#define  MaxReconnctTimeCount               20            //最大重连次数 
#define  OneMicroSecondTime                 1000          //1毫秒

//rtsp url 地址分解
//rtsp://admin:szga2019@190.15.240.189:554
//rtsp ://190.16.37.52:554/03067970000000000102?DstCode=01&ServiceType=1&ClientType=1&StreamID=1&SrcTP=2&DstTP=2&SrcPP=1&DstPP=1&MediaTransMode=0&BroadcastType=0&Token=jCqM1pVyGb6stUfpLZDvgBG92nGzNBbP&DomainCode=49b5dca295cf42b283ca1d5dd2a0f398&UserId=8&
struct RtspURLParseStruct
{
	char szSrcRtspPullUrl[512]; //原始URL
	char szDstRtspUrl[512];//打算分发的RTSP url 
	char szRequestFile[512];//请求的文件 比如 http://admin:szga2019@190.15.240.189:9088/Media/Camera_00001/hls.m3u8 中的 /Media/Camera_00001/hls.m3u8
	// 比如 http://admin:szga2019@190.15.240.189:8088/Media/Camera_00001.flv 中的 /Media/Camera_00001.flv
	char szRtspURLTrim[512];//去掉？后面的字符串得到的rtsp url 

	bool bHavePassword; //是否有密码
	char szUser[32]; //用户名
	char szPwd[32];//密码
	char szIP[32]; //IP
	char szPort[16];//端口

	char szRealm[64];//用于认证方面
	char szNonce[64];//用于认证方面

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

//代理拉流转发参数结构
struct addStreamProxyStruct
{
	char  secret[256];//api操作密码 
	char  vhost[64];//添加流的虚拟主机
	char  app[128];//添加流的应用名
	char  stream[128];//添加流的id 
	char  url[512];//拉流地址 ，支持 rtsp\rtmp\http-flv \ hls 

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
	NetRevcBaseClient_ServerAccept = 1, //服务器端口接入 ，比如 554,8080,8088,8089,1935 等等端口accept进来的
	NetRevcBaseClient_addStreamProxy = 2, //代理拉流方式
	NetRevcBaseClient_addPushStreamProxy = 3, //代理推流方式
	NetRevcBaseClient_addStreamProxyControl = 4, //控制代理拉流
	NetRevcBaseClient_addPushProxyControl = 5, //控制代理推流
	NetRevcBaseClient__NetGB28181Proxy = 6, //GB28181代理 
};

//rtsp交互过程 DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD
enum RtspProcessStep
{
	RtspProcessStep_Initing = 0,//rtsp 刚刚初始化
	RtspProcessStep_OPTIONS = 1, //交互完毕 OPTIONS
	RtspProcessStep_ANNOUNCE = 2,//交互完毕 ANNOUNCE
	RtspProcessStep_DESCRIBE = 3,//交互完毕 DESCRIBE
	RtspProcessStep_SETUP = 4,//交互完毕 SETUP
	RtspProcessStep_RECORD = 5,//交互完毕 RECORD 
	RtspProcessStep_PLAY = 6,//只是发送了Play命令 
	RtspProcessStep_PLAYSucess = 7,//交互完毕 PLAY
	RtspProcessStep_PAUSE = 8,//交互完毕 PAUSE
	RtspProcessStep_TEARDOWN = 9 //交互完毕 TEARDOWN
};
//rtsp验证方式 
enum  WWW_AuthenticateType
{
	WWW_Authenticate_UnKnow = -1, //尚未确定方式
	WWW_Authenticate_None = 0,  //不认证
	WWW_Authenticate_MD5 = 1,  //MD5认证，摘要验证
	WWW_Authenticate_Basic = 2    //base 64 基础认证
};

//rtsp连接状态
enum RtspConnectStatus
{
	RtspConnectStatus_NoConnect = 0,
	RtspConnectStatus_AtConnecting = 1,//正在连接 
	RtspConnectStatus_ConnectSuccess = 2,//连接成功
	RtspConnectStatus_ConnectFailed = 3 //连接失败
};

//rtsp头
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
//rtp包头
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
