#pragma once
#include <modules/audio_device/include/audio_device.h>
#include "AudioCapture.h"
#include <rtc_base/thread.h>
#include <thread>
#ifdef _WIN32
#include <mmsystem.h>  
#include <mmeapi.h>
#include <objbase.h>
#endif

#define  AUDIOCAP_WEBRTC 1
#define BUFFER_SIZE (44100*16*2/8*5)    // 录制声音长度  
#define FRAGMENT_SIZE 2048              // 缓存区大小  
#define FRAGMENT_NUM 20                  // 缓存区个数 
class  AudioDeviceCapture:public webrtc::AudioTransport,public AudioCapture
{
public:
	AudioDeviceCapture();
	~AudioDeviceCapture();



public:
	
	void StartInternal();

	void setDevicename(std::string devicename) { m_strDevicename = devicename; };

	bool IsStop()
	{
		return m_bStop;
	}
	void StopInternal();
	
public://  for webrtc::AudioTransport
	virtual int32_t RecordedDataIsAvailable(const void* audioSamples,
			const size_t nSamples,
			const size_t nBytesPerSample,
			const size_t nChannels,
			const uint32_t samplesPerSec,
			const uint32_t totalDelayMS,
			const int32_t clockDrift,
			const uint32_t currentMicLevel,
			const bool keyPressed,
			uint32_t& newMicLevel);

	virtual int32_t NeedMorePlayData(const size_t nSamples,
			const size_t nBytesPerSample,
			const size_t nChannels,
			const uint32_t samplesPerSec,
			void* audioSamples,
			size_t& nSamplesOut,
			int64_t* elapsed_time_ms,
			int64_t* ntp_time_ms);

			// Method to push the captured audio data to the specific VoE channel.
			// The data will not undergo audio processing.
			// |voe_channel| is the id of the VoE channel which is the sink to the
			// capture data.
	virtual void PushCaptureData(int voe_channel,
				const void* audio_data,
				int bits_per_sample,
				int sample_rate,
				size_t number_of_channels,
				size_t number_of_frames);

		// Method to pull mixed render audio data from all active VoE channels.
		// The data will not be passed as reference for audio processing internally.
		// TODO(xians): Support getting the unmixed render data from specific VoE
		// channel.
	virtual void PullRenderData(int bits_per_sample,
			int sample_rate,
			size_t number_of_channels,
			size_t number_of_frames,
			void* audio_data,
			int64_t* elapsed_time_ms,
			int64_t* ntp_time_ms);
	void OnPcmData(void* audioSamples, int datalen);



public://  for AudioCapture
	virtual int Init(int nSampleRate, int nChannel, int nBitsPerSample) override;

	virtual int Init(const std::map<std::string, std::string>& opts)override;

	virtual int Start() override;

	virtual void Stop() override;

	virtual void RegisterPcmCallback(std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallback) override;

	virtual void RegisterAacCallback(std::function<void(uint8_t* aac_raw, int file_size, int64_t nTimeStamp)> aacCallBack) override;

	virtual void Destroy() override;

	std::string GetAudioConfig();


private:
	rtc::scoped_refptr<webrtc::AudioDeviceModuleForTest> audio_device_;
	bool requirements_satisfied_ = false;
	int m_nSampleRate = 48000;
	int m_nChannel = 2;
	int m_nBitsPerSample;
	int m_nBytesPerSample;
	bool m_bStart = false;
	bool m_bStop = false;
	std::unique_ptr<rtc::Thread> thread;
	std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel,int64_t nTimeStamp)> m_PcmCallBack;
	int64_t m_StartTime = 0;
	std::string m_strDevicename;
#ifdef _WIN32
	WAVEFORMATEX wavform;
	WAVEHDR wh[FRAGMENT_NUM];
	HWAVEIN hWaveIn;
#endif
};