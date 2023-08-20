
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



// ���������
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

// �Ƴ�������
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
	// �ж��ַ����Ƿ���Э�鿪ͷ������ "rtsp://"
	return (str.substr(0, 7) == "rtsp://" || str.substr(0, 7) == "http://" || str.substr(0, 7) == "rtmp://");
}

std::string extractPathFromURL(const std::string& url) {
	size_t pos = url.find("://");
	if (pos != std::string::npos) {
		// ����ַ�������Э�飬��ȡЭ����·������
		return url.substr(pos + 3);
	}
	else {
		// ���û��Э�飬ֱ�ӷ���ԭʼ�ַ���
		return url;
	}
}
std::string getPortionAfterPort(const std::string& str) {
	size_t startPos = str.find(':', 6); // �ӵ�6���ַ���ʼ����ð�ţ�����Э�鲿��
	if (startPos == std::string::npos) {
		return ""; // �Ҳ���ð�ţ����ؿ��ַ���
	}

	size_t endPos = str.find('/', startPos); // ��ð�ź�����ҵ�һ��б��
	if (endPos == std::string::npos) {
		return ""; // �Ҳ���б�ܣ����ؿ��ַ���
	}

	return str.substr(endPos); // ��ȡб�ܺ���Ĳ���
}
// ��ȡ����������
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

