/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FFmpegSource.h
**
** -------------------------------------------------------------------------*/

#pragma once
// 是否支持ffmpeg功能
#define USE_FFMPEG		1

#if USE_FFMPEG

#include <string.h>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "libyuv/video_common.h"
#include "libyuv/convert.h"

#include "media/base/codec.h"
#include "media/base/video_common.h"
#include "media/base/video_broadcaster.h"
#include "media/engine/internal_decoder_factory.h"

#include "common_video/h264/h264_common.h"
#include "common_video/h264/sps_parser.h"
#include "modules/video_coding/h264_sprop_parameter_sets.h"

#include "api/video_codecs/video_decoder.h"



#include "../3rd/libffmpeg/include/ffmpeg_headers.h"


class FFmpegVideoSource : public rtc::VideoSourceInterface<webrtc::VideoFrame>
{
public:
	FFmpegVideoSource(const std::string& uri, const std::map<std::string, std::string>& opts, std::unique_ptr<webrtc::VideoDecoderFactory>& videoDecoderFactory, bool wait) :
		m_decoder(m_broadcaster, opts, videoDecoderFactory, wait)
	{
		m_strUrl = uri;

		this->Start();
	}
	virtual ~FFmpegVideoSource() {
		this->Stop();
	}

	static FFmpegVideoSource* Create(const std::string& url, const std::map<std::string, std::string>& opts, std::unique_ptr<webrtc::VideoDecoderFactory>& videoDecoderFactory) {
		return new FFmpegVideoSource(url, opts, videoDecoderFactory, true);
	}

	void Start()
	{
		RTC_LOG(LS_INFO) << "FFmpegSource::Start";
		m_capturethread = std::thread(&FFmpegVideoSource::CaptureThread, this);


#if USE_FFMPEG	
		m_ffmpegDecoder.Start();
#else
		m_decoder.Start();
#endif




	}
	void Stop()
	{

		RTC_LOG(LS_INFO) << "FFmpegSource::stop";
		m_capturethread.join();
#if USE_FFMPEG
		m_ffmpegDecoder.Stop();
		if (m_pBaseCapture)
		{
			m_pBaseCapture->Stop();
		}
#else
		m_decoder.Stop();
#endif
	}
	bool IsRunning() { return (m_stop == 0); }

	void CaptureThread()
	{
		if (m_pBaseCapture == nullptr)
		{
			m_pBaseCapture = BaseStreamCapture::CreateStreamCapture(m_strUrl);

			m_pBaseCapture->RegisterVideoCallBack([=](uint8_t* raw_data, const char* codecid, int raw_len, bool bKey, int nWidth, int nHeight, int64_t nTimeStamp)
				{
					m_codec[codecid] = codecid;
					onData(codecid, raw_data, raw_len, nTimeStamp);

				});

			m_pBaseCapture->Start();
		}
	}

	bool onData(const char* id, unsigned char* buffer, ssize_t size, int64_t ts)
	{
		//int64_t ts = presentationTime.tv_sec;
		//ts = ts * 1000 + presentationTime.tv_usec / 1000;
		RTC_LOG(LS_VERBOSE) << "FFmpegSource:onData id:" << id << " size:" << size << " ts:" << ts;
		int res = 0;
		std::string codec = m_codec[id];	
		if (codec == "H264")
		{
			webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(buffer[sizeof(H264_marker)]);
			if (nalu_type == webrtc::H264::NaluType::kSps)
			{
				RTC_LOG(LS_VERBOSE) << "FFmpegSource:onData SPS";
				m_cfg.clear();
				m_cfg.insert(m_cfg.end(), buffer, buffer + size);

				absl::optional<webrtc::SpsParser::SpsState> sps = webrtc::SpsParser::ParseSps(buffer + sizeof(H264_marker) + webrtc::H264::kNaluTypeSize, size - sizeof(H264_marker) - webrtc::H264::kNaluTypeSize);
				if (!sps)
				{
					RTC_LOG(LS_ERROR) << "cannot parse sps";
					res = -1;
				}
				else
				{
					if (m_ffmpegDecoder.hasDecoder())
					{
						if ((m_format.width != sps->width) || (m_format.height != sps->height))
						{
							RTC_LOG(LS_INFO) << "format changed => set format from " << m_format.width << "x" << m_format.height << " to " << sps->width << "x" << sps->height;
							m_ffmpegDecoder.destroyDecoder();
						}
					}
					if (!m_ffmpegDecoder.hasDecoder())
					{
						int fps = 25;
						RTC_LOG(LS_INFO) << "FFmpegSource:onData SPS set format " << sps->width << "x" << sps->height << " fps:" << fps;
						cricket::VideoFormat videoFormat(sps->width, sps->height, cricket::VideoFormat::FpsToInterval(fps), cricket::FOURCC_I420);
						m_format = videoFormat;
						FrameInfo frame_info;
						memset(&frame_info, 0, sizeof(FrameInfo));
						frame_info.FramesPerSecond = 30;
						frame_info.Width = sps->width;
						frame_info.Height = sps->height;
						frame_info.VCodec = CAREYE_CODEC_H264;
						bool bres = m_ffmpegDecoder.createDecoder(&frame_info);
						m_ffmpegDecoder.RegisterDecodeCallback([=](uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp) {


							rtc::scoped_refptr<webrtc::I420Buffer> buffer(webrtc::I420Buffer::Copy(nWidth, nHeight, y, nWidth, u, nWidth / 2, v, nWidth / 2));
							//std::this_thread::sleep_for(std::chrono::milliseconds(30));
							int64_t	next_timestamp_us_ = rtc::TimeMicros();

							webrtc::VideoFrame frame = webrtc::VideoFrame(buffer, webrtc::VideoRotation::kVideoRotation_0, next_timestamp_us_);

							m_broadcaster.OnFrame(frame);

							});


					}
				}
			}
			else if (nalu_type == webrtc::H264::NaluType::kPps)
			{
				RTC_LOG(LS_VERBOSE) << "FFmpegSource:onData PPS";
				m_cfg.insert(m_cfg.end(), buffer, buffer + size);
			}
			else if (nalu_type == webrtc::H264::NaluType::kSei)
			{
				m_cfg.insert(m_cfg.end(), buffer, buffer + size);
			}
			if (m_ffmpegDecoder.hasDecoder())
			{
				webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameDelta;
				std::vector<uint8_t> content;
				if (nalu_type == webrtc::H264::NaluType::kIdr)
				{
					frameType = webrtc::VideoFrameType::kVideoFrameKey;
					RTC_LOG(LS_VERBOSE) << "FFmpegSource:onData IDR";
					content.insert(content.end(), m_cfg.begin(), m_cfg.end());
				}
				else
				{
					RTC_LOG(LS_VERBOSE) << "FFmpegSource:onData SLICE NALU:" << nalu_type;
				}
				content.insert(content.end(), buffer, buffer + size);

				m_ffmpegDecoder.PostFrame(content.data(), content.size(),true, rtc::TimeMicros());

			}
			else
			{
				//m_ffmpegDecoder.PostFrame(buffer, size, rtc::TimeMicros());
				RTC_LOG(LS_ERROR) << "FFmpegSource:onData no decoder";
				res = -1;
			}
			return (res == 0);
		}		
	}



	
	void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink, const rtc::VideoSinkWants& wants)
	{
		m_broadcaster.AddOrUpdateSink(sink, wants);
	}

	void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
	{
		m_broadcaster.RemoveSink(sink);
	}

private:
	char        m_stop;



private:
	std::thread                        m_capturethread;
	cricket::VideoFormat               m_format;
	std::vector<uint8_t>               m_cfg;
	std::map<std::string, std::string> m_codec;

	rtc::VideoBroadcaster              m_broadcaster;


	FFmpegDecoderAPI  m_ffmpegDecoder;
	BaseStreamCapture* m_pBaseCapture=nullptr;
	std::string  m_strUrl;

};


#endif