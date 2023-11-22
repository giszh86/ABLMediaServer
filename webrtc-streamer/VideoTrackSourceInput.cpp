#include "VideoTrackSourceInput.h"

#include "EncodedVideoFrameBuffer.h"
#include "common_video/h264/h264_common.h"
#include "common_video/h264/sps_parser.h"
VideoTrackSourceInput::VideoTrackSourceInput() {

	m_bStop.store(false);
	videoFifo.InitFifo(1920 * 1080 * 10);
	Run();
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
	videoFifo.FreeFifo();

	m_vCapture->RegisterH264Callback(nullptr);
	m_bStop.store(true);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	if (m_vCapture)
	{
		VideoCaptureManager::getInstance().RemoveInput(m_videourl);
		m_vCapture = nullptr;
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
			m_nWidth = nWidth;
			m_nHeigh = nHeight;
			m_fps = fps;
			// 使用互斥锁保护共享资源
			std::unique_lock<std::mutex> lock(m_mutex);
			videoFifo.push((unsigned char*)h264_raw, file_size);
			m_condition.notify_one();
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

bool VideoTrackSourceInput::InputVideoFrame(unsigned char* data, size_t size, int nWidth, int nHeigh, int fps)
{

	if (m_bStop.load())
	{
		return false;
	}
	webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameDelta;
	std::vector<webrtc::H264::NaluIndex> naluIndexes = webrtc::H264::FindNaluIndices(data, size);
	for (webrtc::H264::NaluIndex index : naluIndexes) {
		webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(data[index.payload_start_offset]);
		if (nalu_type == webrtc::H264::NaluType::kIdr)
		{
			frameType = webrtc::VideoFrameType::kVideoFrameKey;
			break;
		}
	}
	auto  timestamp_us_ = rtc::TimeMillis();

	int64_t perio = timestamp_us_ - m_prevts;

	if (fps < 1)
	{
		fps = 25;
	}
	int videosize = videoFifo.GetSize();
	int nsleeptime = 0;
	if (videosize <= 3)
	{
		nsleeptime = 1000 / fps;
	
	}
	else if (videosize > 3 && videosize <= 10)
	{
		nsleeptime = 1000 / fps-2;
	
	}
	else if (videosize > 10 && videosize <= 15)
	{
		nsleeptime = 1000 / fps - 5;
	}
	else if (videosize > 15 && videosize <= 25)
	{
		nsleeptime = 1000 / fps - 10;
	}
	else
		nsleeptime = 2;


	if ( nsleeptime >0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(nsleeptime));
	}


	rtc::scoped_refptr<webrtc::EncodedImageBuffer> imageframe = webrtc::EncodedImageBuffer::Create(data, size);
	rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(nWidth, nHeigh, imageframe, frameType);
	//	webrtc::VideoFrame frame(buffer, webrtc::kVideoRotation_0, next_timestamp_us_);
	int64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
	webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
		.set_video_frame_buffer(buffer)
		.set_rotation(webrtc::kVideoRotation_0)
		.set_timestamp_rtp(ts)
		.set_timestamp_ms(ts)
		.set_id(ts)
		.build();
	OnFrame(frame);
	videoFifo.pop_front();
	m_prevts = rtc::TimeMillis();

	return true;
}


bool VideoTrackSourceInput::InputVideoFrame(const char* id, unsigned char* buffer, size_t size, int nWidth, int nHeigh, int64_t ts)
{
	std::lock_guard<std::mutex> guard(m_mutex);
	webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameDelta;
	std::vector<webrtc::H264::NaluIndex> naluIndexes = webrtc::H264::FindNaluIndices(buffer, size);
	for (webrtc::H264::NaluIndex index : naluIndexes) {
		webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(buffer[index.payload_start_offset]);
		if (nalu_type == webrtc::H264::NaluType::kIdr)
		{
			frameType = webrtc::VideoFrameType::kVideoFrameKey;
			break;
		}
	}

	rtc::scoped_refptr<webrtc::EncodedImageBuffer> imagebuffer = webrtc::EncodedImageBuffer::Create(buffer, size);
	next_timestamp_us_ = rtc::TimeMicros();
	rtc::scoped_refptr<webrtc::VideoFrameBuffer> framebuffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(nWidth, nHeigh, imagebuffer, frameType);
	webrtc::VideoFrame frame(framebuffer, webrtc::kVideoRotation_0, next_timestamp_us_);
	OnFrame(frame);
	//

	//rtc::scoped_refptr<webrtc::EncodedImageBuffer> image_frame = _custom_nvenc_encoder->Encode(video_frame);
	//rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(_width, _height, image_frame);
	//webrtc::VideoFrame final_frame(buffer, webrtc::kVideoRotation_0, _next_timestamp_us);
	//OnFrame(final_frame);


	return true;
}



void VideoTrackSourceInput::Run()
{
	if (m_thread == nullptr)
	{
		m_thread = std::make_shared<std::thread>([=]()
			{
				while (!m_bStop.load())
				{
					unsigned char* pData;
					int            nLength;		
					{	
						// 使用互斥锁保护共享资源
						std::unique_lock<std::mutex> lock(m_mutex);
						// 等待条件变量，直到有数据可用
						m_condition.wait(lock, [this] { return (videoFifo.GetSize()>0) || m_bStop.load(); });
						pData = videoFifo.pop(&nLength);
					}		
					if (pData != NULL && nLength > 0)
					{
						InputVideoFrame(pData, nLength, m_nWidth,m_nHeigh,m_fps);
					}					
				}
			});
	}
}
