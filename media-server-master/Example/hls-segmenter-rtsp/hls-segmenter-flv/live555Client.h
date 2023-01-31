/*
  项目： 实现通用的RTSP流媒体数据读取
  Author 罗家兄弟
  Date   2020-04-22
  QQ     79941308 
  E-mail 79941308@qq.com
*/
 
#ifndef _LIVE555CLIENT_API_H
#define _LIVE555CLIENT_API_H

#ifdef LIVE555CLIENT_EXPORTS
#define LIVE555CLIENT_API __declspec(dllexport)
#else
#define LIVE555CLIENT_API __declspec(dllimport)
#endif

#include <stdint.h>

#define  message_timeout              "ConnectTimeout" //连接超时
#define  message_connError            "ConnectError" //连接错误
#define  message_ConnectBreak         "ConnectBreaked" //连接已经断开
#define  message_NoAudioVideoStream   "NoAudioVideoStream" //没视频，音频流
#define  message_Success              "ConnectSuccess" //连接成功
#define  message_NotSupportProtect    "NotSupportProtect" //不支持的协议
//RTSP消息类型 
enum XHRtspDataType
{
	XHRtspDataType_Message = 0,  //消息提示
	XHRtspDataType_Video = 1,   //视频数据
	XHRtspDataType_Audio = 2    //音频数据
};

//url类型，是实况，还是录像 ,还是下载
enum XHRtspURLType
{
	XHRtspURLType_Liveing = 0,  //实况播放
	XHRtspURLType_RecordPlay = 1,   //录像播放
	XHRtspURLType_RecordDownload = 2    //录像下载
};

/*参数说明
   int             nRtspChan       rtsp通道号，0 ~ 255
   char*           dataType        返回的数据类型，可能为 XHRtspDataType, XHRtspDataType_Message、  XHRtspDataType_Video 、 XHRtspDataType_Audio  
   char*           codeName        可能为 M4V-ES (mpeg4),h264,"mp3","g711",error
   unsigned char * pAVData         音频，或者视频数据
   int             nAVDataLength   音频，视频数据长度
   int64_t         timeValue       时间戳
   void*           pCustomerData   客户传入的指针
*/
typedef void (CALLBACK* LIVE555RTSP_AudioVideo)  (int nRtspChan,int RtspDataType,char* codeName,unsigned char * pAVData,int nAVDataLength,int64_t timeValue,void* pCustomerData)  ;

//音频，视频回调函数
void CALLBACK live555RTSP_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);
 	
 	
//网络初始化
/*
   注意：
    如果连接调用 live555Client_ConnectCallBack ，那么 live555Client_Init 初始化参数设置为NULL
	如果连接调用 live555Client_Connect ，那么live555Client_Init 初始化参数就不能为NULL
*/
LIVE555CLIENT_API BOOL live555Client_Init(LIVE555RTSP_AudioVideo callbackFunc) ;

/*
功能：连接某路流媒体
 参数：
      char* szURL         流媒体url
      BOOL  bTCPFlag      连接是否使用TCP
	  void* pCustomerPtr  用户指针

      int& nOutChan        输出通道号，相当于连接句柄
*/
LIVE555CLIENT_API BOOL live555Client_Connect(char* szURL, BOOL bTCPFlag, void* pCustomerPtr, int& nOutChan);

/*
功能：连接某路流媒体
参数：
char* szURL         流媒体url
BOOL  bTCPFlag      连接是否使用TCP
void* pCustomerPtr  用户指针
LIVE555RTSP_AudioVideo callbackFunc

int& nOutChan        输出通道号，相当于连接句柄
*/
LIVE555CLIENT_API BOOL live555Client_ConnectCallBack(char* szURL, XHRtspURLType nXHRtspURLType, BOOL bTCPFlag, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, int& nOutChan);

/*
功能：设置播放速度
  参数：
  int   nChan        通道号
  float fSpeed       播放速度 1 ，2，4，8，16 ，255
*/
LIVE555CLIENT_API BOOL live555Client_Speed(int nChan, float fSpeed);

/*
功能：拖动播放
参数：
int     nChan        通道号
int64_t timeStamp    拖动播放
*/
LIVE555CLIENT_API BOOL live555Client_Seek(int nChan, int64_t timeStamp);

/*
功能：暂停播放
参数：
int     nChan        通道号
*/
LIVE555CLIENT_API BOOL live555Client_Pause(int nChan);

/*
功能：  继续播放
参数：
int     nChan        通道号
*/
LIVE555CLIENT_API BOOL live555Client_Resume(int nChan);

/*
功能：  获取某路rtsp录像回放是否在处于断流状态 
参数：
int     nChan        通道号
返回值：
     TRUE   网络接收码流正常
	 FALSE  网络接收码流不正常，已经断流 
*/
LIVE555CLIENT_API BOOL live555Client_GetNetStreamStatus(int nChan);

/*
  功能：断开某路流媒体
    参数：
    int   nChan        通道号
*/ 
LIVE555CLIENT_API BOOL live555Client_Disconnect(int nChan) ;
 
//网络销毁
LIVE555CLIENT_API BOOL live555Client_Cleanup() ;

#endif
 