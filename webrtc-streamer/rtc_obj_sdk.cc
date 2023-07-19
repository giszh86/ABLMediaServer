

#include <map>
#include "rtc_obj_sdk.h"
#include "WebRtcEndpointImpl.h"

#include "rtc_base/checks.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/win32_socket_init.h"
#include "system_wrappers/include/field_trial.h"
#include "test/field_trial.h"
#include <libyuv.h>
#include <modules/video_capture/video_capture_factory.h>
#include <modules/desktop_capture/win/screen_capture_utils.h>
#include <modules/desktop_capture/desktop_capture_options.h>
#include <modules/desktop_capture/desktop_capturer.h>

#include "modules/audio_device/include/audio_device.h"
#include "common_audio/resampler/include/resampler.h"
#include "common_audio/vad/include/webrtc_vad.h"


#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/physical_socket_server.h"
#ifdef WEBRTC_WIN
#include "modules/audio_device/include/audio_device_factory.h"
#include "modules/audio_device/win/core_audio_utility_win.h"
#endif



std::map<const WebRtcEndpointImpl*, std::unique_ptr<WebRtcEndpointImpl>> LinkMicManagerImpllist;

WebRtcEndpoint* CreateLinkMicManager(MainWndCallback* main_wnd_)
{	
	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;

		rtc::WinsockInitializer winsock_init;
		rtc::PhysicalSocketServer ss;
		rtc::AutoSocketServerThread main_thread(&ss);

		rtc::LogMessage::LogTimestamps();
		rtc::LogMessage::LogThreads();
		rtc::LogMessage::AddLogToStream(new RtcLogSink(), rtc::LS_ERROR);
		
		rtc::InitializeSSL();
		//auto thread = rtc::Thread::Current();	
	//	rtc::ThreadManager::Instance()->SetCurrentThread(thread);

	


		std::string webrtcTrialsFields = "WebRTC-FrameDropper/Disabled/";
		webrtc::field_trial::InitFieldTrialsFromString(webrtcTrialsFields.c_str());
	}
	WebRtcEndpointImpl *pnew = new WebRtcEndpointImpl(main_wnd_);
	std::unique_ptr<WebRtcEndpointImpl> LinkMicManagerImpl_ = std::unique_ptr<WebRtcEndpointImpl>(pnew);
	WebRtcEndpoint* pret = pnew;
	LinkMicManagerImpllist[pnew].swap(LinkMicManagerImpl_);
	return pret;
}

void ReleaseLinkMicManager(WebRtcEndpoint* pwnd)
{
	if (pwnd)
	{
		WebRtcEndpointImpl* p = (WebRtcEndpointImpl*)pwnd;
		p->Release();
		LinkMicManagerImpllist.erase(p);
		p = nullptr;
		pwnd = nullptr;
		//rtc::CleanupSSL();
	}
	

}

RtmpPushMgr* CreateRtmpPushManager(RtmpPushMgrListen*)
{
	RtmpPushMgr* pRtmp = RtmpPushMgr::CreateRtmpPushMgr();
	return pRtmp;
}

void ReleaseRtmpPushManager(RtmpPushMgr* pRtmp)
{
	if (pRtmp)
	{
		delete pRtmp;
		pRtmp = nullptr;
	}
	return ;
}

std::list<std::string> GetVideoSourceList()
{
	std::string publishFilter(".*");
	const std::regex  m_publishFilter(publishFilter);
	const std::list<std::string> videoCaptureDevice = CapturerFactory::GetVideoSourceList(m_publishFilter, false);
	return videoCaptureDevice;

}

BOOL GetWindowListHandler(HWND hwnd, LPARAM param)
{
	std::map<int, std::wstring>* videoCaptureDevice= (std::map<int, std::wstring> *)(param);

	// Skip invisible and minimized windows
	if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
		return TRUE;
	}
	// Skip untitled window if ignoreUntitled specified
	if (GetWindowTextLength(hwnd) == 0) {
		return TRUE;
	}
	// Skip windows which are not presented in the taskbar,
	// namely owned window if they don't have the app window style set
	HWND owner = GetWindow(hwnd, GW_OWNER);
	LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	if (owner && !(exstyle & WS_EX_APPWINDOW)) {
		return TRUE;
	}


	// Capture the window class name, to allow specific window classes to be
	// skipped.
	//
	// https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
	// says lpszClassName field in WNDCLASS is limited by 256 symbols, so we don't
	// need to have a buffer bigger than that.
	const size_t kMaxClassNameLength = 256;
	WCHAR class_name[kMaxClassNameLength] = L"";
	const int class_name_length =
		GetClassNameW(hwnd, class_name, kMaxClassNameLength);
	if (class_name_length < 1)
		return TRUE;

	// Skip Program Manager window.
	if (wcscmp(class_name, L"Progman") == 0)
		return TRUE;

	// Skip Start button window on Windows Vista, Windows 7.
	// On Windows 8, Windows 8.1, Windows 10 Start button is not a top level
	// window, so it will not be examined here.
	if (wcscmp(class_name, L"Button") == 0)
		return TRUE;

	int nId = reinterpret_cast<int>(hwnd);
	const size_t kTitleLength = 500;
	WCHAR window_title[kTitleLength] = L"";
	if (GetWindowTextW(hwnd, window_title, kTitleLength) > 0)
	{	
		(*videoCaptureDevice)[nId]= std::wstring(window_title);		
	}
	return TRUE;
}

std::list<std::string> getVideoDeviceList()
{
	auto _worker_thread_ptr = rtc::Thread::Create();
	if (_worker_thread_ptr)
	{
		_worker_thread_ptr->Start();
		std::list<std::string> device_names_;
		device_names_ = _worker_thread_ptr->Invoke<std::list<std::string>>(RTC_FROM_HERE, [&]()
			{
				std::string publishFilter(".*");
				const std::regex  m_publishFilter(publishFilter);
				const std::list<std::string> videoCaptureDevice = CapturerFactory::GetVideoCaptureDeviceList(m_publishFilter,false);
				return videoCaptureDevice;
			});

		return device_names_;
	}
	else
	{
		auto main_thread = rtc::Thread::Current();//不需要执行thread_->start()；
		std::list<std::string> device_names_;
		device_names_ = main_thread->Invoke<std::list<std::string>>(RTC_FROM_HERE, [&]()
			{
				std::string publishFilter(".*");
				const std::regex  m_publishFilter(publishFilter);
				const std::list<std::string> videoCaptureDevice = CapturerFactory::GetVideoCaptureDeviceList(m_publishFilter,false);
				return videoCaptureDevice;
			});

		return device_names_;
	}
}

std::map<int, std::string> GetVideoSourceMap()
{
	std::map<int, std::string>videoCaptureDevice = CapturerFactory::GetVideoCaptureDeviceMap();
	return videoCaptureDevice;
	//std::atomic<bool> bRun=true;
	//std::map<int, std::string> lstName;
	//auto main_thread = rtc::Thread::Create();
	//main_thread->Start();
	//if (main_thread ==nullptr)
	//{
	//	auto _worker_thread_ptr = rtc::Thread::Current();//不需要执行thread_->start()；
	//	
	//	 _worker_thread_ptr->PostTask(RTC_FROM_HERE, [&]
	//		{
	//			std::map<int, std::string>videoCaptureDevice = CapturerFactory::GetVideoCaptureDeviceMap();			
	//			lstName = videoCaptureDevice;
	//			bRun.store(false);
	//			return ;
	//		});	
	//}
	//else
	//{	
	//	main_thread->PostTask(RTC_FROM_HERE, [&]
	//		{
	//			std::map<int, std::string>videoCaptureDevice = CapturerFactory::GetVideoCaptureDeviceMap();		
	//			lstName = videoCaptureDevice;
	//			bRun.store(false);
	//			return;
	//		});
	//
	//}
	//while (bRun.load())
	//{
	//	Sleep(100);
	//}

	//return lstName;
}
//
//void GetAudioList()
//{
//	shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
//	_worker_thread_ptr->Start();
//	uint32_t vol2 = 100;
//	bool is_mute2 = true;
//  //创建AudioDeviceModule指针变量
//	rtc::scoped_refptr<webrtc::AudioDeviceModule> _adm_ptr =
//		_worker_thread_ptr->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule>>(RTC_FROM_HERE, [] 
//			{
//				//create adm
//				std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
//				return webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory.get());
//			});
//	_adm_ptr->Init();
//	int num = 0;
//	int ret = 0;
//	num = _adm_ptr->RecordingDevices();  //列举麦克风设备数量
//
//	bool can_mute = false;
//	bool can_vol = false;
//	if (num <= 0)
//	{
//		return;
//	}
//	//枚举麦克风设备
//	for (int i = 0; i < num; i++)
//	{
//		char name[webrtc::kAdmMaxDeviceNameSize];
//		char guid[webrtc::kAdmMaxGuidSize];
//		int ret = _adm_ptr->RecordingDeviceName(i, name, guid);
//		if (ret != -1) {
//			LOG_INFO("麦克风:%s\n", name);
//		}
//	}
//	ret = _adm_ptr->SetRecordingDevice(0);  //选择麦克风
//
//	_adm_ptr->MicrophoneMuteIsAvailable(&can_mute);
//
//	_adm_ptr->MicrophoneVolumeIsAvailable(&can_vol);
//
//	if (can_vol)
//		_adm_ptr->SetMicrophoneVolume(vol2);  //设置音量
//	else
//		LOG_INFO("麦克风音量调节不可用\n");
//	if (can_mute)
//		_adm_ptr->SetMicrophoneMute(is_mute2);  //设置是否静音
//	else
//		LOG_INFO("麦克风静音不可用\n");
//	_adm_ptr->InitRecording();
//}

 std::list<std::string> getRecordingDeviceList()
{
	 std::list<std::string> device_names_;
	 auto _worker_thread_ptr = rtc::Thread::Create();
	 _worker_thread_ptr->Start();
	 //创建AudioDeviceModule指针变量
	 device_names_ = _worker_thread_ptr->Invoke<std::list<std::string> >(RTC_FROM_HERE, [&]()
		 {
			 std::list<std::string> device_names;
			 //create adm
			 std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
			 rtc::scoped_refptr<webrtc::AudioDeviceModule> _adm_ptr = webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory.get());

			 if (_adm_ptr == nullptr)
			 {
				 return device_names;
			 }
			 _adm_ptr->Init();
			 int num_audioDevices = _adm_ptr->RecordingDevices();  //列举麦克风设备数量
			 if (num_audioDevices < 1)
			 {
				 return device_names;
			 }
			 //枚举麦克风设备
			 for (int i = 0; i < num_audioDevices; i++)
			 {
				 char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
				 char guid[webrtc::kAdmMaxGuidSize] = { 0 };
				 int ret = _adm_ptr->RecordingDeviceName(i, name, guid);
				 if (ret != -1)
				 {
					 device_names.push_back(name);
				 }
			 }
			 return device_names;
		 });
	 return device_names_;
}

 std::list<std::string> getPlayoutDeviceList()
 {
	 std::list<std::string> device_names_;
	 auto _worker_thread_ptr = rtc::Thread::Create();
	 _worker_thread_ptr->Start();
	 //创建AudioDeviceModule指针变量
	 device_names_ = _worker_thread_ptr->Invoke<std::list<std::string> >(RTC_FROM_HERE, [&]()
		 {
			 std::list<std::string> device_names;
			 //create adm
			 std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
			 if (task_queue_factory ==nullptr)
			 {
				 return device_names;
			 }
			 rtc::scoped_refptr<webrtc::AudioDeviceModule> _adm_ptr = webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory.get());
			 if (_adm_ptr == nullptr)
			 {
				 return device_names;
			 }

			 _adm_ptr->Init();
			 int num_audioDevices = _adm_ptr->PlayoutDevices();  //列举麦克风设备数量
			 if (num_audioDevices <= 0)
			 {
				 return device_names;
			 }
	
			 //枚举麦克风设备
			 for (int i = 0; i < num_audioDevices; i++)
			 {
				 char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
				 char guid[webrtc::kAdmMaxGuidSize] = { 0 };
				 int ret = _adm_ptr->PlayoutDeviceName(i, name, guid);
				 if (ret != -1)
				 {
					 device_names.push_back(name);
				 }
			 }
			 return device_names;
		 });
	 return device_names_;
 }



 int ws_I420ToARGB(const unsigned char* src_y,
	 int src_stride_y,
	 const unsigned char* src_u,
	 int src_stride_u,
	 const unsigned char* src_v,
	 int src_stride_v,
	 unsigned char* dst_argb,
	 int dst_stride_argb,
	 int width,
	 int height)
 {
	 return libyuv::I420ToARGB(src_y, src_stride_y, src_u, src_stride_u, src_v, src_stride_v, dst_argb, dst_stride_argb, width, height);
 }
 int ws_I420Copy(const unsigned char* src_y,
	 int src_stride_y,
	 const unsigned char* src_u,
	 int src_stride_u,
	 const unsigned char* src_v,
	 int src_stride_v,
	 unsigned char* dst_y,
	 int dst_stride_y,
	 unsigned char* dst_u,
	 int dst_stride_u,
	 unsigned char* dst_v,
	 int dst_stride_v,
	 int width,
	 int height)
 {
	 return libyuv::I420Copy(src_y,
		 src_stride_y,
		 src_u,
		 src_stride_u,
		 src_v,
		 src_stride_v,
		 dst_y,
		 dst_stride_y,
		 dst_u,
		 dst_stride_u,
		 dst_v,
		 dst_stride_v,
		 width,
		 height);
 }
 int ws_I420Scale(const unsigned char* src_y,
	 int src_stride_y,
	 const unsigned char* src_u,
	 int src_stride_u,
	 const unsigned char* src_v,
	 int src_stride_v,
	 int src_width,
	 int src_height,
	 unsigned char* dst_y,
	 int dst_stride_y,
	 unsigned char* dst_u,
	 int dst_stride_u,
	 unsigned char* dst_v,
	 int dst_stride_v,
	 int dst_width,
	 int dst_height)
 {
	 return libyuv::I420Scale(src_y,
		 src_stride_y,
		 src_u,
		 src_stride_u,
		 src_v,
		 src_stride_v,
		 src_width,
		 src_height,
		 dst_y,
		 dst_stride_y,
		 dst_u,
		 dst_stride_u,
		 dst_v,
		 dst_stride_v,
		 dst_width,
		 dst_height,
		 libyuv::kFilterBox);
 }


 int ws_I420Rotate(const unsigned char* src_y,
	 int src_stride_y,
	 const unsigned char* src_u,
	 int src_stride_u,
	 const unsigned char* src_v,
	 int src_stride_v,
	 unsigned char* dst_y,
	 int dst_stride_y,
	 unsigned char* dst_u,
	 int dst_stride_u,
	 unsigned char* dst_v,
	 int dst_stride_v,
	 int dst_width,
	 int dst_height)
 {
	 return libyuv::I420Rotate(src_y,
		 src_stride_y,
		 src_u,
		 src_stride_u,
		 src_v,
		 src_stride_v,
		 dst_y,
		 dst_stride_y,
		 dst_u,
		 dst_stride_u,
		 dst_v,
		 dst_stride_v,
		 dst_width,
		 dst_height,libyuv::kRotate180);
 }

 int64_t TimeMillis()
 {
	 return rtc::TimeMillis();
 }