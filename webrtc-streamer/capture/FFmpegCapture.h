#pragma once
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <list>
#include <atomic>
#include <vector>
#include <memory>
#include "BaseStreamCapture.h"
#include "../ffmpeg/ffmpeg.h"
#include "../ffmpeg/ff_capture.h"

class FFmpegCapture :public BaseStreamCapture
{
public:
	FFmpegCapture(std::string url);
	~FFmpegCapture();
public:
	virtual bool Start() override;	
	virtual void Init(const std::map<std::string, std::string>& opts) override;
	virtual void Stop() override;//½áÊø	
	virtual void RegisterVideoCallBack(VideoCllBack videoCallback) override;
	virtual void RegisterAudioCallBack(AudioCallBack audioCallback) override;
private:
	void OnVideo(uint8_t* raw_data, const char* codecid, int raw_len, bool bKey, int nWidth, int nHeight, int64_t nTimeStamp);
	void OnAudio(uint8_t* raw_data, const char* codecid, int raw_len, int channels, int sample_rate, int bytes, int64_t nTimeStamp);
private:
	std::string m_url;		
	std::unique_ptr<FF_Capturer> m_ptr_capture;
	VideoCllBack  m_callback_video;
	AudioCallBack m_callback_audio;
};

