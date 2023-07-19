#include "Live555Capture.h"
#include <thread>
#include "audioProcessImpl.h"
Live555Capture::Live555Capture(std::string url): m_url(url)
{

}
Live555Capture::~Live555Capture()
{
	Stop();
	if (m_capturethread && m_capturethread->joinable())
	{
		m_capturethread->join();
		m_capturethread.reset();
	}
}
std::string Live555Capture::GetAudioConfig()
{
	return m_audio_config;
}

bool Live555Capture::Start()
{
	if (m_url.empty())
	{
		std::cout << "ERROR URL=" << m_url << std::endl;
		return false;
	}	
	m_ptr_connetion.reset(CreateRTSPConnectionObject());
	m_capturethread = std::make_shared<std::thread>([&]() {
			while (true)
			{
				if (m_environment)
					m_environment->mainloop();
				if (m_environment && m_bRestart)
				{
					m_bRestart = false;
					m_ptr_connetion.reset(CreateRTSPConnectionObject());
				}
				else
				{
					break;
				}
			}
		});
	return true;
}
void Live555Capture::RegisterVideoCallBack(VideoCllBack videoCallback)
{
	if (videoCallback)
	{
		m_h264CallBack = videoCallback;
	}
}
void Live555Capture::RegisterAudioCallBack(AudioCallBack  audioCallback)
{
	if (audioCallback)
	{
		m_aacCallBack = audioCallback;
	}
}
void Live555Capture::Init(const std::map<std::string, std::string>& opts)
{
	return;
}

void Live555Capture::Stop()
{		
	if(m_environment)
		m_environment->stop();
}
//void Live555Capture::CaptureThread()
//{
//	while (true)
//	{
//		if (m_environment)
//		m_environment->mainloop();
//		if (m_environment && m_bRestart)
//		{									
//			m_bRestart = false;
//			m_ptr_connetion.reset(CreateRTSPConnectionObject());
//		}	
//		else
//		{
//			break;
//		}
//	}	
//}
RTSPConnection* Live555Capture::CreateRTSPConnectionObject()
{		
	m_environment_stop = 0;
	if (m_environment)
	{
		delete m_environment;
		m_environment = nullptr;
	}
	m_environment = new Environment(m_environment_stop);
	if (!m_environment)
		return nullptr;
	return new RTSPConnection(*m_environment, this, m_url.c_str());
}
bool Live555Capture::onNewSession(const char* id, const char* media, const char* codec, const char* sdp)
{
	bool success = false;
	if (strcmp(media, "video") == 0)
	{
	//	RTC_LOG(INFO) << "LiveVideoSource::onNewSession id:" << id << " media:" << media << "/" << codec << " sdp:" << sdp;
		if (strcmp(codec, "H264") == 0)			
		{
			m_codec[id] = codec;
			success = true;
		}
	}	
	if (strcmp(media, "audio") == 0)
	{
		//	RTC_LOG(INFO) << "LiveVideoSource::onNewSession id:" << id << " media:" << media << "/" << codec << " sdp:" << sdp;
		if (strcmp(codec, "MPEG4-GENERIC") == 0)
		{
			m_codec[id] = codec;
			success = true;
			if (strstr(sdp, "config="))
			{
				char str[5] = { 0 };
				memcpy(&str[0], strstr(sdp, "config=") + 7, 4);
				m_audio_config = str;
			}
		}
	}
	return success;
}
bool Live555Capture::onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime)
{
	//std::cout << "onData" << std::endl;
	int64_t ts = presentationTime.tv_sec;
	ts = ts * 1000 + presentationTime.tv_usec / 1000;// ms
	std::string codec = m_codec[id];
	if (codec == "H264")
	{						
		int type = buffer[4] & 0x1f;
		//std::cout << "H264=" << presentationTime.tv_sec << ":" << presentationTime.tv_usec << " type="<< type << std::endl;		
		if (type == NaluType::kSps)//序列参数集
		{
			m_cfg.clear();
			m_cfg.insert(m_cfg.end(), buffer, buffer + size);
			/*if (m_ptr_vdecoder->h264_decode_seq_parameter_set(m_cfg.data() + 4, m_cfg.size() - 4, _width, _height))
			{
				if ((m_width != _width) || (m_height != _height))
				{
					m_width = _width;
					m_height = _height;					
				}
			}*/
		}else
		{
			if (type == NaluType::kPps)//图像参数集
			{
				m_cfg.insert(m_cfg.end(), buffer, buffer + size);
			}
			else
			{
				if (type == NaluType::kSei)//辅助增强信息 
				{

				}
				else
				{
					uint8_t bkey = false;
					std::vector<uint8_t> content;
					if (type == NaluType::kIdr)//IDR图像的编码条带(⽚)
					{
						content.insert(content.end(), m_cfg.begin(), m_cfg.end());
						bkey = true;
					}
					content.insert(content.end(), buffer, buffer + size);
					if (m_h264CallBack)
					{
						m_h264CallBack(content.data(), "H264",content.size(), bkey, m_width, m_height, ts);
					}				
				}

			}
		}
	}
	if (codec == "MPEG4-GENERIC")
	{
		//std::cout << " AAC=" << presentationTime.tv_sec << ":" << presentationTime.tv_usec << std::endl;
		if (m_aacCallBack)
		{
			if (AudioProcessImpl::Parser::ParserAudioConfig(m_audio_config.c_str(), m_channel, m_sample_rate) == 0)
			{
				m_aacCallBack(buffer, codec.c_str(), size, m_channel, m_sample_rate, m_pool_deep, ts);
			}
		}		
	}
	return true;
}
void Live555Capture::onError(RTSPConnection& rtsp_connection, const char* msg)
{
	std::cout << "RtspVideoCapture::onError " << std::endl;
	m_bRestart = true;
	m_environment->stop();
}
void Live555Capture::onConnectionTimeout(RTSPConnection& rtsp_connection)
{
	std::cout << "RtspVideoCapture::onConnectionTimeout " << std::endl;
	m_bRestart = true;
	 m_environment->stop();
}
 void Live555Capture::onDataTimeout(RTSPConnection& rtsp_connection)
{
	 std::cout << "RtspVideoCapture::onDataTimeout " << std::endl;
	 m_bRestart = true;
	 m_environment->stop();
}