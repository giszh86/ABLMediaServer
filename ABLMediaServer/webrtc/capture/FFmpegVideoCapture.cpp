#include "FFmpegVideoCapture.h"

#include <iostream>
#include <fstream>
#include <mutex>

#include "ffmpeg_headers.h"
#include "darknetTools.h"
FFmpegVideoCapture::FFmpegVideoCapture(const std::string& uri, const std::map<std::string, std::string>& opts)
	:m_videourl(uri),
	m_bStop(false)
{

}

FFmpegVideoCapture::~FFmpegVideoCapture()
{
	Destroy();
}

bool FFmpegVideoCapture::Start()
{
	//RTC_LOG(LS_INFO) << "LiveVideoSource::Start";
	m_capturethread = std::thread(&FFmpegVideoCapture::CaptureThread, this);
	return true;
}

void FFmpegVideoCapture::Destroy()
{
	//RTC_LOG(LS_INFO) << "LiveVideoSource::stop";
	source->SyncStop();
	m_capturethread.join();
	m_YuvCallbackList.clear();
	m_h264Callback = nullptr;
	Stop(NULL);
}

void FFmpegVideoCapture::Stop(VideoYuvCallBack yuvCallback)
{
	bool bEmpty = false;

	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			m_YuvCallbackList.erase(it);
			break;
		}
		it++;
	}
	if (m_YuvCallbackList.empty())
	{
		bEmpty = true;
	}

}

void FFmpegVideoCapture::Init(const char* devicename, int nWidth, int nHeight, int nFrameRate)
{

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFrameRate = nFrameRate;
}

void FFmpegVideoCapture::Init(std::map<std::string, std::string> opts)
{
	if (opts.find("width") != opts.end())
	{
		m_nWidth = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		m_nHeight = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		m_nFrameRate = std::stoi(opts.at("fps"));
	}
}

void FFmpegVideoCapture::RegisterCallback(VideoYuvCallBack yuvCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			m_mutex.unlock();
			return;
		}
		it++;
	}
	m_YuvCallbackList.push_back(yuvCallback);
}

void FFmpegVideoCapture::CaptureThread()
{

	source = MediaSourceAPI::CreateMediaSource(m_videourl, this, true, true);
	source->Run(false);
}

bool FFmpegVideoCapture::onData(const char* id, unsigned char* buffer, int size, int64_t ts)
{
	m_h264Callback((char*)buffer, size, 0, m_nWidth, m_nHeight, ts);
	return false;
}

bool FFmpegVideoCapture::onData(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)
{
	for (auto callback: m_YuvCallbackList)
	{
		callback(y, strideY, u, strideU, v, strideV, nWidth, nHeight, nTimeStamp);
	}
	return false;
}

void FFmpegVideoCapture::OnSourceConnected(void* arg, const std::map<std::string, std::string>& opts)
{
	if (m_bStop.load() == true)
	{
		return;
	}
	m_bStop.store(true);
	std::map<std::string, std::string> streamInfo = opts;

	// 访问视频流信息
	if (streamInfo.count("videocodecname") > 0)
	{
		std::string videoCodecName = streamInfo["videocodecname"];
		std::string width = streamInfo["width"];
		std::string height = streamInfo["height"];
		std::string bitrate = streamInfo["bitrate"];
		std::string framerate = streamInfo["framerate"];
		decoder = FFmpegDecoderAPI::CreateDecoder();
		FrameInfo _info;
		memset(&_info, 0, sizeof(FrameInfo));
		_info.FramesPerSecond = 30;
		if (videoCodecName == "hevc")
		{
			_info.VCodec = CAREYE_CODEC_H265;
		}
		if (videoCodecName == "h264")
		{
			_info.VCodec = CAREYE_CODEC_H264;
		}
		_info.DecType = DecodeType::kNvencDecode;
		_info.Width = atoi(width.c_str());
		m_nWidth = _info.Width;
		_info.Height = atoi(height.c_str());
		m_nHeight = _info.Height;
		decoder->createDecoder(_info);
		decoder->Start();

		std::string cfg_file = R"(D:\study\yolo_test\x64\Release\cfg\yolov4.cfg)";
		std::string weights_file = R"(D:\study\yolo_test\x64\Release\backup/yolov4.weights)";
		auto bres = built_with_cuda();
		my_detector = new Detector(cfg_file, weights_file, 0);
		m_ffmpegEncoder = FFmpegVideoEncoderAPI::CreateEncoder();

		m_listnames = gbldarknetToolsGet->readLinesToList(R"(D:\study\yolo_test\x64\Release\data/coco.names)");
		m_ffmpegEncoder->Init("h264_nvenc", CAREYE_FMT_YUV420P, _info.Width, _info.Height, 30, _info.Width * _info.Height * 3);
		decoder->RegisterDecodeCallback([=](uint8_t* yuv, int nWidth, int nHeight, int strideY, int strideU, int strideV, bool bKey, int64_t nTimeStamp)
			{

				if (m_detector == 0)
				{
					m_detector++;
					cv::Mat image = gbldarknetToolsGet->convertYUVtoMat(yuv, nWidth, nHeight);
					auto image_ptr = my_detector->mat_to_image(image);
					// //设置yolo阈值
					float thresh = 0.5;
					//yolo检测			
					std::vector<bbox_t> result_vec = my_detector->detect(*(image_ptr.get()), thresh);
					int num = 0, car_num = 0;
					//遍历bbox_t结构体
					for (std::vector<bbox_t>::iterator iter = result_vec.begin(); iter != result_vec.end(); iter++)
					{
						//画检测框
						cv::Rect rect(iter->x, iter->y, iter->w, iter->h);
						cv::rectangle(image, rect, cv::Scalar(255, 0, 0), 2);
						// 设置文字参数
						if (iter->obj_id == 0)
						{
							num++;
						}
						if (iter->obj_id == 2)
						{
							car_num++;
						}
						std::string text = gbldarknetToolsGet->get_line_value(m_listnames, iter->obj_id + 1);
						cv::Point position(iter->x, iter->y); // 文字位置
						cv::Scalar color(0, 0, 255); // 文字颜色，这里使用红色
						int fontFace = cv::FONT_HERSHEY_SIMPLEX; // 字体样式
						double fontScale = 1.0; // 字体缩放系数
						int thickness = 2; // 文字线条粗细
						// 添加文字到图像
						cv::putText(image, text, position, fontFace, fontScale, color, thickness);
					}
					uint8_t* newyuv = new uint8_t[nWidth * nHeight * 3 / 2];
					int nw, nh;
					gbldarknetToolsGet->convertMatToYUV420(image, newyuv, nWidth, nHeight);
					unsigned char* pOutEncodeBuffer = new unsigned char[nWidth * nHeight];
					int            nOneFrameLength = 0;
					std::string stdfilter = "person num = " + std::to_string(num) + "  car num = " + std::to_string(car_num);
					//m_ffmpegEncoder->ChangeVideoFilter(stdfilter.c_str(), 30, (char*)"red", 0.85, 50, 50);
					m_ffmpegEncoder->EncodecYUV(newyuv, nWidth * nHeight * 3 / 2, pOutEncodeBuffer, &nOneFrameLength);

					if (nOneFrameLength > 0)
					{

						m_h264Callback((char*)pOutEncodeBuffer, nOneFrameLength, 0, nWidth, nHeight, 0);

					}
					delete[]newyuv;
					newyuv = nullptr;

					delete[]pOutEncodeBuffer;
					pOutEncodeBuffer = nullptr;

				}
				else
				{

					m_detector++;
					if (m_detector > 40)
					{
						m_detector = 0;
					}
					unsigned char* pOutEncodeBuffer = new unsigned char[nWidth * nHeight];
					int            nOneFrameLength = 0;
					m_ffmpegEncoder->EncodecYUV(yuv, nWidth * nHeight * 3 / 2, pOutEncodeBuffer, &nOneFrameLength);
					if (nOneFrameLength > 0)
					{

						m_h264Callback((char*)pOutEncodeBuffer, nOneFrameLength, 0, nWidth, nHeight, 0);
					}
					delete[]pOutEncodeBuffer;
					pOutEncodeBuffer = nullptr;

				}


			}

		);
	}

	// 访问音频流信息
	if (streamInfo.count("audiocodecname") > 0) {
		std::string audioCodecName = streamInfo["audiocodecname"];
		std::string channels = streamInfo["channels"];
		std::string samplerate = streamInfo["samplerate"];
	}

}

void FFmpegVideoCapture::OnSourceVideoPacket(const char* id, uint8_t* aBytes, int aSize, int64_t ts)
{
	std::string strid = id;
	if (strid.find("H264") != std::string::npos)
	{
		std::lock_guard<std::mutex> _lock(m_mutex);
		if (decoder)
		{
			decoder->PostFrame(aBytes, aSize, true, 0);
		}
		//	std::this_thread::sleep_for(std::chrono::milliseconds(30));
			//m_h264Callback((char*)aBytes, aSize, true, m_nWidth, m_nHeight, ts);
	}
	if (strid.find("H265") != std::string::npos)
	{
		if (decoder)
		{
			decoder->PostFrame(aBytes, aSize, true, 0);
		}



	}




}
