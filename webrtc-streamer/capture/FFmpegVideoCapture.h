#pragma once

#include "VideoCapture.h"
#include <fstream>
#include <string.h>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>

#include "../3rd/libffmpeg/include/ffmpeg_headers.h"
#include "yolo_v2_class.hpp"

class FFmpegVideoCapture :public VideoCapture ,public MediaSourceEvent
{


public:
	FFmpegVideoCapture(const std::string& uri, const std::map<std::string, std::string>& opts = {});

	~FFmpegVideoCapture();


	/*VideoCapture callback*/
	virtual bool Start();

	virtual void Destroy();

	virtual void Stop(VideoYuvCallBack yuvCallback);

	virtual void Init(const char* devicename, int nWidth, int nHeight, int nFrameRate);

	virtual void Init(std::map<std::string, std::string> opts = {});

	virtual void RegisterCallback(VideoYuvCallBack yuvCallback);

	virtual void RegisterH264Callback(H264CallBack h264Callback) { m_h264Callback = h264Callback; };

	virtual bool onData(const char* id, unsigned char* buffer, int size, int64_t ts);

	virtual bool onData(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp) ;
	
	void CaptureThread();



public:
	virtual void OnSourceConnected(void* arg, const std::map<std::string, std::string>& opts)override;
	virtual void OnSourceDisConnected(int err) override {};
	virtual void OnSourceVideoFrame(const char* id, int index, uint8_t* yuv, int size, int64_t ts) override {};
	virtual void OnSourceAudioFrame(const char* id, int index, uint8_t* pcm, int size, int64_t ts) override {};
	virtual void OnSourceVideoPacket(const char* id, AVPacket* packet) override {};
	virtual void OnSourceAudioPacket(const char* id, AVPacket* packet)override {};

	virtual void OnSourceVideoPacket(const char* id, uint8_t* aBytes, int aSize, int64_t ts) override ;

	virtual void OnSourceAudioPacket(const char* id, uint8_t* aBytes, int aSize, int64_t t)override {};
	virtual bool OnIsQuitCapture() { return false; };

private:

	std::atomic<bool>m_bStop;
	std::mutex m_mutex;                //»¥³âËø		


	std::list<VideoYuvCallBack> m_YuvCallbackList;

	H264CallBack m_h264Callback;
	std::thread                        m_capturethread;
	std::string m_videourl;
	std::vector<uint8_t>               m_cfg;
	std::map<std::string, std::string> m_codec;
	MediaSourceAPI* source;
	FFmpegDecoderAPI* decoder=nullptr;
	FFmpegVideoEncoderAPI* m_ffmpegEncoder;
	int  m_nWidth = 0, m_nHeight = 0;
	int m_nFrameRate = 30;
	Detector* my_detector;
	std::list<std::string> m_listnames;
	int m_detector = 0;
};

