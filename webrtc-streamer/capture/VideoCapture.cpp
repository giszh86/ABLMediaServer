
#include "VideoCapture.h"

//#include "FFmpegVideoCapture.h"


#include <iostream>

#include "VideoCaptureImpl.h"


VideoCapture* VideoCapture::CreateVideoCapture(std::string videourl)
{
	std::map<std::string, std::string> opts;
	if ((videourl.find("rtsp://") == 0)|| (videourl.find("rtmp://") == 0))
	{	
		return new RtspVideoCapture(videourl, opts);
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
		return new RtspVideoCapture(videourl, opts);
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

bool isURLWithProtocol(const std::string& str) {
	// 判断字符串是否以协议开头，比如 "rtsp://"
	return (str.substr(0, 7) == "rtsp://" || str.substr(0, 7) == "http://" || str.substr(0, 7) == "rtmp://");
}

std::string extractPathFromURL(const std::string& url) {
	size_t pos = url.find("://");
	if (pos != std::string::npos) {
		// 如果字符串包含协议，提取协议后的路径部分
		return url.substr(pos + 3);
	}
	else {
		// 如果没有协议，直接返回原始字符串
		return url;
	}
}
std::string getPortionAfterPort(const std::string& str) {
	size_t startPos = str.find(':', 6); // 从第6个字符开始查找冒号，跳过协议部分
	if (startPos == std::string::npos) {
		return ""; // 找不到冒号，返回空字符串
	}

	size_t endPos = str.find('/', startPos); // 从冒号后面查找第一个斜杠
	if (endPos == std::string::npos) {
		return ""; // 找不到斜杠，返回空字符串
	}

	return str.substr(endPos); // 提取斜杠后面的部分
}
// 获取输入流对象
VideoCapture* VideoCaptureManager::GetInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string path="";
	if (isURLWithProtocol(videoUrl)) {
		//std::cout << "Input string is a URL with protocol." << std::endl;
		path = getPortionAfterPort(videoUrl);
	//	std::cout << "Extracted path: " << path << std::endl;
	}
	else {
		//std::cout << "Input string is a simple path." << std::endl;
		path = videoUrl;
	}


	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		return it->second;
	}
	else
	{
		VideoCapture* input = VideoCapture::CreateVideoCapture(path);
		if (input)
		{
			m_inputMap[path] = input;
		}
		return input;
	}
}

VideoCaptureManager& VideoCaptureManager::getInstance()
{
	static VideoCaptureManager instance;
	return instance;
}

