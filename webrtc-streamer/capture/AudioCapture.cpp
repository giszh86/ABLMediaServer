

#include "AudioCapture.h"
#include <thread>

#include "FFmpegAudioCapturer.h"
AudioCapture* AudioCapture::CreateAudioCapture(std::string audiourl, const std::map<std::string, std::string> opts)
{
	return new FFmpegAudioSource(audiourl, opts);
}




// 添加输入流
AudioCapture* AudioCaptureManager::AddInput(const std::string& audioUrl)
{
	if (audioUrl.empty())
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string  path = audioUrl;
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		AudioCapture* input = it->second;
		return input;
	}
	else
	{
		AudioCapture* input = AudioCapture::CreateAudioCapture(audioUrl);
		if (input)
		{
			m_inputMap[audioUrl] = input;
		}
		return input;
	}

}

// 移除输入流
void AudioCaptureManager::RemoveInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string path = videoUrl;
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		delete it->second;
		it->second = nullptr;
		m_inputMap.erase(it);
	}
}

// 获取输入流对象
AudioCapture* AudioCaptureManager::GetInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string  path = videoUrl;
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


std::string AudioCaptureManager::getStream(const std::string& videoUrl)
{


	std::string path = "";
	if (isURLWithProtocol(videoUrl)) {
		//std::cout << "Input string is a URL with protocol." << std::endl;
		path = getPortionAfterPort(videoUrl);
		//	std::cout << "Extracted path: " << path << std::endl;
	}
	else {
		//std::cout << "Input string is a simple path." << std::endl;
		path = videoUrl;
	}
	return videoUrl;
}

AudioCaptureManager& AudioCaptureManager::getInstance()
{
	static AudioCaptureManager instance;
	return instance;
}
