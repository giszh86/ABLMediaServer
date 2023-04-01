// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef  _Stdafx_H
#define  _Stdafx_H

//定义当前操作系统为Windows 
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

#include "cudaCodecDLL.h"

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
#include <thread>

#define      BYTE     unsigned char 

#endif

uint64_t GetCurrentSecond();
uint64_t GetCurrentSecondByTime(char* szDateTime);
bool     QureyRecordFileFromRecordSource(char* szShareURL, char* szFileName);

//rtsp 播放类型
enum ABLRtspPlayerType
{
	RtspPlayerType_Unknow = -1 ,  //未知 
	RtspPlayerType_Liveing = 0 , //实时播放
	RtspPlayerType_RecordReplay = 1 ,//录像回放
	RtspPlayerType_RecordDownload = 2,//录像下载
};

//媒体格式信息
struct MediaCodecInfo
{
	char szVideoName[64]; //H264、H265 
	int  nVideoFrameRate; //视频帧速度 
	int  nWidth;          //视频宽
	int  nHeight;         //视频高
	int  nVideoBitrate;   //视频码率

	char szAudioName[64]; //AAC、 G711A、 G711U
	int  nChannels;       //通道数量 1 、2 
	int  nSampleRate;     //采样频率 8000、16000、22050 、32000、 44100 、 48000 
	int  nBaseAddAudioTimeStamp;//每次音频递增
	int  nAudioBitrate;   //音频码率

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

//媒体服务器运行端口结构
struct MediaServerPort
{
	char secret[256];  //api操作密码
	uint64_t nServerStartTime;//服务器启动时间
	bool     bNoticeStartEvent;//是否已经通知上线 
	int  nHttpServerPort; //http服务端口
	int  nRtspPort;     //rtsp
	int  nRtmpPort;     //rtmp
	int  nHttpFlvPort;  //http-flv
	int  nWSFlvPort;    //ws-flv
	int  nHttpMp4Port;  //http-mp4
	int  ps_tsRecvPort; //国标单端口

	int  nHlsPort;     //Hls 端口 
	int  nHlsEnable;   //HLS 是否开启 
	int  nHLSCutType;  //HLS切片方式  1 硬盘，2 内存 
	int  nH265CutType; //H265切片方式 1  切片为TS ，2 切片为 mp4  
	int  hlsCutTime; //切片时长
	int  nMaxTsFileCount;//保存最大TS切片文件数量
	char wwwPath[256];//hls切片路径

	int  nRecvThreadCount;//用于网络数据接收线程数量 
	int  nSendThreadCount;//用于网络数据发送线程数量
	int  nRecordReplayThread;//用于录像回放，读取文件的线程数量
	int  nGBRtpTCPHeadType;  //GB28181 TCP 方式发送rtp(负载PS)码流时，包头长度选择（1： 4个字节方式，2：2个字节方式）
	int  nEnableAudio;//是否启用音频

	int  nIOContentNumber; //ioContent数量
	int  nThreadCountOfIOContent;//每个iocontent上创建的线程数量
	int  nReConnectingCount;//重连次数

	char recordPath[256];//录像保存路径
	int  pushEnable_mp4;//推流上来是否开启录像
	int  fileSecond;//fmp4切割时长
	int  videoFileFormat;//录像文件格式 1 为 fmp4, 2 为 mp4 
	int  fileKeepMaxTime;//录像文件最大保留时长，单位小时
	int  httpDownloadSpeed;//http录像下载速度设定
	int  fileRepeat;//MP4点播(rtsp/rtmp/http-flv/ws-flv)是否循环播放文件

	char picturePath[256];//图片抓拍保存路径
	int  pictureMaxCount; //每路媒体源最大抓拍保留数量
	int  snapOutPictureWidth;//抓拍输出宽
	int  snapOutPictureHeight;//抓拍输出高
	int  captureReplayType; //抓拍返回类型
	int  snapObjectDestroy;//抓拍对象是否销毁
	int  snapObjectDuration;//抓拍对象最长生存时长，单位秒
	int  maxSameTimeSnap;//抓拍最大并发数量
	int  maxTimeNoOneWatch;//无人观看最大时长 
	int  nG711ConvertAAC; //是否转换为AAC 
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
	float nFilterFontAlpha;//透明度
	int  nFilterFontLeft;//x坐标
	int  nFilterFontTop;//y坐标

	//事件通知模块
	int  hook_enable;//是否开启事件通知
	int  noneReaderDuration;//无人观看时间长
	char on_stream_arrive[256];
	char on_stream_not_arrive[256]; //码流未到达 代理拉流，国标接收流中支持 
	char on_stream_none_reader[256];
	char on_stream_disconnect[256];
	char on_stream_not_found[256];
	char on_record_mp4[256];
	char on_record_progress[256];//录像进度
	char on_record_ts[256];
	char on_server_started[256];
	char on_server_keepalive[256];
	char on_delete_record_mp4[256];
	char on_play[256];
	char on_publish[256];

	uint64_t    nClientNoneReader;
	uint64_t    nClientNotFound;
	uint64_t    nClientRecordMp4;
	uint64_t    nClientDeleteRecordMp4;
	uint64_t    nClientRecordProgress;
	uint64_t    nClientArrive;
	uint64_t    nClientNotArrive;
	uint64_t    nClientDisconnect;
	uint64_t    nClientRecordTS;
	uint64_t    nServerStarted;//服务器启动
	uint64_t    nServerKeepalive;//服务器保活消息 
	uint64_t    nPlay;//播放
	uint64_t    nPublish;//接入
	int         MaxDiconnectTimeoutSecond;//最大断线超时检测
	int         ForceSendingIFrame;//强制发送I帧 
	uint64_t    nServerKeepaliveTime;//服务器心跳时间

	char       debugPath[256];//调试文件
	int        nSaveProxyRtspRtp;//是否保存代理拉流数据0 不保存，1 保存
	int        nSaveGB28181Rtp;//是否保存GB28181数据，0 未保存，1 保存 

	int        gb28181LibraryUse;//国标打包、解包库的选择, 1 使用自研库国标打包解包库，2 使用北京老陈国标打包解包库 
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
		memset(on_play, 0x00, sizeof(on_play));
		memset(on_publish, 0x00, sizeof(on_publish));

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
		nPlay=0;//播放
	    nPublish=0;//接入

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
		nFilterFontAlpha = 0.6;//透明度
		nFilterFontLeft = 10;//x坐标
		nFilterFontTop = 10 ;//y坐标
		MaxDiconnectTimeoutSecond = 16;
		ForceSendingIFrame = 0;

		nSaveProxyRtspRtp = 0; 
		nSaveGB28181Rtp = 0 ; 
		memset(debugPath, 0x00, sizeof(debugPath));

		gb28181LibraryUse = 1;
 	}
};

//真对单独某一路视频转码结构
struct H265ConvertH264Struct
{
	int  H265ConvertH264_enable;//H265是否转码
	int  H264DecodeEncode_enable;//h264是否重新解码，再编码 
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

//网络基本类型
enum NetBaseNetType
{
	NetBaseNetType_Unknown                 = 20 ,  //未定义的网络类型
	NetBaseNetType_RtmpServerRecvPush      = 21,//RTMP 服务器，接收客户端的推流 
	NetBaseNetType_RtmpServerSendPush      = 22,//RTMP 服务器，转发客户端的推上来的码流
	NetBaseNetType_RtspServerRecvPush      = 23,//RTSP 服务器，接收客户端的推流 
	NetBaseNetType_RtspServerSendPush      = 24,//RTSP 服务器，转发客户端的推上来的码流
	NetBaseNetType_HttpFLVServerSendPush   = 25,//Http-FLV 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流
	NetBaseNetType_HttpHLSServerSendPush   = 26,//Http-HLS 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流
	NetBaseNetType_WsFLVServerSendPush     = 27,//WS-FLV 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流
	NetBaseNetType_HttpMP4ServerSendPush   = 28,//http-mp4 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流以mp4发送出去
	NetBaseNetType_WebRtcServerSendPush    = 29,//WebRtc 服务器，转发 rtsp 、rtmp 、GB28181等等推流上来的码流

	//主动拉流对象
	NetBaseNetType_RtspClientRecv          = 30 ,//rtsp主动拉流对象 
	NetBaseNetType_RtmpClientRecv          = 31 ,//rtmp主动拉流对象 
	NetBaseNetType_HttpFlvClientRecv       = 32 ,//http-flv主动拉流对象 
	NetBaseNetType_HttpHLSClientRecv       = 33 ,//http-hls主动拉流对象 
	NetBaseNetType_HttClientRecvJTT1078    = 34, //接收交通部JTT1078 

	//主动推流对象
	NetBaseNetType_RtspClientPush          = 40,//rtsp主动推流对象 s
	NetBaseNetType_RtmpClientPush          = 41,//rtmp主动推流对象 
 	NetBaseNetType_GB28181ClientPushTCP    = 42,//GB28181主动推流对象 
	NetBaseNetType_GB28181ClientPushUDP    = 43,//GB28181主动推流对象 

	NetBaseNetType_addStreamProxyControl   = 50,//控制代理rtsp\rtmp\flv\hsl 拉流
	NetBaseNetType_addPushProxyControl     = 51,//控制代理rtsp\rtmp  推流 代理

	NetBaseNetType_NetGB28181RtpServerListen      = 56,//国标TCP方式接收Listen类
	NetBaseNetType_NetGB28181RtpServerUDP         = 60,//国标28181 UDP方式 接收码流
	NetBaseNetType_NetGB28181RtpServerTCP_Server  = 61,//国标28181 TCP方式 接收码流,被动连接方式 
	NetBaseNetType_NetGB28181RtpServerTCP_Client  = 62,//国标28181 TCP方式 接收码流,主动连接方式 
	NetBaseNetType_NetGB28181RtpServerRTCP        = 63,//国标28181 UDP方式 接收码流 中的 rtcp 包
	NetBaseNetType_NetGB28181SendRtpUDP           = 65,//国标28181 UDP方式 推送码流
	NetBaseNetType_NetGB28181SendRtpTCP_Connect   = 66,//国标28181 TCP方式 接收码流,主动连接方式 推送码流
	NetBaseNetType_NetGB28181SendRtpTCP_Server    = 67,//国标28181 TCP方式 接收码流,被动连接方式 推送码流
	NetBaseNetType_NetGB28181RecvRtpPS_TS         = 68,//国标28181 单端口接收PS、TS码流
	NetBaseNetType_NetGB28181UDPTSStreamInput     = 69,//TS推流接入
	NetBaseNetType_NetGB28181UDPPSStreamInput     = 64,//PS推流接入国标单端口推流接入

	NetBaseNetType_RecordFile_FMP4                = 70,//录像存储为fmp4格式
	NetBaseNetType_RecordFile_TS                  = 71,//录像存储为TS格式
	NetBaseNetType_RecordFile_PS                  = 72,//录像存储为PS格式
	NetBaseNetType_RecordFile_FLV                 = 73,//录像存储为flv格式
	NetBaseNetType_RecordFile_MP4                 = 74,//录像存储为mp4格式

 	ReadRecordFileInput_ReadFMP4File              = 80,//以读取fmp4文件格式 
	ReadRecordFileInput_ReadTSFile                = 81,//以读取TS文件格式 
	ReadRecordFileInput_ReadPSFile                = 82,//以读取PS文件格式 
	ReadRecordFileInput_ReadFLVFile               = 83,//以读取FLV文件格式 

	NetBaseNetType_HttpClient_None_reader           = 90,//无人观看
	NetBaseNetType_HttpClient_Not_found             = 91,//流没有找到
	NetBaseNetType_HttpClient_Record_mp4            = 92,//完成一段录像
	NetBaseNetType_HttpClient_on_stream_arrive      = 93,//码流到达
	NetBaseNetType_HttpClient_on_stream_disconnect  = 94,//连接已断开
	NetBaseNetType_HttpClient_on_record_ts          = 95,//TS切片完成
	NetBaseNetType_HttpClient_on_stream_not_arrive  = 96,//码流没有到达
	NetBaseNetType_HttpClient_Record_Progress       = 97,//录像下载进度
 
	NetBaseNetType_SnapPicture_JPEG               =100,//抓拍为JPG 
	NetBaseNetType_SnapPicture_PNG                =101,//抓拍为PNG

	NetBaseNetType_NetServerHTTP                  =110,//http 操作请求 

	NetBaseNetType_HttpClient_ServerStarted       = 120,//服务器启动
	NetBaseNetType_HttpClient_ServerKeepalive     = 121,//服务器心跳
	NetBaseNetType_HttpClient_DeleteRecordMp4     = 122,//覆盖录像文件
	NetBaseNetType_HttpClient_on_play              = 123,//播放视频事件通知
	NetBaseNetType_HttpClient_on_publish           = 124,//码流接入通知 

};

#define   MediaServerVerson                 "ABLMediaServer-6.3.5(2023-03-30)"
#define   RtspServerPublic                  "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD，GET_PARAMETER"
#define   RecordFileReplaySplitter          "__ReplayFMP4RecordFile__"  //实况、录像区分的标志字符串，用于区分实况，放置在url中。

#define  MaxNetDataCacheBufferLength        1024*1024*3   //网络缓存区域字节大小
#define  MaxLiveingVideoFifoBufferLength    1024*1024*3   //最大的视频缓存 
#define  MaxLiveingAudioFifoBufferLength    1024*512      //最大的音频缓存 
#define  BaseRecvRtpSSRCNumber              0xFFFFFFFFFF  //用于接收TS接收时 加上 ssrc 的值作为关键字 Key
#define  IDRFrameMaxBufferLength            1024*1024*3   //IDR帧最大缓存区域字节大小
#define  MaxClientConnectTimerout           15*1000       //连接服务器最大超时时长 10 秒 

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
	char  isRtspRecordURL[128];//是否rtsp录像回放 
	char  enable_mp4[64];//是否录像
	char  enable_hls[64];//是否开启hls
	char  convertOutWidth[64];//转码输出宽
	char  convertOutHeight[64];//转码输出高 
	char  H264DecodeEncode_enable[64];//H264是否解码再编码 
	char  RtpPayloadDataType[64]; 
	char  disableVideo[16];//过滤掉视频 1 过滤掉视频 ，0 不过滤视频 ，默认 0 

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
		memset(RtpPayloadDataType, 0x00, sizeof(RtpPayloadDataType));
		memset(disableVideo, 0x00, sizeof(disableVideo));
	}
};

//所以删除请求结构
struct delRequestStruct
{
	char  secret[256];//api操作密码 
	char  key[128];//key
 
	delRequestStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
	}
};

//代理推流转发参数结构
struct addPushProxyStruct
{
	char  secret[256];//api操作密码 
	char  vhost[64];//添加流的虚拟主机
	char  app[128];//添加流的应用名
	char  stream[128];//添加流的id 
	char  url[384];//拉流地址 ，支持 rtsp\rtmp\http-flv \ hls 

	addPushProxyStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(url, 0x00, sizeof(url));
	}
};

//创建GB28181接收码流
struct openRtpServerStruct
{
	char   secret[256];//api操作密码 
	char   vhost[64];//添加流的虚拟主机
	char   app[128];//添加流的应用名
	char   stream_id[128];//添加流的id 
	char   port[64] ;//GB2818端口
	char   enable_tcp[16]; //0 UDP，1 TCP 
	char   payload[64]; //payload rtp 打包的payload 
	char   enable_mp4[64];//是否录像
	char   enable_hls[64];//是否开启hls
	char  convertOutWidth[64];//转码输出宽
	char  convertOutHeight[64];//转码输出高 
	char  H264DecodeEncode_enable[64];//H264是否解码再编码 
	char  RtpPayloadDataType[16];//国标接入rtp负载的数据类型 【 1 rtp + PS 】 , 【 2 rtp + ES 】 ,【3 ，rtp + XHB 一家公司的私有格式】
	char  disableVideo[16];//过滤掉视频 1 过滤掉视频 ，0 不过滤视频 ，默认 0 

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
		memset(RtpPayloadDataType, 0x00, sizeof(RtpPayloadDataType));
		memset(disableVideo, 0x00, sizeof(disableVideo));
	}
};

//创建GB28181码流发送
struct startSendRtpStruct
{
	char   secret[256];//api操作密码 
	char   vhost[64];//添加流的虚拟主机
	char   app[128];//添加流的应用名
	char   stream[128];//添加流的id 
	char   ssrc[128];//ssrc
	char   src_port[64];//发送源绑定的端口号，0 自动产生一个端口，大于0 则绑定用户指定的端口
	char   dst_url[512];//
	char   dst_port[64];//GB2818端口
	char   is_udp[16]; //0 UDP，1 TCP 
	char   payload[24]; //payload rtp 打包的payload 
	char   RtpPayloadDataType[64];//打包格式 1　PS、２　ES、３　XHB

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
		memset(RtpPayloadDataType, 0x00, sizeof(RtpPayloadDataType));
	}
};

//获取列表结构
struct getMediaListStruct 
{
	char   secret[256];//api操作密码 
	char   vhost[256];//添加流的虚拟主机
	char   app[256];//添加流的应用名
	char   stream[256];//添加流的id 

	getMediaListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
	}
};

//获取往外发送媒体列表结构
struct getOutListStruct
{
	char   secret[256];//api操作密码 
	char   outType[128];//媒体源类型 rtsp ,rtmp ,flv ,hls ,gb28181 ,webrtc  
	getOutListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(outType, 0x00, sizeof(outType));
	}
};

//获取系统配置
struct getServerConfigStruct
{
	char   secret[256];//api操作密码 
	getServerConfigStruct()
	{
		memset(secret, 0x00, sizeof(secret));
 	}
};

//关闭源流结构
struct closeStreamsStruct
{
	char    secret[256];//api操作密码
	char    schema[256];
	char   	vhost[256];
	char   	app[256];
	char   	stream[256];
	int	    force ; //1 强制关闭，不管是否有人观看 ，0 有人能观看时，不关闭
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

//开始、停止录像
struct startStopRecordStruct
{
	char  secret[256];//api操作密码 
	char  vhost[64];//添加流的虚拟主机
	char  app[128];//添加流的应用名
	char  stream[128];//添加流的id 

	startStopRecordStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
 	}
};

//查询录像文件列表
struct queryRecordListStruct
{
	char  secret[256];//api操作密码 
	char  vhost[64];//添加流的虚拟主机
	char  app[128];//添加流的应用名
	char  stream[128];//添加流的id 
	char  starttime[128];//开始时间
	char  endtime[128];//开始时间

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

//请求抓拍
struct getSnapStruct
{
	char  secret[256];//api操作密码 
	char  vhost[64];//添加流的虚拟主机
	char  app[128];//添加流的应用名
	char  stream[128];//添加流的id 
	char  timeout_sec[128];//抓拍图片超时 ，单位 秒

	getSnapStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(timeout_sec, 0x00, sizeof(timeout_sec));
	}
};

//查询图片文件列表
struct queryPictureListStruct
{
	char  secret[256];//api操作密码 
	char  vhost[64];//添加流的虚拟主机
	char  app[128];//添加流的应用名
	char  stream[128];//添加流的id 
	char  starttime[128];//开始时间 
	char  endtime[128];//开始时间  

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

//查询图片文件列表
struct controlStreamProxy
{
	char  secret[256];//api操作密码 
	char  key[64];//key
 	char  command[128];//命令 
	char  value[128];//值

	controlStreamProxy()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
		memset(command, 0x00, sizeof(command));
		memset(value, 0x00, sizeof(value));
 	}
};

//设置配置参数值
struct SetConfigParamValue
{
	char  secret[256];//api操作密码 
	char  key[128];//key
	char  value[128];//值

	SetConfigParamValue()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
 		memset(value, 0x00, sizeof(value));
	}
};

enum NetRevcBaseClientType
{
	NetRevcBaseClient_ServerAccept               = 1, //服务器端口接入 ，比如 554,8080,8088,8089,1935 等等端口accept进来的
	NetRevcBaseClient_addStreamProxy             = 2, //代理拉流方式
	NetRevcBaseClient_addPushStreamProxy         = 3, //代理推流方式
	NetRevcBaseClient_addStreamProxyControl      = 4, //控制代理拉流
	NetRevcBaseClient_addPushProxyControl        = 5, //控制代理推流
	NetRevcBaseClient__NetGB28181Proxy           = 6, //GB28181代理 
};

//http请求参数 
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

//http 操作返回码
enum HttpReponseIndexApiCode
{
	IndexApiCode_OK           = 0 ,     //操作正常
	IndexApiCode_ErrorRequest = -100,   //非法请求
	IndexApiCode_secretError  = -200,   //secret错误
	IndexApiCode_ParamError   = -300,   //参数错误
	IndexApiCode_KeyNotFound  = -400,   //Key 没有找到
	IndexApiCode_SqlError     = -500,   //Sql错误
	IndexApiCode_ConnectFail  = -600,   //连接失败
	IndexApiCode_RtspSDPError = -700,   //rtsp推流失败
	IndexApiCode_RtmpPushError = -800,  //rtmp推流失败
	IndexApiCode_BindPortError = -900,  //网络绑定失败
	IndexApiCode_ConnectTimeout = -1000, //网络连接超时
	IndexApiCode_HttpJsonError = -1001, //http 请求 json参数非法
	IndexApiCode_HttpProtocolError = -1002, //http 请求 协议错误
	IndexApiCode_MediaSourceNotFound = -1003, //可以媒体源没有找到 
	IndexApiCode_RequestProcessFailed  = -1004 ,//执行失败 
	IndexApiCode_RequestFileNotFound = -1005,//文件没有找到
	IndexApiCode_ContentTypeNotSupported = -1006,//Content-Type 类型不支持
	IndexApiCode_OverMaxSameTimeSnap = -1007,//超过最大抓拍数量
	IndexApiCode_AppStreamAlreadyUsed = -1008,//app/sream 已经使用中
	IndexApiCode_PortAlreadyUsed      = -1009,//port 已经使用中
	IndexApiCode_SSRClreadyUsed       = -1010,//SSRC 已经使用中
	IndexApiCode_TranscodingVideoFilterNotEnable = -1011,//转码、水印功能尚未开启
	IndexApiCode_TranscodingVideoFilterTakeEffect = -1012,//转码、水印功能尚未生效
	IndexApiCode_RecvRtmpFailed = -1100, //获取rtmp码流失败
	IndexApiCode_AppStreamHaveUsing = -1200, //app,stream 正在使用，但是码流尚未到达 
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

//国标代理类型 
enum NetGB28181ProxyType
{
	NetGB28181ProxyType_RecvStream = 1, //国标收流
	NetGB28181ProxyType_PushStream = 2 ,//国标推流
};

//accept 过来的网络类型
enum NetServerHandleType
{
	NetServerHandleType_GB28181RecvStream = 10, //tcp方式接收国标码流
};

//参数
struct NetServerHandleParam
{
	int      nNetServerHandleType;
	uint64_t hParent; //依附的对象 
	char     szMediaSource[256];
	int      nAcceptNumber; //连接次数 
	NetServerHandleParam()
	{
		nNetServerHandleType = 0;
		hParent = 0;
		memset(szMediaSource, 0x00, sizeof(szMediaSource));
		nAcceptNumber = 0 ;
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

struct _rtsp_header
{
	unsigned char  head;
	unsigned char  chan;
	unsigned short Length;
};

//媒体源类型
enum MediaSourceType
{
	MediaSourceType_LiveMedia = 0,  //实况播放
	MediaSourceType_ReplayMedia = 1, //录像点播
};

//rtsp 拉流时 rtp 负载的数据类型 
enum RtspRtpPayloadType
{
	RtspRtpPayloadType_Unknow = 0,  //未知
	RtspRtpPayloadType_ES     = 1, //rtp负载ES 
	RtspRtpPayloadType_PS     = 2, //rtp负载PS 
};

//图片类型
enum HttpImageType
{
	HttpImageType_jpeg = 1,  //jpeg
    HttpImageType_png = 2, //png
};
bool          ABLDeleteFile(char* szFileName);

//消息通知结构 
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

#ifdef USE_BOOST
void          Sleep(int mMicroSecond);
#endif

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

#ifdef USE_BOOST
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





#else

#include "ABLString.h"
#include <cctype>

#endif

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
