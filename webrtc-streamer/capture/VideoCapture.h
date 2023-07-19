#pragma once

#include <functional>
#include <map>
#include <string>
#include "HSingleton.h"



typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)> VideoYuvCallBack; 

typedef std::function<void(char* h264_raw, int file_size, bool bKey, int nWidth, int nHeight, int64_t nTimeStamp)> H264CallBack;



#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
#include "api/video/video_frame.h"
typedef std::function<void(const webrtc::VideoFrame& videoFrame)> FrameCallBack;
#endif // WEBRTC_WIN


/*
	std::map<std::string, std::string> opts;
	opts["width"] = to_string(m_StreamConfig.nWidth);
	opts["height"] = to_string(m_StreamConfig.nHeight);
	opts["fps"] = to_string(m_StreamConfig.fps);
	opts["bitrate"] = to_string(m_StreamConfig.startVideoBitrate);
	opts["timeout"] = to_string(5);
if (opts.find("width") != opts.end()) {
	width = std::stoi(opts.at("width"));
}
if (opts.find("height") != opts.end()) {
	height = std::stoi(opts.at("height"));
}
if (opts.find("fps") != opts.end()) {
	fps = std::stoi(opts.at("fps"));
}

*/

class VideoCapture
{
public:
	static VideoCapture* CreateVideoCapture(std::string videourl="");

	VideoCapture()
	{
	}
	virtual  ~VideoCapture()
	{
	}
	virtual bool Start() = 0;

	virtual void Init(const char* devicename, int nWidth = 1920, int nHeight = 1080, int nFrameRate = 30) = 0;
	//virtual void Init(std::map<std::string, std::string> opts ={}) = 0;

	virtual void Stop(VideoYuvCallBack yuvCallback) = 0;
	virtual void Destroy()=0;

	virtual void RegisterCallback(VideoYuvCallBack yuvCallback) = 0;

	virtual void RegisterH264Callback(H264CallBack yuvCallback) = 0;

	virtual bool onData(const char* id, unsigned char* buffer, int size, int64_t ts)=0;

	virtual bool onData(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp) = 0;

};


class VideoCaptureManager
{
public:
	VideoCaptureManager() = default;
	~VideoCaptureManager() {};

	// 添加输入流
	void AddInput(const std::string& videoUrl);

	// 移除输入流
	void RemoveInput(const std::string& videoUrl);

	// 获取输入流对象
	VideoCapture* GetInput(const std::string& videoUrl);

private:
	std::mutex m_mutex;
	std::map<std::string, VideoCapture*> m_inputMap;
};

#define gbVideoTrackMgrGet HSingletonTemplatePtr<VideoCaptureManager>::Instance()