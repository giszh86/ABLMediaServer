#include "../../3rd/glog/include/glog/logging.h"
#include <thread>
#include <iostream>
#include "FFmpegCapture.h"


#ifdef __cplusplus
extern"C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libavutil/fifo.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avstring.h"
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/timestamp.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#ifdef __cplusplus
};
#endif


FFmpegCapture::FFmpegCapture(std::string url) :m_url(url)
{
	/*if (access(m_url.c_str(), F_OK) != 0)
	{
		std::cout << "not find file:" << m_url << std::endl;
		return;
	}	*/
}
FFmpegCapture::~FFmpegCapture()
{
	
}
bool FFmpegCapture::Start()
{	
	if (m_url.empty())
		return false;
	std::vector<std::string> inUrls;
	inUrls.push_back(m_url);
	std::unique_ptr<FF_Capturer> ptr(new FF_Capturer());
	m_ptr_capture = std::move(ptr);
	m_ptr_capture->Init(inUrls);
	m_ptr_capture->RegisterVideoCallback(std::bind(&FFmpegCapture::OnVideo, this, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3,
		std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
	m_ptr_capture->RegisterAudioCallback(std::bind(&FFmpegCapture::OnAudio, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
		std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
	m_ptr_capture->Start(0);
	return true;
}
void FFmpegCapture::Init(const std::map<std::string, std::string>& opts)
{
	return;
}
void FFmpegCapture::Stop()
{
	if (m_ptr_capture.get())
	{
		m_ptr_capture->Stop();
	}
}
void FFmpegCapture::RegisterVideoCallBack(VideoCllBack videoCallback)
{	
	if (videoCallback)
	{
		m_callback_video = videoCallback;
	}
}
void FFmpegCapture::RegisterAudioCallBack(AudioCallBack audioCallback)
{
	if (audioCallback)
	{
		m_callback_audio = audioCallback;
	}
}
void FFmpegCapture::OnVideo(uint8_t* raw_data, const char* codecid, int raw_len, bool bKey, int nWidth, int nHeight, int64_t nTimeStamp)
{
	if (m_callback_video)
	{
		m_callback_video(raw_data, codecid, raw_len, bKey, nWidth, nHeight, nTimeStamp);
	}
}
void FFmpegCapture::OnAudio(uint8_t* raw_data, const char* codecid,int raw_len, int channels, int sample_rate, int bytes, int64_t nTimeStamp)
{
	if (m_callback_audio)
	{
		m_callback_audio(raw_data, codecid,raw_len, channels, sample_rate, bytes, nTimeStamp);
	}
}
