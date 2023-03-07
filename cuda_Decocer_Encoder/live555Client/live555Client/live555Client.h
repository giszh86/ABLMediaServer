/*
��Ŀ�� ʵ��ͨ�õ�RTSP��ý�����ݶ�ȡ   
Author �޼��ֵ�
Date   2021-08-31
QQ     79941308
E-mail 79941308@qq.com
*/

#ifndef _LIVE555CLIENT_API_H
#define _LIVE555CLIENT_API_H

#include <stdint.h>

#define  message_timeout              "ConnectTimeout" //���ӳ�ʱ
#define  message_connError            "ConnectError" //���Ӵ���
#define  message_ConnectBreak         "ConnectBreaked" //�����Ѿ��Ͽ�
#define  message_NoAudioVideoStream   "NoAudioVideoStream" //û��Ƶ����Ƶ��
#define  message_Success              "ConnectSuccess" //���ӳɹ�
#define  message_NotSupportProtect    "NotSupportProtect" //��֧�ֵ�Э��

//RTSP��Ϣ���� 
enum XHRtspDataType
{
	XHRtspDataType_Message = 0,  //��Ϣ��ʾ
	XHRtspDataType_Video = 1,   //��Ƶ����
	XHRtspDataType_Audio = 2    //��Ƶ����
};

//url���ͣ���ʵ��������¼�� ,��������
enum XHRtspURLType
{
	XHRtspURLType_Liveing = 0,  //ʵ������
	XHRtspURLType_RecordPlay = 1,   //¼�񲥷�
	XHRtspURLType_RecordDownload = 2    //¼������
};

/*����˵��
int             nRtspChan       rtspͨ���ţ�0 ~ 255
char*           dataType        ���ص��������ͣ�����Ϊ XHRtspDataType, XHRtspDataType_Message��  XHRtspDataType_Video �� XHRtspDataType_Audio
char*           codeName        ����Ϊ M4V-ES (mpeg4),h264,"mp3","g711",error
unsigned char * pAVData         ��Ƶ��������Ƶ����
int             nAVDataLength   ��Ƶ����Ƶ���ݳ���
int64_t         timeValue       ʱ���
void*           pCustomerData   �ͻ������ָ��
*/
typedef void(*LIVE555RTSP_AudioVideo)  (int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);

//��Ƶ����Ƶ�ص�����
void live555RTSP_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);


//�����ʼ��
/*
ע�⣺
������ӵ��� live555Client_ConnectCallBack ����ô live555Client_Init ��ʼ����������ΪNULL
������ӵ��� live555Client_Connect ����ôlive555Client_Init ��ʼ�������Ͳ���ΪNULL
*/
extern "C" __attribute__((visibility("default"))) bool live555Client_Init(LIVE555RTSP_AudioVideo callbackFunc);

/*
���ܣ�����ĳ·��ý��
������
char* szURL         ��ý��url
BOOL  bTCPFlag      �����Ƿ�ʹ��TCP
void* pCustomerPtr  �û�ָ��

int& nOutChan        ���ͨ���ţ��൱�����Ӿ��
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_Connect(char* szURL, bool bTCPFlag, void* pCustomerPtr, int& nOutChan);

/*
���ܣ�����ĳ·��ý��
������
char* szURL         ��ý��url
BOOL  bTCPFlag      �����Ƿ�ʹ��TCP
void* pCustomerPtr  �û�ָ��
LIVE555RTSP_AudioVideo callbackFunc

int& nOutChan        ���ͨ���ţ��൱�����Ӿ��
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_ConnectCallBack(char* szURL, XHRtspURLType nXHRtspURLType, bool bTCPFlag, void* pCustomerPtr, LIVE555RTSP_AudioVideo callbackFunc, int& nOutChan);

/*
���ܣ����ò����ٶ�
������
int   nChan        ͨ����
float fSpeed       �����ٶ� 1 ��2��4��8��16 ��255
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_Speed(int nChan, float fSpeed);

/*
���ܣ��϶�����
������
int     nChan        ͨ����
int64_t timeStamp    �϶�����
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_Seek(int nChan, int64_t timeStamp);

/*
���ܣ���ͣ����
������
int     nChan        ͨ����
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_Pause(int nChan);

/*
���ܣ�  ��������
������
int     nChan        ͨ����
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_Resume(int nChan);

/*
���ܣ�  ��ȡĳ·rtsp¼��ط��Ƿ��ڴ��ڶ���״̬
������
int     nChan        ͨ����
����ֵ��
TRUE   ���������������
FALSE  ��������������������Ѿ�����
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_GetNetStreamStatus(int nChan);

/*
���ܣ��Ͽ�ĳ·��ý��
������
int   nChan        ͨ����
*/
extern "C"   __attribute__((visibility("default")))  bool live555Client_Disconnect(int nChan);

//��������
extern "C"   __attribute__((visibility("default")))  bool live555Client_Cleanup();

#endif
