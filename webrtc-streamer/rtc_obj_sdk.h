/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef _RTC_OBJ_SDK_H_
#define _RTC_OBJ_SDK_H_

#include <map>
#include <list>
#include <vector>
#include <string>
#include <functional>

#include "pch.h"
#include "../webrtc-streamer/WSStreamConfig.h"
#include "../webrtc-streamer/RtmpPushMgr.h"



typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight)> LocalFrameCallBackFunc;

typedef std::function<void(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight)> RemoteFrameCallBackFunc;

class WEBRTCSDK_EXPORTSIMPL MainWndCallback {
public:
	MainWndCallback()
	{

	}
	virtual ~MainWndCallback() {}

	

	//����ͷyuv���ݻص�
	virtual void onLocalFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;
	
	//Զ����Ƶyuv���ݻص�
	virtual void onRemoteFrame(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;

	//������˷����ݻص�
	virtual void onLocalAudio(const void* audio_data ,int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//�Է���˷����ݻص�
	virtual void onRemoteAudio(std::string userID, const void* audio_data, int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//Զ�̷���ֹͣ������Ƶ��
	virtual  void onRemoveTrack(std::string userID) = 0;

	//websocket����Ϣ h�ص�
	virtual  void onWsMessage(std::string strMessage) = 0;

	//log��Ϣ�ص�
	virtual  void onLogMessage(std::string strLog) = 0;

	//������Ϣ�ص�
	virtual void onError(int nErrorCode, std::string msg) = 0;


	virtual void OnDataChannel(std::string userID, const char* data, uint32_t len, bool binary) = 0;
};



class WEBRTCSDK_EXPORTSIMPL WebRtcEndpoint {
public:
	//��ʼ��
	virtual bool Init(StreamConfig config) = 0;

	virtual bool Release() = 0;

	/**
	* ���뷿�䣬
	* ֤�����䴴���ɹ������Զ���ʼ������
	*/
	virtual void joinRoom(std::string  strUrl, std::string strJson, std::string  strUserid) = 0;

	/**
	* ����websocket��Ϣ
	*/
	virtual bool sendWsMessage(std::string strMessage) = 0;

	/**
	* ����DataChannel��Ϣ
	*/
	virtual bool sendDataChannelMessage(const char* data, uint32_t len, bool binary) = 0;

	
	//�޸�����Դ
	virtual void changeVideoInput(StreamConfig config)=0;

	//�뿪����
	virtual void leaveRoom() = 0;

	//�Ƿ��ֹ����ͷ��׽����
	virtual bool setVideoEnable(std::string  peerid,bool bEnable)=0;

	//�Ƿ����
	virtual bool setMicEnable(std::string  peerid, bool bEnable)=0;

	//������˷�����
	virtual bool setMicVolume(std::string  peerid, int  nVolume)=0;

	//��������
	virtual bool setAudioVolume(std::string  strUserid, int  nVolume)=0;

	//�Ƿ�������
	virtual bool setAudioEnable(std::string  strUserid, bool bEnable)=0;

	// ������ͷ�豸����ʾ
	virtual bool getCameraFrame(std::string  strDeviveName, LocalFrameCallBackFunc callbackfuc) = 0;

	//ֹͣ��ȡ����ͷ�Ļ���
	virtual bool stopCameraFrame(std::string  strDeviveName)=0;

	//������Ƶ����
	virtual int32_t sendRecordedBuffer(int8_t* audio_data,
		uint32_t data_len,
		int bits_per_sample,
		int sample_rate,
		size_t number_of_channels) = 0;

	//���ò����豸
	virtual bool setPlayoutDevice(std::string  strPlayoutDevice) =0;

	virtual std::string  getStats()=0;

protected:
	virtual ~WebRtcEndpoint() {}
};

//����
WEBRTCSDK_EXPORTSIMPL WebRtcEndpoint* CreateLinkMicManager(MainWndCallback*);

WEBRTCSDK_EXPORTSIMPL void ReleaseLinkMicManager(WebRtcEndpoint*);

//����
WEBRTCSDK_EXPORTSIMPL RtmpPushMgr* CreateRtmpPushManager(RtmpPushMgrListen*);

WEBRTCSDK_EXPORTSIMPL void ReleaseRtmpPushManager(RtmpPushMgr*);


//��ȡӲ������ͷ�б�
WEBRTCSDK_EXPORTSIMPL std::list<std::string> getVideoDeviceList();

//��ȡ�����豸�б�
WEBRTCSDK_EXPORTSIMPL std::list<std::string> getPlayoutDeviceList();

//��ȡ��˷��豸�б�
WEBRTCSDK_EXPORTSIMPL std::list<std::string> getRecordingDeviceList();

//��ȡ�����б�
WEBRTCSDK_EXPORTSIMPL std::list<std::string> GetVideoSourceList();

//��ȡ�����б�
WEBRTCSDK_EXPORTSIMPL std::map<int, std::string> GetVideoSourceMap();

WEBRTCSDK_EXPORTSIMPL int ws_I420ToARGB(const unsigned char* src_y,
	int src_stride_y,
	const unsigned char* src_u,
	int src_stride_u,
	const unsigned char* src_v,
	int src_stride_v,
	unsigned char* dst_argb,
	int dst_stride_argb,
	int width,
	int height);
WEBRTCSDK_EXPORTSIMPL int ws_I420Copy(const unsigned char* src_y,
	int src_stride_y,
	const unsigned char* src_u,
	int src_stride_u,
	const unsigned char* src_v,
	int src_stride_v,
	unsigned char* dst_y,
	int dst_stride_y,
	unsigned char* dst_u,
	int dst_stride_u,
	unsigned char* dst_v,
	int dst_stride_v,
	int width,
	int height);
WEBRTCSDK_EXPORTSIMPL int ws_I420Scale(const unsigned char* src_y,
	int src_stride_y,
	const unsigned char* src_u,
	int src_stride_u,
	const unsigned char* src_v,
	int src_stride_v,
	int src_width,
	int src_height,
	unsigned char* dst_y,
	int dst_stride_y,
	unsigned char* dst_u,
	int dst_stride_u,
	unsigned char* dst_v,
	int dst_stride_v,
	int dst_width,
	int dst_height);

WEBRTCSDK_EXPORTSIMPL int ws_I420Rotate(const unsigned char* src_y,
	int src_stride_y,
	const unsigned char* src_u,
	int src_stride_u,
	const unsigned char* src_v,
	int src_stride_v,
	unsigned char* dst_y,
	int dst_stride_y,
	unsigned char* dst_u,
	int dst_stride_u,
	unsigned char* dst_v,
	int dst_stride_v,
	int dst_width,
	int dst_height);

WEBRTCSDK_EXPORTSIMPL int64_t TimeMillis();







#endif  // _RTC_OBJ_SDK_H_
