#pragma once
#include "AudioCapture.h"

#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <iostream>
#include <queue>
#include <mutex>

#include "../inc/rtspconnectionclient.h"
#include "../inc/environment.h"
#include "../encoder/aac_endecoder.hpp"
#include "pc/local_audio_source.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"



class RtspAudioCapture :public AudioCapture, public RTSPConnection::Callback
{
public:
	RtspAudioCapture(const std::string& uri, const std::map<std::string, std::string>& opts = {},  bool wait=true);
	
	~RtspAudioCapture();

	std::string GetAudioConfig();

public://  for AudioCapture
	virtual int Init(int nSampleRate, int nChannel, int nBitsPerSample)override;

	virtual int Init(const std::map<std::string, std::string>& opts) override;

	virtual int Start() override;	

	virtual void Stop() override;

	virtual void RegisterPcmCallback(std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallback) override ;

	virtual void RegisterAacCallback(std::function<void(uint8_t* aac_raw, int file_size, int64_t nTimeStamp)> aacCallBack) override;

	virtual void Destroy() override;	  
	
private:// for RTSPConnection 
	
	virtual bool onNewSession(const char* id, const char* media, const char* codec, const char* sdp) override;
	
	virtual bool onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) override;
	
	virtual void onError(RTSPConnection& rtsp_connection, const char* msg) override;
	
	virtual void onConnectionTimeout(RTSPConnection& rtsp_connection) override;
	
	virtual void onDataTimeout(RTSPConnection& rtsp_connection) override;

	virtual void HangUp()override {};

	virtual void WakeUp()override {};

private:
	void CaptureThread();




private:
	char m_stop=0;
	Environment m_env;

private:


	RTSPConnection* m_pConnetion = nullptr;

	rtc::scoped_refptr<webrtc::AudioDecoderFactory> m_factory;

	std::unique_ptr<webrtc::AudioDecoder> m_decoder;

	std::unique_ptr<AACDecoder> m_aacdecoder;

	int m_freq;
	int m_channel;
	std::queue<uint16_t> m_buffer;
	std::list<webrtc::AudioTrackSinkInterface*> m_sinks;
	std::mutex m_sink_lock;



	bool m_wait;
	int64_t m_previmagets;
	int64_t m_prevts;
	std::string m_strConfig;


private:	

	std::shared_ptr<std::thread> m_capturethread =nullptr;	
	std::string m_url;
	std::map<std::string, std::string> m_codec;
	

////for Audio
	int m_pool_deep;
	int m_sample_rate = -1;	
	PcmCallBack m_pcmCallback = nullptr;
	AacCallBack m_aacCallBack = nullptr;
	std::string m_audio_config;
	bool m_bRestart = false;
};

