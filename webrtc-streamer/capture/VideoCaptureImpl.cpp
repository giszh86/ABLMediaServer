#include "VideoCaptureImpl.h"


#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"


RtspVideoCapture::RtspVideoCapture(const std::string& uri, const std::map<std::string, std::string>& opts)
{
	m_bStop.store(false);

}

RtspVideoCapture::~RtspVideoCapture()
{
	SPDLOG_LOGGER_ERROR(spdlogptr, "FFmpegVideoCapture::stop start ");
	Destroy();
	SPDLOG_LOGGER_ERROR(spdlogptr, "FFmpegVideoCapture::stop end ");
}

bool RtspVideoCapture::Start()
{
	SPDLOG_LOGGER_INFO(spdlogptr, "LiveVideoSource::Start");

	return true;
}

void RtspVideoCapture::Destroy()
{

	m_bStop.store(true);
	m_YuvCallbackList.clear();
	m_h264Callback = nullptr;
	m_callbackEvent = nullptr;
	Stop(NULL);
}

void RtspVideoCapture::Stop(VideoYuvCallBack yuvCallback)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	using CallbackType = VideoYuvCallBack;

	auto it = std::find_if(m_YuvCallbackList.begin(), m_YuvCallbackList.end(), [&yuvCallback](const CallbackType& cb) {
		return *cb.target<void(*)()>() == *yuvCallback.target<void(*)()>();
		});
	if (it != m_YuvCallbackList.end())
	{
		m_YuvCallbackList.erase(it);
	}
}

void RtspVideoCapture::Init(const char* devicename, int nWidth, int nHeight, int nFrameRate)
{

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFrameRate = nFrameRate;
}

void RtspVideoCapture::Init(std::map<std::string, std::string> opts)
{
	if (opts.find("width") != opts.end())
	{
		m_nWidth = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		m_nHeight = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		m_nFrameRate = std::stoi(opts.at("fps"));
	}
}

void RtspVideoCapture::RegisterCallback(VideoYuvCallBack yuvCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			return;
		}
		it++;
	}
	m_YuvCallbackList.push_back(yuvCallback);
}

void RtspVideoCapture::CaptureThread()
{

}

bool RtspVideoCapture::onData(const char* id, unsigned char* buffer, int size, int64_t ts)
{
	if (m_bStop.load())
	{
		return false;
	}
	if (m_callbackEvent)
	{
		m_callbackEvent->OnSourceVideoPacket(id, (uint8_t*)buffer, size, ts);
	}
	return true;
}
