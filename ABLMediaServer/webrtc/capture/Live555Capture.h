#pragma once
#include "BaseStreamCapture.h"
#include "live555/src/rtspconnectionclient.h"
#include "ffmpeg/ffmpeg_enc_decoder.hpp"
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
class Live555Capture :public BaseStreamCapture,public RTSPConnection::Callback//,public Environment
{
public:
	Live555Capture(std::string url);
	~Live555Capture();
public:
	std::string GetAudioConfig();
public://  for BaseStreamCapture
	virtual void Init(const std::map<std::string, std::string>& opts) override;
	virtual bool Start() override;		 
	virtual void RegisterVideoCallBack(VideoCllBack videoCallback) override;
	virtual void RegisterAudioCallBack(AudioCallBack  audioCallback) override;
	virtual void Stop() override;
private:// for RTSPConnection::Callback
	virtual bool onNewSession(const char* id, const char* media, const char* codec, const char* sdp) override;
	virtual bool onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) override;
	virtual void onError(RTSPConnection& rtsp_connection, const char* msg) override;
	virtual void onConnectionTimeout(RTSPConnection& rtsp_connection) override;
	virtual void onDataTimeout(RTSPConnection& rtsp_connection) override;
private:	
	//void CaptureThread();
	RTSPConnection* CreateRTSPConnectionObject();
private:	
	Environment* m_environment=nullptr;
	std::unique_ptr<RTSPConnection> m_ptr_connetion = nullptr;		
	std::shared_ptr<std::thread> m_capturethread =nullptr;		
	std::map<std::string, std::string> m_codec;
	std::vector<uint8_t>               m_cfg;	
	std::string m_url;
	char m_environment_stop;
	bool m_bRestart = false;
	//video
	int m_width =-1;
	int m_height = -1;	
	VideoCllBack m_h264CallBack = nullptr;
	//std::shared_ptr<FFMPEG_ENC_DECODER> m_ptr_vdecoder = nullptr;
	//for Audio
	int m_channel;
	int m_pool_deep = 4;
	int m_sample_rate = -1;
	AudioCallBack m_aacCallBack = nullptr;
	//std::shared_ptr<FFMPEG_ENC_DECODER> m_ptr_adecoder = nullptr;
	std::string m_audio_config;
};

