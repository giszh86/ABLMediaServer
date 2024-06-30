

//#include "FFmpegVideoCapture.h"


#include <iostream>
#include "VideoCapture.h"
#include "VideoCaptureImpl.h"


VideoCapture* VideoCapture::CreateVideoCapture(std::string videourl)
{
	std::map<std::string, std::string> opts;
	return new RtspVideoCapture(videourl, opts);

}



// 添加输入流
VideoCapture* VideoCaptureManager::AddInput(const std::string& videoUrl)
{
	if (videoUrl.empty())
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string  path = getStream(videoUrl);
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		VideoCapture* input = it->second;
		return input;
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



// 移除输入流
void VideoCaptureManager::RemoveInput(const std::string& videoUrl)
{
	if (videoUrl.empty())
	{
		return;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string path = getStream(videoUrl);
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		delete it->second;
		it->second = nullptr;
		m_inputMap.erase(it);
	}

}
// 获取输入流对象
VideoCapture* VideoCaptureManager::GetInput(const std::string& videoUrl)
{
	if (videoUrl.empty())
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string path= getStream(videoUrl);
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

std::string VideoCaptureManager::getStream(const std::string& videoUrl)
{


	std::string path = "";
	if (VideoCaptureManager::getInstance().isURLWithProtocol(videoUrl)) {
		//std::cout << "Input string is a URL with protocol." << std::endl;
		path = VideoCaptureManager::getInstance().getPortionAfterPort(videoUrl);
		//	std::cout << "Extracted path: " << path << std::endl;
	}
	else {
		//std::cout << "Input string is a simple path." << std::endl;
		path = videoUrl;
	}
	return videoUrl;
}

VideoCaptureManager& VideoCaptureManager::getInstance()
{
	static VideoCaptureManager instance;
	return instance;
}

