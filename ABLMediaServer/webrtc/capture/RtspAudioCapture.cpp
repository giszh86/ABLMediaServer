#include "RtspAudioCapture.h"
#include <thread>

#include "rtc_base/logging.h"
RtspAudioCapture::RtspAudioCapture(const std::string& uri, const std::map<std::string, std::string>& opts, bool wait)
	:m_env(m_stop)
, m_freq(8000)
, m_channel(1)
, m_wait(wait)
, m_previmagets(0)
, m_prevts(0)
{
	m_pConnetion = new RTSPConnection(m_env, this, uri.c_str(), opts);
	m_factory = webrtc::CreateBuiltinAudioDecoderFactory();
}

RtspAudioCapture::~RtspAudioCapture()
{
	Destroy();
}

int RtspAudioCapture::Init(int nSampleRate, int nChannel, int nBitsPerSample)
{
	return 0;
}

int RtspAudioCapture::Init(const std::map<std::string, std::string>& opts)
{
	if (opts.find("url") != opts.end()) {
		m_url = opts.at("url");
	}	
	return 0;
}


int RtspAudioCapture::Start()
{
	m_capturethread = std::make_shared<std::thread>(&RtspAudioCapture::CaptureThread, this);
	return 0;
}

void RtspAudioCapture::Stop()
{
}

void RtspAudioCapture::RegisterPcmCallback(std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallback)
{

	m_pcmCallback = PcmCallback;

}

void RtspAudioCapture::RegisterAacCallback(std::function<void(uint8_t* aac_raw, int file_size, int64_t nTimeStamp)> aacCallBack)
{
	if (aacCallBack)
	{
		m_aacCallBack = aacCallBack;
	}

}


void RtspAudioCapture::Destroy()
{	
	m_env.stop();
	m_capturethread->join();
	m_pcmCallback = nullptr;
	m_aacCallBack = nullptr;
}
std::string RtspAudioCapture::GetAudioConfig()
{
	return m_audio_config;
}
void RtspAudioCapture::CaptureThread()
{
	int nError = 0;
	while (true)
	{
		m_env.mainloop();
		if (m_bRestart&&nError<3)
		{
			std::cout << "RtspVideoCapture::m_bRestart " << std::endl;
			std::map<std::string, std::string> opts;
			m_env.restart();
			m_bRestart = false;
			m_pConnetion = new RTSPConnection(m_env, this, m_url.c_str(), opts);
			nError++;
		}
		else
		{
			break;
		}
	}

}
bool RtspAudioCapture::onNewSession(const char* id, const char* media, const char* codec, const char* sdp)
{
	bool success = false;
	if (strcmp(media, "audio") == 0)
	{
		RTC_LOG(LS_INFO) << "LiveAudioSource::onNewSession " << media << "/" << codec << " " << sdp;

		// parse sdp to extract freq and channel
		std::string fmt(sdp);
		std::transform(fmt.begin(), fmt.end(), fmt.begin(), [](unsigned char c) { return std::tolower(c); });
		std::string codecstr(codec);
		std::transform(codecstr.begin(), codecstr.end(), codecstr.begin(), [](unsigned char c) { return std::tolower(c); });
		size_t pos = fmt.find(codecstr);
		if (pos != std::string::npos)
		{
			fmt.erase(0, pos + strlen(codec));
			fmt.erase(fmt.find_first_of(" \r\n"));
			std::istringstream is(fmt);
			std::string dummy;
			std::getline(is, dummy, '/');
			std::string freq;
			std::getline(is, freq, '/');
			if (!freq.empty())
			{
				m_freq = std::stoi(freq);
			}
			std::string channel;
			std::getline(is, channel, '/');
			if (!channel.empty())
			{
				m_channel = std::stoi(channel);
			}
		}
		std::string strConfig(sdp);
		size_t pos2 = strConfig.find("config=");
		if (pos2 != std::string::npos)
		{
			strConfig.erase(0, pos2 + strlen("config="));
			strConfig.erase(4);
			m_strConfig = strConfig;
		}

		RTC_LOG(LS_INFO) << "LiveAudioSource::onNewSession codec:" << codecstr << " freq:" << m_freq << " channel:" << m_channel;
		std::map<std::string, std::string> params;
		if (m_channel == 2)
		{
			params["stereo"] = "1";
		}

		webrtc::SdpAudioFormat format = webrtc::SdpAudioFormat(codecstr, m_freq, m_channel, std::move(params));
		//std::vector<webrtc::AudioCodecSpec> vec=  m_factory->GetSupportedDecoders();
		if (m_factory->IsSupportedDecoder(format))
		{
			m_decoder = m_factory->MakeAudioDecoder(format, absl::optional<webrtc::AudioCodecPairId>());
			m_codec[id] = codec;
			success = true;
		}
		else if (codec == "MPEG4-GENERIC")
		{
			m_aacdecoder = std::unique_ptr<AACDecoder>(new AACDecoder());
			m_codec[id] = codec;
			success = true;
		}
		else
		{
			RTC_LOG(LS_ERROR) << "LiveAudioSource::onNewSession not support codec" << sdp;		
		}
	}
	return success;
}
bool RtspAudioCapture::onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime)
{	

	bool success = false;
	int segmentLength = m_freq / 100;

	if (m_codec.find(id) != m_codec.end())
	{

		int64_t sourcets = presentationTime.tv_sec;
		sourcets = sourcets * 1000 + presentationTime.tv_usec / 1000;

		int64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;

		RTC_LOG(LS_VERBOSE) << "LiveAudioSource::onData decode ts:" << ts
			<< " source ts:" << sourcets;

		if (m_decoder.get() == NULL && m_aacdecoder.get() == nullptr)
		{
			RTC_LOG(LS_VERBOSE) << "LiveAudioSource::onData error:No Audio decoder";
			return success;
		}

		// waiting
		if ((m_wait) && (m_prevts != 0))
		{
			int64_t periodSource = sourcets - m_previmagets;
			int64_t periodDecode = ts - m_prevts;

			RTC_LOG(LS_VERBOSE) << "LiveAudioSource::onData interframe decode:" << periodDecode << " source:" << periodSource;
			int64_t delayms = periodSource - periodDecode;
			if ((delayms > 0) && (delayms < 1000))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(delayms));
			}
		}

		if (m_decoder.get() != NULL)
		{	
			int maxDecodedBufferSize = m_decoder->PacketDuration(buffer, size) * m_channel * sizeof(int16_t);
			int16_t* decoded = new int16_t[maxDecodedBufferSize];
			webrtc::AudioDecoder::SpeechType speech_type;
			int decodedBufferSize = m_decoder->Decode(buffer, size, m_freq, maxDecodedBufferSize, decoded, &speech_type);

			RTC_LOG(LS_VERBOSE) << "LiveAudioSource::onData size:" << size << " decodedBufferSize:" << decodedBufferSize << " maxDecodedBufferSize: " << maxDecodedBufferSize << " channels: " << m_channel;
			if (decodedBufferSize > 0)
			{
				for (int i = 0; i < decodedBufferSize; ++i)
				{
					m_buffer.push(decoded[i]);
				}
			}
			else
			{
				RTC_LOG(LS_ERROR) << "LiveAudioSource::onData error:Decode Audio failed";
			}
			delete[] decoded;

		}
		else
		{
			m_aacCallBack(buffer, size, ts);
			unsigned int pcm_size = 800 * 2048 * sizeof(INT_PCM);
			UCHAR* decoded = new UCHAR[pcm_size];
			UINT maxDecodedBufferSize = pcm_size;

			uint8_t* adts_ptr = buffer;
			if (buffer[0] != 0xFF) //判断adts头
			{
				AACHead head;
				head.SetAdtsHead(m_strConfig, size + 7);//2,3,2
				adts_ptr = head.AppendRawData(buffer, size);
				size += 7;
			}
			bool bres = m_aacdecoder->Decoder(&adts_ptr, size, decoded, &maxDecodedBufferSize);
		
			if (maxDecodedBufferSize > 0 && bres)
			{

				UCHAR* newdecoded = decoded;
				for (int i = 0; i < maxDecodedBufferSize / 2; ++i)
				{
					int16_t pOut;
					memcpy(&pOut, newdecoded, 2);
					newdecoded += 2;//字符型字节后移两位    
					m_buffer.push(pOut);
				}
			}
			else
			{
				RTC_LOG(LS_ERROR) << "LiveAudioSource::onData error:Decode Audio failed";
			}
			delete[] decoded;
			//delete[] intdecoded;	
		}
		while (m_buffer.size() > segmentLength * m_channel)
		{
			int16_t* outbuffer = new int16_t[segmentLength * m_channel];
			for (int i = 0; i < segmentLength * m_channel; ++i)

			{
				uint16_t value = m_buffer.front();
				outbuffer[i] = value;
				m_buffer.pop();
			}
			std::lock_guard<std::mutex> lock(m_sink_lock);
			for (auto* sink : m_sinks)
			{
				sink->OnData(outbuffer, 16, m_freq, m_channel, segmentLength);

			}
			delete[] outbuffer;
		}

		m_previmagets = sourcets;
		m_prevts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
		success = true;
	}
	return success;

}
void RtspAudioCapture::onError(RTSPConnection& rtsp_connection, const char* msg)
{
	std::cout << "RtspVideoCapture::onError " << std::endl;
	m_bRestart = true;
	m_env.stop();
}
void RtspAudioCapture::onConnectionTimeout(RTSPConnection& rtsp_connection)
{
	std::cout << "RtspVideoCapture::onConnectionTimeout " << std::endl;
	m_bRestart = true;
	m_env.stop();
}
void RtspAudioCapture::onDataTimeout(RTSPConnection& rtsp_connection)
{
	std::cout << "RtspVideoCapture::onDataTimeout " << std::endl;
	m_bRestart = true;
	m_env.stop();
}