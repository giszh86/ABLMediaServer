#include "VideoTrackSourceInput.h"

#include "../capture/EncodedVideoFrameBuffer.h"

VideoTrackSourceInput::VideoTrackSourceInput() {



}

VideoTrackSourceInput* VideoTrackSourceInput::Create(const std::string& videourl, const std::map<std::string, std::string>& opts)
{
	VideoTrackSourceInput* vcm_capturer = new  rtc::RefCountedObject<VideoTrackSourceInput>();
	size_t width = 0;
	size_t height = 0;
	size_t fps = 0;
	if (opts.find("width") != opts.end()) {
		width = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		height = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		fps = std::stoi(opts.at("fps"));
	}
	//if (!vcm_capturer->Init(width, height, fps, videourl)) {
	//	RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer(w = " << width
	//		<< ", h = " << height << ", fps = " << fps
	//		<< ")";
	//	return nullptr;
	//}
	if (!vcm_capturer->Init(videourl, opts)) {
		RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer(w = " << width
			<< ", h = " << height << ", fps = " << fps
			<< ")";
		return nullptr;
	}
	return vcm_capturer;
}


VideoTrackSourceInput::~VideoTrackSourceInput()
{
	if (m_vCapture)
	{
		VideoCaptureManager::getInstance().RemoveInput(m_videourl);
		//m_vCapture->Destroy();
		//delete m_vCapture;;
		//m_vCapture = nullptr;
	}
}
bool VideoTrackSourceInput::Init(size_t width, size_t height, size_t target_fps, const std::string& videourl)
{

	m_videourl = videourl;
	m_vCapture = VideoCaptureManager::getInstance().GetInput(videourl);
	m_vCapture->Init(videourl.c_str(), width, height, target_fps);
	m_vCapture->RegisterCallback([this](uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)
		{
			InputVideoFrame(y, strideY, u, strideU, v, strideV, nWidth, nHeight,
				nTimeStamp);
		});
	m_vCapture->RegisterH264Callback([this](char* h264_raw, int file_size, bool bKey, int nWidth, int nHeight, int fps, int64_t nTimeStamp)
		{
			InputVideoFrame((unsigned char*)h264_raw, file_size, nWidth, nHeight,fps);
		});

	m_vCapture->Start();
	return true;

}
bool VideoTrackSourceInput::Init(std::string videourl, std::map<std::string, std::string> opts)
{
	m_videourl = videourl;
	m_opts = opts;
	size_t width = 0;
	size_t height = 0;
	size_t fps = 0;
	if (opts.find("width") != opts.end()) {
		width = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		height = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		fps = std::stoi(opts.at("fps"));
	}
	/// <summary>
	/// 
	/// </summary>
	/// <param name="videourl"></param>
	/// <param name="opts"></param>
	/// <returns></returns>
	return Init(width, height, fps, videourl);
}
void VideoTrackSourceInput::changeVideoInput(size_t width, size_t height, size_t target_fps, std::string videourl)
{
	Init(width, height, target_fps, videourl);

}
webrtc::MediaSourceInterface::SourceState VideoTrackSourceInput::state() const
{
	return webrtc::MediaSourceInterface::kLive;
}

bool VideoTrackSourceInput::remote() const
{
	return false;
}

bool VideoTrackSourceInput::is_screencast() const
{
	if ((m_videourl.find("window://") == 0))
	{
		return true;
	}
	else if ((m_videourl.find("screen://") == 0))
	{
		return true;
	}
	else if ((m_videourl.find("rtsp://") == 0))
	{
		return true;
	}
	else if ((m_videourl.find("file://") == 0))
	{
		return true;
	}
	else
	{
		return false;
	}

}

absl::optional<bool> VideoTrackSourceInput::needs_denoising() const
{
	return false;
}

void VideoTrackSourceInput::InputVideoFrame(const unsigned char* y, const unsigned char* u, const unsigned char* v, int width, int height, int frame_rate)
{
	std::shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
	_worker_thread_ptr->Start();
	_worker_thread_ptr->PostTask([&]()
		{
			rtc::scoped_refptr<webrtc::I420Buffer> buffer(webrtc::I420Buffer::Copy(width, height, y, width, u, width / 2, v, width / 2));

			next_timestamp_us_ = rtc::TimeMicros();

			webrtc::VideoFrame frame = webrtc::VideoFrame(buffer, webrtc::VideoRotation::kVideoRotation_0, next_timestamp_us_);

			next_timestamp_us_ += rtc::kNumMicrosecsPerSec / frame_rate;

			OnFrame(frame);

		}
	);

}

void VideoTrackSourceInput::InputVideoFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)
{

	std::shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
	_worker_thread_ptr->Start();
	_worker_thread_ptr->PostTask([&]()
		{
			rtc::scoped_refptr<webrtc::I420Buffer> buffer(webrtc::I420Buffer::Copy(nWidth, nHeight, y, nWidth, u, nWidth / 2, v, nWidth / 2));

			next_timestamp_us_ = rtc::TimeMicros();

			webrtc::VideoFrame frame = webrtc::VideoFrame(buffer, webrtc::VideoRotation::kVideoRotation_0, next_timestamp_us_);


			OnFrame(frame);

		}
	);

}

bool VideoTrackSourceInput::InputVideoFrame(unsigned char* data, size_t size, int nWidth, int nHeigh,int fps)
{
	std::shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
	_worker_thread_ptr->Start();
	_worker_thread_ptr->PostTask([&]()
		{
			std::lock_guard<std::mutex> guard(m_mutex);

			int  timestamp_us_ = rtc::TimeMicros();
			int64_t perio = timestamp_us_ - m_prevts;
			if (perio < 0 || m_prevts ==0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000/fps));
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(perio));

			}
			rtc::scoped_refptr<webrtc::EncodedImageBuffer> imageframe = webrtc::EncodedImageBuffer::Create(data, size);
			rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(1280, 780, imageframe);
			//	webrtc::VideoFrame frame(buffer, webrtc::kVideoRotation_0, next_timestamp_us_);
			int64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
			webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
				.set_video_frame_buffer(buffer)
				.set_rotation(webrtc::kVideoRotation_0)
				.set_timestamp_ms(ts)
				.set_id(ts)
				.build();

			OnFrame(frame);


			m_prevts = rtc::TimeMicros();


		});
	return true;
}


bool VideoTrackSourceInput::InputVideoFrame(const char* id, unsigned char* buffer, size_t size, int nWidth, int nHeigh, int64_t ts)
{
	//rtc::scoped_refptr<webrtc::EncodedImageBufferInterface> encodedData = buffer;
	rtc::scoped_refptr<webrtc::EncodedImageBuffer> imagebuffer = webrtc::EncodedImageBuffer::Create(buffer, size);
	next_timestamp_us_ = rtc::TimeMicros();
	rtc::scoped_refptr<webrtc::VideoFrameBuffer> framebuffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(nWidth, nHeigh, imagebuffer);
	webrtc::VideoFrame frame(framebuffer, webrtc::kVideoRotation_0, next_timestamp_us_);
	OnFrame(frame);
	//

	//rtc::scoped_refptr<webrtc::EncodedImageBuffer> image_frame = _custom_nvenc_encoder->Encode(video_frame);
	//rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(_width, _height, image_frame);
	//webrtc::VideoFrame final_frame(buffer, webrtc::kVideoRotation_0, _next_timestamp_us);
	//OnFrame(final_frame);


	return true;
}