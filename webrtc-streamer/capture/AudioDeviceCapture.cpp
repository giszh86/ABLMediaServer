
#include "AudioDeviceCapture.h"

#include "libyuv.h"
#include "rtc_base/thread.h"
#include "pc/video_track_source.h"
#include "api/video/i420_buffer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"

#include "modules/audio_device/include/audio_device.h"
#include "api/task_queue/default_task_queue_factory.h"

#ifdef _WIN32
static std::string GbkToUtf8(const char* src_str)
{
	int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	std::string strTemp = str;
	if (wstr) delete[] wstr;
	if (str) delete[] str;
	return strTemp;
}
static std::string Utf8ToGbk(const char* src_str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}
#endif
AudioDeviceCapture::AudioDeviceCapture()
{
	thread = rtc::Thread::Create();
	thread->Start();
}

AudioDeviceCapture::~AudioDeviceCapture()
{
	thread->Stop();
}
int AudioDeviceCapture::Init(int nSampleRate, int nChannel, int nBitsPerSample)
{
	m_nSampleRate = nSampleRate;
	m_nChannel = nChannel;
	m_nBitsPerSample = nBitsPerSample;
	return 0;
}
int AudioDeviceCapture::Init(const std::map<std::string, std::string>& opts)
{
	return 0;
}
int AudioDeviceCapture::Start()
{
	thread->Invoke<void>(
		RTC_FROM_HERE,
		std::bind(&AudioDeviceCapture::StartInternal,
			this));
	return 0;
}
#ifdef _WIN32
void CALLBACK waveInProc(HWAVEIN hwi,
	UINT uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2)
{
	LPWAVEHDR pwh = (LPWAVEHDR)dwParam1;
	AudioDeviceCapture* pAudioCaptureImpl = (AudioDeviceCapture*)dwInstance;

	if ((WIM_DATA == uMsg))
	{
		if (pAudioCaptureImpl->IsStop())
		{
			return;
		}
		MMRESULT mmres = waveInUnprepareHeader(hwi, pwh, sizeof(WAVEHDR));
		pAudioCaptureImpl->OnPcmData((void*)pwh->lpData, pwh->dwBufferLength);
		mmres = waveInPrepareHeader(hwi, pwh, sizeof(WAVEHDR));
		waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
	}
	else if (WIM_CLOSE == uMsg)
	{

	}
}
#endif
void AudioDeviceCapture::StartInternal()
{
	m_StartTime = 0;
	m_bStart = true;
	m_bStop = false;
#if AUDIOCAP_WEBRTC
	std::unique_ptr<webrtc::TaskQueueFactory>   task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();

	audio_device_ = webrtc::AudioDeviceModule::CreateForTest(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory.get());
	audio_device_->RegisterAudioCallback(this);
	webrtc::AudioDeviceModule::AudioLayer audio_layer;
	int got_platform_audio_layer = audio_device_->ActiveAudioLayer(&audio_layer);
	// First, ensure that a valid audio layer can be activated.
	if (got_platform_audio_layer != 0) {
		requirements_satisfied_ = false;
	}

	if (audio_device_->Init() != 0)
	{
		return;
	}
	// 		audio_device_->SetPlayoutDevice(webrtc::AudioDeviceModule::kDefaultCommunicationDevice);
	// 		if (audio_device_->InitSpeaker() != 0)
	// 		{
	// 			return;
	// 		}
	if (m_strDevicename.empty())
	{
		audio_device_->SetRecordingDevice(webrtc::AudioDeviceModule::kDefaultDevice);
	}
	else
	{
		char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char id[webrtc::kAdmMaxGuidSize] = { 0 };
		int16_t idx_audioDevice = -1;
		int16_t num_record_devices = audio_device_->RecordingDevices();
		for (int i = 0; i < num_record_devices; ++i)
		{
			if (audio_device_->RecordingDeviceName(i, name, id) != -1)
			{
#ifdef _WIN32		
				if (m_strDevicename == Utf8ToGbk(name) || m_strDevicename == name || Utf8ToGbk(m_strDevicename.c_str()) == Utf8ToGbk(name))
				{
					idx_audioDevice = i;
					audio_device_->SetRecordingDevice(idx_audioDevice);
					break;
				}
#else
				if (m_strDevicename == name)
				{
					idx_audioDevice = i;
					audio_device_->SetRecordingDevice(idx_audioDevice);
					break;
				}
#endif

			}
		}

	}

	if (audio_device_->InitMicrophone() != 0)
	{
		return;
	}
	bool available = false;
	//audio_device_->StereoPlayoutIsAvailable(&available);
	//audio_device_->SetStereoPlayout(available);
	audio_device_->StereoRecordingIsAvailable(&available);
	audio_device_->SetStereoRecording(available);	
	audio_device_->SetRecordingSampleRate(m_nSampleRate);
	if (m_nChannel == 1)
	{
		//audio_device_->SetRecordingChannel(webrtc::AudioDeviceModule::kChannelLeft);
	}
	else
	{
	//	audio_device_->SetRecordingChannel(webrtc::AudioDeviceModule::kChannelBoth);
	}
	audio_device_->InitRecording();
	audio_device_->StartRecording();
	//audio_device_->SetAGC(true);
	//audio_device_->EnableBuiltInAEC(true);
	//audio_device_->EnableBuiltInAGC(true);
	//audio_device_->EnableBuiltInNS(true);
	audio_device_->SetMicrophoneVolume(255);
#else
	int nReturn = waveInGetNumDevs();
	WAVEINCAPS wic;
	for (int i = 0; i < nReturn; i++)
	{
		waveInGetDevCaps(i, &wic, sizeof(WAVEINCAPS));
	}
	// open   


	wavform.wFormatTag = WAVE_FORMAT_PCM;
	wavform.nChannels = m_nChannel;
	wavform.nSamplesPerSec = m_nSampleRate;
	wavform.wBitsPerSample = m_nBitsPerSample;
	wavform.nAvgBytesPerSec = m_nSampleRate * wavform.wBitsPerSample * m_nChannel / 8;
	wavform.nBlockAlign = wavform.wBitsPerSample * m_nChannel / 8;
	wavform.cbSize = 0;
	m_nBytesPerSample = wavform.wBitsPerSample / 8;
	waveInOpen(&hWaveIn, WAVE_MAPPER, &wavform, (DWORD_PTR)waveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);

	//WAVEINCAPS wic;
	waveInGetDevCaps((UINT_PTR)hWaveIn, &wic, sizeof(WAVEINCAPS));
	int nPCMBufferSize = 1024 * wavform.nBlockAlign;

	for (int i = 0; i < FRAGMENT_NUM; i++)
	{
		wh[i].lpData = new char[nPCMBufferSize];
		memset(wh[i].lpData, 0xff, nPCMBufferSize);
		wh[i].dwBufferLength = nPCMBufferSize;
		wh[i].dwBytesRecorded = 0;
		wh[i].dwUser = NULL;
		wh[i].dwFlags = 0;
		wh[i].dwLoops = 1;
		wh[i].lpNext = NULL;
		wh[i].reserved = 0;

		waveInPrepareHeader(hWaveIn, &wh[i], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &wh[i], sizeof(WAVEHDR));
	}
	waveInStart(hWaveIn);
#endif
}
void AudioDeviceCapture::Stop()
{
	m_PcmCallBack = NULL;
	thread->Invoke<void>(
		RTC_FROM_HERE,
		std::bind(&AudioDeviceCapture::StopInternal,
			this));
}
void AudioDeviceCapture::StopInternal()
{
	m_bStart = false;
	m_bStop = true;
#if AUDIOCAP_WEBRTC
	if (audio_device_)
	{
		audio_device_->StopRecording();
		audio_device_.release();
	}
#else
	m_PcmCallBack = NULL;
	
	waveInStop(hWaveIn);

	waveInReset(hWaveIn);
	
	for (int i = 0; i < FRAGMENT_NUM; i++)
	{
		waveInUnprepareHeader(hWaveIn, &wh[i], sizeof(WAVEHDR));
		if (wh[i].lpData)
		{
			delete[] wh[i].lpData;
			wh[i].lpData = NULL;
		}

	}

	waveInClose(hWaveIn);


#endif


}

int32_t AudioDeviceCapture::RecordedDataIsAvailable(const void* audioSamples,
	const size_t nSamples,
	const size_t nBytesPerSample,
	const size_t nChannels,
	const uint32_t samplesPerSec,
	const uint32_t totalDelayMS,
	const int32_t clockDrift,
	const uint32_t currentMicLevel,
	const bool keyPressed,
	uint32_t& newMicLevel)
{
	m_nBytesPerSample = nBytesPerSample;
	size_t number_of_bytes = nBytesPerSample * nSamples * nChannels; //assuming bits_per_sample is 16      

	//LOG_INFO("nSamples:%d nBytesPerSample:%d nChannels:%d samplesPerSec:%d totalDelayMS:%d clockDrift:%d currentMicLevel:%d keyPressed:%d newMicLevel:%d ", nSamples,nBytesPerSample, nChannels, samplesPerSec, totalDelayMS, clockDrift, currentMicLevel, keyPressed,newMicLevel);
	newMicLevel = 100;
	if (m_PcmCallBack)
	{
		m_PcmCallBack((uint8_t*)audioSamples, number_of_bytes, samplesPerSec, nChannels, rtc::TimeMillis());
	}
	return 1;
}
void AudioDeviceCapture::OnPcmData(void* audioSamples, int datalen)
{
	if (m_PcmCallBack)
	{
		m_PcmCallBack((uint8_t*)audioSamples, datalen, m_nSampleRate, m_nChannel, rtc::TimeMillis());
	}
}
int32_t AudioDeviceCapture::NeedMorePlayData(const size_t nSamples,
	const size_t nBytesPerSample,
	const size_t nChannels,
	const uint32_t samplesPerSec,
	void* audioSamples,
	size_t& nSamplesOut,
	int64_t* elapsed_time_ms,
	int64_t* ntp_time_ms)
{
	return 1;
}

// Method to push the captured audio data to the specific VoE channel.
// The data will not undergo audio processing.
// |voe_channel| is the id of the VoE channel which is the sink to the
// capture data.
void AudioDeviceCapture::PushCaptureData(int voe_channel,
	const void* audio_data,
	int bits_per_sample,
	int sample_rate,
	size_t number_of_channels,
	size_t number_of_frames)
{

}

// Method to pull mixed render audio data from all active VoE channels.
// The data will not be passed as reference for audio processing internally.
// TODO(xians): Support getting the unmixed render data from specific VoE
// channel.
void AudioDeviceCapture::PullRenderData(int bits_per_sample,
	int sample_rate,
	size_t number_of_channels,
	size_t number_of_frames,
	void* audio_data,
	int64_t* elapsed_time_ms,
	int64_t* ntp_time_ms)
{

}
void AudioDeviceCapture::RegisterPcmCallback(std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallback)
{
	m_PcmCallBack = PcmCallback;
}

void AudioDeviceCapture::RegisterAacCallback(std::function<void(uint8_t* aac_raw, int file_size, int64_t nTimeStamp)> aacCallBack)
{
}

void AudioDeviceCapture::Destroy()
{
}

std::string AudioDeviceCapture::GetAudioConfig()
{
	return std::string();
}
