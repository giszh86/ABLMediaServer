/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FFmpegAudioSource.h
**
** -------------------------------------------------------------------------*/
// 是否支持ffmpeg功能
#define USE_FFMPEG		0

#if USE_FFMPEG

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <cctype>
#include <stdio.h>
#include <string.h>


#include "environment.h"
#include "pc/local_audio_source.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"



#include "common_video/h265/h265_common.h"
#include "common_video/h265/h265_sps_parser.h"
#include "common_audio/resampler/include/push_resampler.h"

#include "../nvenc_encoder/include/ffmpeg_headers.h"



typedef unsigned char uchar;
typedef unsigned int uint;


class FFmpegAudioSource : public webrtc::Notifier<webrtc::AudioSourceInterface>
{
public:
	static rtc::scoped_refptr<FFmpegAudioSource> Create(rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioDecoderFactory, const std::string& uri, const std::map<std::string, std::string>& opts) {
		rtc::scoped_refptr<FFmpegAudioSource> source(new rtc::RefCountedObject<FFmpegAudioSource>(audioDecoderFactory, uri, opts,true));
		return source;
	}

    SourceState state() const override { return kLive; }

    bool remote() const override { return true; }

    virtual void AddSink(webrtc::AudioTrackSinkInterface *sink) override
    {
        RTC_LOG(LS_INFO) << "FFmpegAudioSource::AddSink ";
        std::lock_guard<std::mutex> lock(m_sink_lock);
        m_sinks.push_back(sink);
    }

    virtual void RemoveSink(webrtc::AudioTrackSinkInterface *sink) override
    {
        RTC_LOG(LS_INFO) << "FFmpegAudioSource::RemoveSink ";
        std::lock_guard<std::mutex> lock(m_sink_lock);
        m_sinks.remove(sink);
    }

    void CaptureThread()
    {
        if (m_pBaseCapture == nullptr)
        {
            m_pBaseCapture = BaseStreamCapture::CreateStreamCapture(m_strUrl);

            m_pBaseCapture->RegisterAudioCallBack([=](uint8_t* raw_data, const char* codecid, int raw_len, int channels, int sample_rate, int bytes, int64_t nTimeStamp)
                {
                   // fwrite(raw_data, raw_len, 1, aacHandle);
                    m_codec[codecid] = codecid;
                    /*			struct timeval presentationTime;
                                timerclear(&presentationTime);*/
                    m_channel = channels;
                    m_freq = sample_rate;
                    frame_duration_ = 1.0 * 1024 / sample_rate * 1000;  // 得到一帧的毫秒时间
                    onData(codecid, raw_data, raw_len, nTimeStamp);

                });

            m_pBaseCapture->Start();
        }
    }


    bool onData(const char *id, unsigned char *buffer, ssize_t size, int64_t sourcets)
    {
        bool success = false;
        int segmentLength = m_freq / 100;

        if (m_codec.find(id) != m_codec.end())
        {
			if ((m_wait) && (m_prevts != 0))
			{
				int64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
				int64_t delayms = ts - m_prevts;
				int64_t dif = delayms - frame_duration_;
				if (dif > 0)
				{
					//std::this_thread::sleep_for(std::chrono::milliseconds(dif));
				}
				RTC_LOG(LS_VERBOSE) << "FFmpegAudioSource::onData decode ts:" << ts
					<< " source ts:" << sourcets;
			}        
		
			if (!m_ffmpegDecoder.hasDecoder())
			{		
                FrameInfo frame_info ;			
                if (m_codec[id] == "PCMA")
                {
                    frame_info.ACodec = CAREYE_CODEC_G711A;
                }
				else if (m_codec[id] == "PCMU")
				{
					frame_info.ACodec = CAREYE_CODEC_G711U;
				}
                else if (m_codec[id] == "AAC" ||m_codec[id] == "mpeg4-generic" || R"(MPEG4-GENERIC)" == m_codec[id])
                {
                    frame_info.ACodec = CAREYE_CODEC_AAC;
                }
                frame_info.VCodec = CAREYE_CODEC_NONE;
				frame_info.SampleRate = m_freq;
				frame_info.Channels = m_channel;
				frame_info.BitsPerSample = 16;
				frame_info.AudioBitrate = 64000;
				bool bres = m_ffmpegDecoder.createDecoder(&frame_info);      
                success = true;
			}
            else
            {
                unsigned char* decoded = NULL;
                decoded = (unsigned char*)malloc(1024 * 256);
                int maxDecodedBufferSize = m_ffmpegDecoder.DecoderPCM(buffer, size, decoded);
				if (maxDecodedBufferSize > 0)
				{
					int16_t* newdecoded = (int16_t*)decoded;
					
						for (int i = 0; i < maxDecodedBufferSize / 2; )
						{
							m_vecbuffer.push_back((int16_t)(newdecoded)[i]);
							i++;
						}
				}
				delete decoded;
				decoded = nullptr;
				delete buffer;
				buffer = nullptr;
				while (m_vecbuffer.size() > segmentLength * m_channel)
				{					
					std::lock_guard<std::mutex> lock(m_sink_lock);
					for (auto* sink : m_sinks)
					{
						sink->OnData(&m_vecbuffer[0], 16, m_freq, m_channel, segmentLength);
					}
					m_vecbuffer.erase(m_vecbuffer.begin(), m_vecbuffer.begin() + (segmentLength * m_channel));
				}                
				m_previmagets = sourcets;
				m_prevts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
				pcm_total_duration_ += frame_duration_;
                success = true;
            }         
        }



            
        return success;
    }

protected:
    FFmpegAudioSource(rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioDecoderFactory, const std::string &uri, const std::map<std::string, std::string> &opts, bool wait)
        : m_factory(audioDecoderFactory)
        , m_freq(8000)
        , m_channel(1)
        , m_wait(wait)
        , m_previmagets(0)
        , m_prevts(0)
    {
        m_capturethread = std::thread(&FFmpegAudioSource::CaptureThread, this);
        m_strUrl = uri;
#if USE_FFMPEG
		m_ffmpegDecoder.Start();
#endif
        
    }
    virtual ~FFmpegAudioSource()
    {
        m_pBaseCapture->Stop();
        m_capturethread.join();
#if USE_FFMPEG
		m_ffmpegDecoder.Stop();

#endif

	
    }

private:
    char m_stop;
 private:
  
    std::thread m_capturethread;
    rtc::scoped_refptr<webrtc::AudioDecoderFactory> m_factory;
    std::unique_ptr<webrtc::AudioDecoder> m_decoder;
    std::unique_ptr<AACDecoder> m_aacdecoder;
    int m_freq;
    int m_channel;
    std::vector<uint16_t> m_vecbuffer;

    std::list<webrtc::AudioTrackSinkInterface *> m_sinks;
    std::mutex m_sink_lock;

    std::map<std::string, std::string> m_codec;

    bool m_wait;
    int64_t m_previmagets;
    int64_t m_prevts;
    std::string m_strConfig;
#if USE_FFMPEG

    FFmpegDecoderAPI  m_ffmpegDecoder;
    BaseStreamCapture* m_pBaseCapture = nullptr;
    std::string  m_strUrl;
    int16_t m_ts;
    double frame_duration_ = 23.2;
    double pcm_total_duration_ = 0; // 推流时长的统计
#endif
  
};
#endif

