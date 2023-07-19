
#include "VideoCapture.h"
#include "VcamVideoCapture.h"
//#include "FFmpegVideoCapture.h"

VideoCapture* VideoCapture::CreateVideoCapture(std::string videourl)
{
	std::map<std::string, std::string> opts;
	if ((videourl.find("rtsp://") == 0)|| (videourl.find("rtmp://") == 0))
	{	
		//return new FFmpegVideoCapture(videourl, opts);
	}
	else if ((videourl.find("file://") == 0))
	{
	//	return new FFmpegVideoCapture(videourl, opts);
	}
#if defined(WEBRTC_WIN) 
	else if ((videourl.find("screen://") == 0))
	{
		//return new MyDesktopCapture();
	}
#endif // WEBRTC_WIN
	else if ((videourl.find("window://") == 0))
	{
	//	return new MyDesktopCapture();
	}
	else
	{
	//	return new VcamVideoCapture();
	}
	return nullptr;

}



// 添加输入流
void VideoCaptureManager::AddInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_inputMap.count(videoUrl) == 0)
	{
		VideoCapture* input = VideoCapture::CreateVideoCapture(videoUrl);
		if (input)
		{
			m_inputMap[videoUrl] = input;
		}
	}

}

// 移除输入流
void VideoCaptureManager::RemoveInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_inputMap.find(videoUrl);
	if (it != m_inputMap.end())
	{
		delete it->second;
		m_inputMap.erase(it);
	}
}

// 获取输入流对象
VideoCapture* VideoCaptureManager::GetInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_inputMap.find(videoUrl);
	if (it != m_inputMap.end())
	{
		return it->second;
	}
	else
	{
		VideoCapture* input = VideoCapture::CreateVideoCapture(videoUrl);
		if (input)
		{
			m_inputMap[videoUrl] = input;
		}
		return input;
	}
}
