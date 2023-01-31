/*
	��Ŀ��  ��װlive555 �� rtsp�ͻ���
	Author �޼��ֵ�
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

#define  LiveRtsp_message_timeout              "ConnectTimeout" //���ӳ�ʱ
#define  LiveRtsp_message_connError            "ConnectError" //���Ӵ���
#define  LiveRtsp_message_ConnectBreak         "ConnectBreaked" //�����Ѿ��Ͽ�
#define  LiveRtsp_message_NoAudioVideoStream   "NoAudioVideoStream" //û��Ƶ����Ƶ��
#define  LiveRtsp_message_Success              "ConnectSuccess" //���ӳɹ�
#define  LiveRtsp_message_NotSupportProtect    "NotSupportProtect" //��֧�ֵ�Э��

//RTSP��Ϣ���� 
enum LiveRtspDataType
{
	LiveRtspDataType_Message = 0,  //��Ϣ��ʾ
	LiveRtspDataType_Video = 1,   //��Ƶ����
	LiveRtspDataType_Audio = 2    //��Ƶ����
};

//url���ͣ���ʵ��������¼�� ,��������
enum LiveRtspURLType
{
	LiveRtspURLType_Liveing = 0,  //ʵ������
	LiveRtspURLType_RecordPlay = 1,   //¼�񲥷�
	LiveRtspURLType_RecordDownload = 2    //¼������
};

/*����˵��
int             nRtspChan       rtspͨ���ţ�0 ~ 255
char*           dataType        ���ص��������ͣ�����Ϊ XHRtspDataType, LiveRtspDataType_Message��  LiveRtspDataType_Video �� LiveRtspDataType_Audio
char*           codeName        ����Ϊ M4V-ES (mpeg4),h264,"mp3","g711",error
unsigned char * pAVData         ��Ƶ��������Ƶ����
int             nAVDataLength   ��Ƶ����Ƶ���ݳ���
int64_t         timeValue       ʱ���
void*           pCustomerData   �ͻ������ָ��
*/
typedef void (CALLBACK* LiveRtspCB_AudioVideo)  (int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);

//��Ƶ����Ƶ�ص�����
void CALLBACK LiveRtsp_AudioVideo(int nRtspChan, int RtspDataType, char* codeName, unsigned char * pAVData, int nAVDataLength, int64_t timeValue, void* pCustomerData);


//�����ʼ��
/*
ע�⣺
������ӵ��� LiveRtspClient_ConnectCallBack ����ô LiveRtspClient_Init ��ʼ����������ΪNULL
������ӵ��� LiveRtspClient_Connect ����ôLiveRtspClient_Init ��ʼ�������Ͳ���ΪNULL
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Init(LiveRtspCB_AudioVideo callbackFunc);

/*
���ܣ�����ĳ·��ý��
������
char* szURL         ��ý��url
BOOL  bTCPFlag      �����Ƿ�ʹ��TCP
void* pCustomerPtr  �û�ָ��

int& nOutChan        ���ͨ���ţ��൱�����Ӿ��
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Connect(char* szURL, BOOL bTCPFlag, void* pCustomerPtr, int& nOutChan);

/*
���ܣ�����ĳ·��ý��
������
char* szURL         ��ý��url
BOOL  bTCPFlag      �����Ƿ�ʹ��TCP
void* pCustomerPtr  �û�ָ��
LIVE555RTSP_AudioVideo callbackFunc

int& nOutChan        ���ͨ���ţ��൱�����Ӿ��
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_ConnectCallBack(char* szURL, LiveRtspURLType nXHRtspURLType, BOOL bTCPFlag, void* pCustomerPtr, LiveRtspCB_AudioVideo callbackFunc, int& nOutChan);

/*
���ܣ����ò����ٶ�
������
int   nChan        ͨ����
float fSpeed       �����ٶ� 1 ��2��4��8��16 ��255
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Speed(int nChan, float fSpeed);

/*
���ܣ��϶�����
������
int     nChan        ͨ����
int64_t timeStamp    �϶�����
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Seek(int nChan, int64_t timeStamp);

/*
���ܣ���ͣ����
������
int     nChan        ͨ����
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Pause(int nChan);

/*
���ܣ�  ��������
������
int     nChan        ͨ����
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Resume(int nChan);

/*
���ܣ�  ��ȡĳ·rtsp¼��ط��Ƿ��ڴ��ڶ���״̬
������
int     nChan        ͨ����
����ֵ��
TRUE   ���������������
FALSE  ��������������������Ѿ�����
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_GetNetStreamStatus(int nChan);

/*
���ܣ��Ͽ�ĳ·��ý��
������
int   nChan        ͨ����
*/
LIVERTSPCLIENT_API BOOL LiveRtspClient_Disconnect(int nChan);

//��������
LIVERTSPCLIENT_API BOOL LiveRtspClient_Cleanup();

#endif
