/*
	项目：  封装live555 的 rtsp客户端
	Author 罗家兄弟
	Date   2021-04-25
	QQ     79941308
	E-mail 79941308@qq.com
*/

#ifndef _LIVERTSPCLIENT_API_H
#define _LIVERTSPCLIENT_API_H

#ifdef LIVERTSPCLIENT_EXPORTS
#define LIVERTSPCLIENT_API __declspec(dllexport)
#else
#define LIVERTSPCLIENT_API __declspec(dllimport)
#endif

#include <stdint.h>
#include <Windows.h>

#define  LiveRtsp_message_timeout              "ConnectTimeout" //连接超时
#define  LiveRtsp_message_connError            "ConnectError" //连接错误
#define  LiveRtsp_message_ConnectBreak         "ConnectBreaked" //连接已经断开
#define  LiveRtsp_message_NoAudioVideoStream   "NoAudioVideoStream" //没视频，音频流
#define  LiveRtsp_message_Success              "ConnectSuccess" //连接成功
#define  LiveRtsp_message_NotSupportProtect    "NotSupportProtect" //不支持的协议

//RTSP消息类型 
enum LiveRtspDataType
{
	LiveRtspDataType_Message = 0,  //消息提示
	LiveRtspDataType_Video = 1,   //视频数据
	LiveRtspDataType_Audio = 2    //音频数据
};

//url类型，是实况，还是录像 ,还是下载
enum LiveRtspURLType
{
	LiveRtspURLType_Liveing = 0,  //实况播放
	LiveRtspURLType_RecordPlay = 1,   //录像播放
	LiveRtspURLType_RecordDownload = 2    //录像下载
};

/*参数说明
int             nRtspChan       rtsp通道号，0 ~ 255
char*           dataType        返回的数据类型，可能为 XHRtspDataType, LiveRtspDataType_Message、  LiveRtspDataType_Video 、 LiveRtspDataType_Audio
char*           codeName        可能为 M4V-ES (mpeg4),h264,"mp3","g711",error
unsigned char * pAVData         音频，或者视频数据
int             nAVDataLength   音频，视频数据长度
int64_t         timeValue       时间戳
void*           pCustomerData   客户传入的指针
*/
typedef void (CALLBACK* LiveRtspCB_AudioVideo)  (int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);

//音频，视频回调函数
void CALLBACK LiveRtsp_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);


//网络初始化
/*
注意：
如果连接调用 LiveRtspClient_ConnectCallBack ，那么 LiveRtspClient_Init 初始化参数设置为NULL
如果连接调用 LiveRtspClient_Connect ，那么LiveRtspClient_Init 初始化参数就不能为NULL
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Init(LiveRtspCB_AudioVideo callbackFunc);

/*
功能：连接某路流媒体
参数：
char* szURL         流媒体url
BOOL  bTCPFlag      连接是否使用TCP
void* pCustomerPtr  用户指针

int& nOutChan        输出通道号，相当于连接句柄
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Connect(char* szURL, BOOL bTCPFlag, void* pCustomerPtr, int& nOutChan);

/*
功能：连接某路流媒体
参数：
char* szURL         流媒体url
BOOL  bTCPFlag      连接是否使用TCP
void* pCustomerPtr  用户指针
LIVE555RTSP_AudioVideo callbackFunc

int& nOutChan        输出通道号，相当于连接句柄
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_ConnectCallBack(char* szURL, LiveRtspURLType nXHRtspURLType, BOOL bTCPFlag, void* pCustomerPtr, LiveRtspCB_AudioVideo callbackFunc, int& nOutChan);

/*
功能：设置播放速度
参数：
int   nChan        通道号
float fSpeed       播放速度 1 ，2，4，8，16 ，255
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Speed(int nChan, float fSpeed);

/*
功能：拖动播放
参数：
int     nChan        通道号
int64_t timeStamp    拖动播放
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Seek(int nChan, int64_t timeStamp);

/*
功能：暂停播放
参数：
int     nChan        通道号
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Pause(int nChan);

/*
功能：  继续播放
参数：
int     nChan        通道号
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Resume(int nChan);

/*
功能：  获取某路rtsp录像回放是否在处于断流状态
参数：
int     nChan        通道号
返回值：
TRUE   网络接收码流正常
FALSE  网络接收码流不正常，已经断流
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_GetNetStreamStatus(int nChan);

/*
功能：断开某路流媒体
参数：
int   nChan        通道号
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Disconnect(int nChan);

//网络销毁
LIVERTSPCLIENT_API BOOL LiveRtspClient_Cleanup();

#endif
