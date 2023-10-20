

#include <map>
#include "rtc_obj_sdk.h"


#include "PeerConnectionManager.h"

#include "rtc_base/checks.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/win32_socket_init.h"
#include "system_wrappers/include/field_trial.h"
#include "rtc_base/physical_socket_server.h"
#include "NSJson.h"

class RtcLogSink :public rtc::LogSink {
public:
	RtcLogSink() {}
	~RtcLogSink() {}
	virtual void OnLogMessage(const std::string& message)
	{
		std::cout<<"RtcLog OnLogMessage ="<< message;
		static FILE* file = fopen("./rtc.log", "ab+");
		if (file)
		{
			std::string tmp = message + "\r\n";
			fwrite(tmp.c_str(), 1, tmp.length(), file);
			fflush(file);
		}
	}
};


void WebRtcEndpoint::init(const char* webrtcConfig, std::function<void(const char* callbackJson, void* pUserHandle)> callback)
{

	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;
		m_callback = callback;
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
		// webrtc server
		std::list<std::string> iceServerList;
		std::string publishFilter(".*");
		Json::Value config;
		bool        useNullCodec = true;
		bool        usePlanB = false;
		int         maxpc = 0;
		std::string localWebrtcUdpPortRange = "0:65535";

		webrtc::AudioDeviceModule::AudioLayer audioLayer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
		webRtcServer = new PeerConnectionManager(iceServerList, config, audioLayer, publishFilter, localWebrtcUdpPortRange, useNullCodec, usePlanB, maxpc);
		if (!webRtcServer->InitializePeerConnection())
		{
			std::cout << "Cannot Initialize WebRTC server" << std::endl;
		}
		std::map<std::string, HttpServerRequestHandler::httpFunction> func = webRtcServer->getHttpApi();

		webRtcServer->setCallBcak(callback);
		// http server
		const char* webroot = "./html";

		std::string httpAddress("0.0.0.0:");
		std::string httpPort =ABL::NSJson::ParseStr(webrtcConfig).GetString("webrtcPort");

		if (httpPort.size()<1)
		{
			httpPort = "8000";
		}
		httpAddress.append(httpPort);


		std::vector<std::string> options;
		options.push_back("document_root");
		options.push_back(webroot);
		options.push_back("enable_directory_listing");
		options.push_back("no");

		options.push_back("access_control_allow_origin");
		options.push_back("*");
		options.push_back("listening_ports");
		options.push_back(httpAddress);
		options.push_back("enable_keep_alive");
		options.push_back("yes");
		options.push_back("keep_alive_timeout_ms");
		options.push_back("1000");
		options.push_back("decode_url");
		options.push_back("no");
		httpServer = new HttpServerRequestHandler(func, options);

	}
}

bool WebRtcEndpoint::stopWebRtcPlay(const char* peerid)
{
	if (webRtcServer)
	{
		webRtcServer->hangUp(peerid);
	}


	return false;
}

bool WebRtcEndpoint::deleteWebRtcSource(const char* szMediaSource)
{
	VideoCaptureManager::getInstance().RemoveInput(szMediaSource);
	return false;
}

WebRtcEndpoint& WebRtcEndpoint::getInstance()
{
	static WebRtcEndpoint instance;
	return instance;
}



 WebRtcEndpoint* CreateWebRtcEndpoint()
 {
	 return nullptr;
	// return HSingletonTemplatePtr<WebRtcEndpointImp>::Instance();
 }
