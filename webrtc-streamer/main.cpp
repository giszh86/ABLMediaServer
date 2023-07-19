/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
**
** -------------------------------------------------------------------------*/

#include <signal.h>

#include <iostream>
#include <fstream>

#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"
#include "p2p/base/stun_server.h"
#include "p2p/base/basic_packet_socket_factory.h"
#include "p2p/base/turn_server.h"

#include "system_wrappers/include/field_trial.h"

#include "PeerConnectionManager.h"
#include "HttpServerRequestHandler.h"
#include "framework.h"





PeerConnectionManager* webRtcServer = NULL;

void sighandler(int n)
{
	printf("SIGINT\n");
	// delete need thread still running
	delete webRtcServer;
	webRtcServer = NULL;
	rtc::Thread::Current()->Quit(); 
}

void  printfVersion()
{
	RTC_LOG(LS_ERROR) << "******************************************";
	RTC_LOG(LS_ERROR) << "*                                         *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*          rtsp- webrtc                  *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                " << VERSION << "              *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*           " << FileLog::getDatetime("yyyy-MM-dd hh:mm:ss.zzz") << "          *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "*                                        *";
	RTC_LOG(LS_ERROR) << "******************************************";

}

bool checkTimer()
{
	char Time[256] = "2023-8-1 14:00:00";
	unsigned int year, mon, day, hour, min, sec;
	sscanf(Time, "%d-%d-%d %d:%d:%d", &year, &mon, &day, &hour, &min, &sec);
	time_t timep = time(NULL);;//获取当前时间
	struct tm* p;

	p = localtime(&timep); //取得当地时间
	unsigned int dayin, daysys, tickin, ticksys;
	dayin = year * 10000 + mon * 100 + day;
	daysys = (p->tm_year + 1900) * 10000 + (p->tm_mon + 1) * 100 + p->tm_mday;
	if (dayin > daysys)
	{
		return true;
	}
	else if (dayin < daysys)
	{
		return false;
	}
	//日期相等
	tickin = hour * 3600 + min * 60 + sec;
	ticksys = p->tm_hour * 3600 + p->tm_min * 60 + p->tm_sec;
	if (tickin > ticksys)
	{

		return true;
	}
	else
	{
		return false;
	}

}

/* ---------------------------------------------------------------------------
**  main
** -------------------------------------------------------------------------*/




//#include "encoder/VideoEncoder.h"
int main(int argc, char* argv[])
{

#if 0

	if (checkTimer() == false)
	{
		std::cout << " time out ，please add QQ 125388771" << std::endl;
		system("pause");
		return 0;
	}
#else


#endif // !1


	const char* turnurl = "admin:admin@175.178.213.69:3478";
	const char* defaultlocalstunurl = "0.0.0.0:3479";
	const char* localstunurl = "0.0.0.0:3478";
	const char* defaultlocalturnurl = "admin:admin@175.178.213.69:3478";
	const char* localturnurl = NULL;
	const char* stunurl = "192.168.2.12:3479";
	std::string localWebrtcUdpPortRange = "0:65535";
	int logLevel              = rtc::LS_ERROR;
	const char* webroot       = "./html";
	std::string sslCertificate;
	webrtc::AudioDeviceModule::AudioLayer audioLayer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
	std::string nbthreads;
	std::string passwdFile;
	std::string authDomain = "mydomain.com";
	bool        disableXframeOptions = false;

	std::string publishFilter(".*");
	Json::Value config;  
	bool        useNullCodec = true;
	bool        usePlanB = false;
	int         maxpc = 0;
	std::string webrtcTrialsFields = "WebRTC-FrameDropper/Disabled/";


	std::string httpAddress("0.0.0.0:");
	std::string httpPort = "8000";
	const char * port = getenv("PORT");
	if (port)
	{
		httpPort = port;
	}
	httpAddress.append(httpPort);

	std::string streamName;

	std::cout  << "Version:" << VERSION << std::endl;

	std::cout  << config;

	rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)logLevel);
	rtc::LogMessage::LogTimestamps();
	rtc::LogMessage::LogThreads();
	rtc::LogMessage::AddLogToStream(new FileLog("logs"), (rtc::LoggingSeverity)logLevel);
	std::cout << "Logger level:" << rtc::LogMessage::GetLogToDebug() << std::endl;

	rtc::ThreadManager::Instance()->WrapCurrentThread();
	rtc::Thread* thread = rtc::Thread::Current();
	rtc::InitializeSSL();

	// webrtc server
	std::list<std::string> iceServerList;
	if ((strlen(stunurl) != 0) && (strcmp(stunurl,"-") != 0)) {
		iceServerList.push_back(std::string("stun:")+stunurl);
	}
	if (strlen(turnurl)) {
		iceServerList.push_back(std::string("turn:")+turnurl);
	}

	// init trials fields
	webrtc::field_trial::InitFieldTrialsFromString(webrtcTrialsFields.c_str());

	webRtcServer = new PeerConnectionManager(iceServerList, config["urls"], audioLayer, publishFilter, localWebrtcUdpPortRange, useNullCodec, usePlanB, maxpc);
	if (!webRtcServer->InitializePeerConnection())
	{
		std::cout << "Cannot Initialize WebRTC server" << std::endl;
	}
	else
	{
		// http server
		std::vector<std::string> options;
		options.push_back("document_root");
		options.push_back(webroot);
		options.push_back("enable_directory_listing");
		options.push_back("no");
		if (!disableXframeOptions) {
			options.push_back("additional_header");
			options.push_back("X-Frame-Options: SAMEORIGIN");
		}
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
		if (!sslCertificate.empty()) {
			options.push_back("ssl_certificate");
			options.push_back(sslCertificate);
		}
		if (!nbthreads.empty()) {
			options.push_back("num_threads");
			options.push_back(nbthreads);
		}
		if (!passwdFile.empty()) {
			options.push_back("global_auth_file");
			options.push_back(passwdFile);
			options.push_back("authentication_domain");
			options.push_back(authDomain);
		}
		
		try {
			std::map<std::string,HttpServerRequestHandler::httpFunction> func = webRtcServer->getHttpApi();
			std::cout << "HTTP Listen at " << httpAddress << std::endl;
			HttpServerRequestHandler httpServer(func, options);

	
			//mainloop
			signal(SIGINT, sighandler);
			printfVersion();
			thread->Run();

		} catch (const CivetException & ex) {
			std::cout << "Cannot Initialize start HTTP server exception:" << ex.what() << std::endl;
		}
	}

	rtc::CleanupSSL();


#if WIN32
	//释放套接字 
	::WSACleanup();
#endif
	std::cout << "Exit" << std::endl;
	return 0;
}

