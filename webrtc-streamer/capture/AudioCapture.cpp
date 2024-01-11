

#include "AudioCapture.h"
//#include "AudioDeviceCapture.h"
//#include "RtspAudioCapture.h"

#include <thread>

#ifdef WEBRTC_WIN
//#include "AudioRecordWas.h"
//#include "device_audios.h"
#endif
AudioCapture* AudioCapture::CreateAudioCapture(std::string audiourl, const std::map<std::string, std::string> opts)
{

	if ((audiourl.find("rtsp://") == 0))
	{
	
		//return new RtspAudioCapture(audiourl, opts);
	}
	else if ((audiourl.find("file://") == 0))
	{
		//return new RtspCaptureImpl(audiourl, opts);
	}

#ifdef WEBRTC_WIN
	else if ((audiourl.find("audiocap://") == 0))
	{
		//std::string out_name = audiourl.substr(strlen("audiocap://")).c_str();
	//	return new AudioRecordWas(out_name, opts);
	}
	else
	{
		//return new AudioRecordWas(audiourl, opts);
	}
#endif
	return nullptr;
}