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

	

	//摄像头yuv数据回调
	virtual void onLocalFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;
	
	//远程视频yuv数据回调
	virtual void onRemoteFrame(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;

	//本地麦克风数据回调
	virtual void onLocalAudio(const void* audio_data ,int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//对方麦克风数据回调
	virtual void onRemoteAudio(std::string userID, const void* audio_data, int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//远程房间停止发送视频流
	virtual  void onRemoveTrack(std::string userID) = 0;

	//websocket的消息 h回调
	virtual  void onWsMessage(std::string strMessage) = 0;

	//log消息回调
	virtual  void onLogMessage(std::string strLog) = 0;

	//错误信息回调
	virtual void onError(int nErrorCode, std::string msg) = 0;


	virtual void OnDataChannel(std::string userID, const char* data, uint32_t len, bool binary) = 0;
};



class WEBRTCSDK_EXPORTSIMPL WebRtcEndpoint {
public:
	//初始化
	virtual bool Init(StreamConfig config) = 0;

	virtual bool Release() = 0;

	/**
	* 加入房间，
	* 证明房间创建成功，则自动开始推流；
	*/
	virtual void joinRoom(std::string  strUrl, std::string strJson, std::string  strUserid) = 0;

	/**
	* 发送websocket消息
	*/
	virtual bool sendWsMessage(std::string strMessage) = 0;

	/**
	* 发送DataChannel消息
	*/
	virtual bool sendDataChannelMessage(const char* data, uint32_t len, bool binary) = 0;

	
	//修改输入源
	virtual void changeVideoInput(StreamConfig config)=0;

	//离开房间
	virtual void leaveRoom() = 0;

	//是否禁止摄像头捕捉画面
	virtual bool setVideoEnable(std::string  peerid,bool bEnable)=0;

	//是否禁麦
	virtual bool setMicEnable(std::string  peerid, bool bEnable)=0;

	//调节麦克风音量
	virtual bool setMicVolume(std::string  peerid, int  nVolume)=0;

	//调节音量
	virtual bool setAudioVolume(std::string  strUserid, int  nVolume)=0;

	//是否有声音
	virtual bool setAudioEnable(std::string  strUserid, bool bEnable)=0;

	// 打开摄像头设备并显示
	virtual bool getCameraFrame(std::string  strDeviveName, LocalFrameCallBackFunc callbackfuc) = 0;

	//停止获取摄像头的画面
	virtual bool stopCameraFrame(std::string  strDeviveName)=0;

	//发送音频裸流
	virtual int32_t sendRecordedBuffer(int8_t* audio_data,
		uint32_t data_len,
		int bits_per_sample,
		int sample_rate,
		size_t number_of_channels) = 0;

	//设置播放设备
	virtual bool setPlayoutDevice(std::string  strPlayoutDevice) =0;

	virtual std::string  getStats()=0;

protected:
	virtual ~WebRtcEndpoint() {}
};

//创建
WEBRTCSDK_EXPORTSIMPL WebRtcEndpoint* CreateLinkMicManager(MainWndCallback*);

WEBRTCSDK_EXPORTSIMPL void ReleaseLinkMicManager(WebRtcEndpoint*);

//创建
WEBRTCSDK_EXPORTSIMPL RtmpPushMgr* CreateRtmpPushManager(RtmpPushMgrListen*);

WEBRTCSDK_EXPORTSIMPL void ReleaseRtmpPushManager(RtmpPushMgr*);


//获取硬件摄像头列表
WEBRTCSDK_EXPORTSIMPL std::list<std::string> getVideoDeviceList();

//获取播放设备列表
WEBRTCSDK_EXPORTSIMPL std::list<std::string> getPlayoutDeviceList();

//获取麦克风设备列表
WEBRTCSDK_EXPORTSIMPL std::list<std::string> getRecordingDeviceList();

//获取桌面列表
WEBRTCSDK_EXPORTSIMPL std::list<std::string> GetVideoSourceList();

//获取桌面列表
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
