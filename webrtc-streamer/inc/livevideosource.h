/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** livevideosource.h
**
** -------------------------------------------------------------------------*/

#pragma once

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
#include "api/video_codecs/video_decoder.h"
#include "VideoDecoder.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif


#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include "spdloghead.h"
#include "../capture/VideoCapture.h"

class LiveVideoSource : public VideoSourceWithDecoder, public MediaSourceEvent
{
public:
	LiveVideoSource(const std::string& uri, const std::map<std::string, std::string>& opts, std::unique_ptr<webrtc::VideoDecoderFactory>& videoDecoderFactory, bool wait) :
		VideoSourceWithDecoder(opts, videoDecoderFactory, wait)
    {
        m_videourl = uri;
		Start();
	}
	virtual ~LiveVideoSource() {
		this->Stop();
	}
	static LiveVideoSource* Create(const std::string& url, const std::map<std::string, std::string>& opts, std::unique_ptr<webrtc::VideoDecoderFactory>& videoDecoderFactory) {
		return new LiveVideoSource(url, opts, videoDecoderFactory,false);
	}
    void Start()
    {
        SPDLOG_LOGGER_INFO(spdlogptr, "LiveVideoSource::Start");
		m_vCapture = VideoCaptureManager::getInstance().GetInput(m_videourl);
		if (m_vCapture == nullptr)
		{
			m_vCapture = VideoCaptureManager::getInstance().AddInput(m_videourl);
		}
        m_vCapture->setCallbackEvent(this);
        m_vCapture->Start();


    }
    void Stop()
    {
        SPDLOG_LOGGER_INFO(spdlogptr, "LiveVideoSource::stop");
        m_vCapture = VideoCaptureManager::getInstance().GetInput(m_videourl);
		if (m_vCapture)
		{
			m_vCapture->setCallbackEvent(nullptr);
		
			VideoCaptureManager::getInstance().RemoveInput(m_videourl);
			
		}

    }
   
    void onH264Data(unsigned char *buffer, ssize_t size, int64_t ts, const std::string & codec) {
        std::vector<webrtc::H264::NaluIndex> indexes = webrtc::H264::FindNaluIndices(buffer,size);
        SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData nbNalu:{}" ,indexes.size());
        for (const webrtc::H264::NaluIndex & index : indexes) {
            webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(buffer[index.payload_start_offset]);
            SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData NALU type:{}  payload_size:{}  payload_start_offset:{} start_offset:{}" ,
                (int) nalu_type , index.payload_size , index.payload_start_offset ,index.start_offset);
            if (nalu_type == webrtc::H264::NaluType::kSps)
            {
                SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData SPS");
                m_cfg.clear();
                m_cfg.insert(m_cfg.end(), buffer + index.start_offset, buffer + index.payload_size + index.payload_start_offset);

				absl::optional<webrtc::SpsParser::SpsState> sps = webrtc::SpsParser::ParseSps(buffer + index.payload_start_offset + webrtc::H264::kNaluTypeSize, index.payload_size - webrtc::H264::kNaluTypeSize);
				if (!sps)
				{				
                    SPDLOG_LOGGER_ERROR(spdlogptr, "cannot parse sps");
                    m_decoder.postFormat(codec, 0, 0);
                }
                else
                {
                    SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData SPS set format:{} x={} "
                        ,sps->height);
                    m_decoder.postFormat(codec, sps->width, sps->height);
                }
            }
            else if (nalu_type == webrtc::H264::NaluType::kPps)
            {
                SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData PPS");
                m_cfg.insert(m_cfg.end(), buffer + index.start_offset, buffer + index.payload_size + index.payload_start_offset);
            }
            else if (nalu_type == webrtc::H264::NaluType::kSei) 
            {

            }            
            else
            {
                webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameDelta;
                std::vector<uint8_t> content;
                if (nalu_type == webrtc::H264::NaluType::kIdr)
                {
                    frameType = webrtc::VideoFrameType::kVideoFrameKey;
                    SPDLOG_LOGGER_TRACE(spdlogptr, "LiveVideoSource:onData IDR");
                    content.insert(content.end(), m_cfg.begin(), m_cfg.end());
                }
                else
                {
                    SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData SLICE NALU:{}"
                        ,(int)nalu_type);
                }
                if (m_prevTimestamp && ts < m_prevTimestamp && m_decoder.m_decoder && strcmp(m_decoder.m_decoder->ImplementationName(),"FFmpeg")==0) 
                {
                    SPDLOG_LOGGER_DEBUG(spdlogptr, "LiveVideoSource:onData drop frame in past for FFmpeg:{}", (m_prevTimestamp-ts));

                } else {
                   //ts = rtc::TimeMillis();
                    content.insert(content.end(), buffer + index.start_offset, buffer + index.payload_size + index.payload_start_offset);
                    rtc::scoped_refptr<webrtc::EncodedImageBuffer> frame = webrtc::EncodedImageBuffer::Create(content.data(), content.size());
                    m_decoder.PostFrame(frame, ts, frameType);
                }
            }
        }
    }

    void onH265Data(unsigned char *buffer, ssize_t size, int64_t ts, const std::string & codec) {
       
    }

    int onJPEGData(unsigned char *buffer, ssize_t size, int64_t ts, const std::string & codec) {
        int res = 0;
        int32_t width = 0;
        int32_t height = 0;
        if (libyuv::MJPGSize(buffer, size, &width, &height) == 0)
        {
            int stride_y = width;
            int stride_uv = (width + 1) / 2;

            rtc::scoped_refptr<webrtc::I420Buffer> I420buffer = webrtc::I420Buffer::Create(width, height, stride_y, stride_uv, stride_uv);
            const int conversionResult = libyuv::ConvertToI420((const uint8_t *)buffer, size,
                                                                I420buffer->MutableDataY(), I420buffer->StrideY(),
                                                                I420buffer->MutableDataU(), I420buffer->StrideU(),
                                                                I420buffer->MutableDataV(), I420buffer->StrideV(),
                                                                0, 0,
                                                                width, height,
                                                                width, height,
                                                                libyuv::kRotate0, ::libyuv::FOURCC_MJPG);

            if (conversionResult >= 0)
            {
                webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
                    .set_video_frame_buffer(I420buffer)
                    .set_rotation(webrtc::kVideoRotation_0)
                    .set_timestamp_ms(ts)
                    .set_id(ts)
                    .build();
                m_decoder.Decoded(frame);
            }
            else
            {
                RTC_LOG(LS_ERROR) << "LiveVideoSource:onData decoder error:" << conversionResult;
                res = -1;
            }
        }
        else
        {
            RTC_LOG(LS_ERROR) << "LiveVideoSource:onData cannot JPEG dimension";
            res = -1;
        }
        return res;
    }

	void to_upper(std::string& str)
	{
		for (char& c : str) {
			c = std::toupper(static_cast<unsigned char>(c));
		}
	}

    bool onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime, int64_t ts1)
  {
		//// 使用 std::chrono 获取当前时间
		//auto now = std::chrono::system_clock::now();
		//auto milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		//// 将当前时间赋值给 ts
		//int64_t ts = milliseconds_since_epoch;
        std::lock_guard<std::mutex> _lock(m_mutex);
		// 使用 std::chrono 获取当前时间
		auto now = std::chrono::system_clock::now();
		auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
		auto epoch = now_ms.time_since_epoch();
		int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();


		//SPDLOG_LOGGER_ERROR(spdlogptr, "LiveVideoSource:onData id:{}   size:{} ts:{} ", id, size, ts);
	  int res = 0;

	  std::string codec = id;
	  if (codec == "H264")
	  {
		  onH264Data(buffer, size, ts, codec);
      
	  }
	  else if (codec == "H265")
	  {
		 // onH265Data(buffer, size, ts, codec);
	  }
	  else if (codec == "JPEG")
	  {
		  res = onJPEGData(buffer, size, ts, codec);
	  }
	  else if (codec == "VP9")
	  {
		  m_decoder.postFormat(codec, 0, 0);

		  webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameKey;
		  rtc::scoped_refptr<webrtc::EncodedImageBuffer> frame = webrtc::EncodedImageBuffer::Create(buffer, size);
		  m_decoder.PostFrame(frame, ts, frameType);
	  }

	  m_prevTimestamp = ts;
	  return (res == 0);
  }


	public:
        virtual void OnSourceConnected(void* arg, const std::map<std::string, std::string>& opts)override {};
		virtual bool onNewSession(const char* id, const char* media, const char* codec, const char* sdp, unsigned int rtpfrequency, unsigned int channels) override
		{
			bool success = false;
			if (strcmp(media, "video") == 0)
			{
				SPDLOG_LOGGER_INFO(spdlogptr, "LiveVideoSource::onNewSession id:{} media={} sdp:", id, media, sdp);

				std::string strcodec = codec;
				to_upper(strcodec);
				if ((strcmp(strcodec.c_str(), "H264") == 0)
					|| (strcmp(strcodec.c_str(), "JPEG") == 0)
					|| (strcmp(strcodec.c_str(), "VP9") == 0))
				{
					m_codec[id] = strcodec.c_str();
					success = true;
				}
				SPDLOG_LOGGER_INFO(spdlogptr, "LiveVideoSource::onNewSession success:{}", success);
				if (success)
				{
					struct timeval presentationTime;
					timerclear(&presentationTime);

					std::vector<std::vector<uint8_t>> initFrames = m_decoder.getInitFrames(strcodec.c_str(), sdp);
					for (auto frame : initFrames)
					{
						onData(id, frame.data(), frame.size(), presentationTime,0);
					}
				}
			}
			return success;
		}
        virtual void OnSourceDisConnected(int err) override {};

        virtual void OnSourceVideoPacket(const char* id, uint8_t* aBytes, int aSize, int64_t ts) override 
        {
			// 定义并清空 presentationTime 结构
			struct timeval presentationTime;
			timerclear(&presentationTime);

            onData(id, aBytes, aSize, presentationTime,0);        
        
        };
		virtual void OnSourceAudioPacket(const char* id, uint8_t* aBytes, int aSize, int64_t ts) override {};
		virtual bool OnIsQuitCapture()
        {
			return false;
		};



private:

    std::vector<uint8_t>               m_cfg;
    std::map<std::string, std::string> m_codec;

    uint64_t                           m_prevTimestamp;
    VideoCapture* m_vCapture = nullptr;
    std::string m_videourl;
    std::mutex					m_mutex;
};
