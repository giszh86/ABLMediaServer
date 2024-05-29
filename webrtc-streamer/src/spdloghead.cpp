#include "spdloghead.h"
#include <iostream>
#include <filesystem>
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> g_logger_ptr;

void SPDLOGInit()
{
	auto max_size = 1024 * 1024 * 5;
	auto max_files = 3;
	g_logger_ptr = spdlog::rotating_logger_mt("libFFmpeg", "logs/libFFmpeg.txt", max_size, max_files);
	spdlog::set_default_logger(g_logger_ptr);
	//spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%s:%#][%!][%l] : %v");
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s %!:%#] %v");
	spdlog::info("This an info message with custom format");

	SPDLOG_LOGGER_ERROR(g_logger_ptr, "test3 {}", 3);//会输出文件名和行号

}

#if defined(_WIN32)

int mkdir(const char* path, int mode) {
	return _mkdir(path);
}
#endif

int	replace_all(std::string& strBuf, std::string  strSrc, std::string  strDes)
{
	if (strSrc.empty())
	{
		return 0;  // 无需替换，返回替换次数为0
	}

	std::size_t pos = 0;
	int nCount = 0;

	while ((pos = strBuf.find(strSrc, pos)) != std::string::npos)
	{
		strBuf.replace(pos, strSrc.length(), strDes);
		pos += strDes.length();
		++nCount;
	}

	return nCount;

}

bool create_path(const char* file, unsigned int mod) {

	std::string path = file;
	replace_all(path, "\\", "/");
	std::string dir;
	size_t index = 1;
	while (true) {
		index = path.find('/', index) + 1;
		dir = path.substr(0, index);
		if (dir.length() == 0) {
			break;
		}
		if (_access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
			if (mkdir(dir.c_str(), mod) == -1) {  //如果不存在就用mkdir函数来创建

				return false;
			}
		}
	}
	return true;
}

void spdlog::SPDLOG::init(std::string log_file_path, std::string logger_name, std::string level, size_t max_file_size, size_t max_files, bool mt_security)
{
	try 
	{
		// Convert log file path to absolute path and create the directory
		std::filesystem::path fs_log_file_path = std::filesystem::absolute(log_file_path).parent_path();
		create_path(fs_log_file_path.string().c_str(),777);
		if (mt_security)		
		{

			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_path, max_file_size, max_files);
			file_sink->set_level(spdlog::level::trace);
			file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");

			/* 控制台sink */
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::trace);
			console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");

			/* Sink组合 */
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(console_sink);
			sinks.push_back(file_sink);
			m_logger_ptr = std::make_shared<spdlog::logger>("multi-sink", begin(sinks), end(sinks));
			setLogLevel(level);
			//m_logger_ptr->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s %!:%#] %v"); //设置格式:https://spdlog.docsforge.com/v1.x/3.custom-formatting/
			//m_logger_ptr->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s:%#] %v"); //设置格式:https://spdlog.docsforge.com/v1.x/3.custom-formatting/




		//	m_logger_ptr = spdlog::rotating_logger_mt(logger_name, log_file_path, max_file_size, max_files);
		}
		else {

			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(log_file_path, max_file_size, max_files);
			file_sink->set_level(spdlog::level::trace);
			file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");

			/* 控制台sink */
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
			console_sink->set_level(spdlog::level::trace);
			console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");

			/* Sink组合 */
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(console_sink);
			sinks.push_back(file_sink);
			m_logger_ptr = std::make_shared<spdlog::logger>("st-sink", begin(sinks), end(sinks));
			setLogLevel(level);

		//	m_logger_ptr = spdlog::rotating_logger_st(logger_name, log_file_path, max_file_size, max_files);
		}
	

		
	}
	catch (const spdlog::spdlog_ex& ex) {
	
		std::cout << "Log initialization failed: " << std::string(ex.what()) << std::endl;

	}
}

void spdlog::SPDLOG::setLogLevel(const std::string& level)
{
	char L = toupper(level[0]);
	if (L == 'T') { // trace
		m_logger_ptr->set_level(spdlog::level::trace);
		m_logger_ptr->flush_on(spdlog::level::trace);
	}
	else if (L == 'D') { // debug
		m_logger_ptr->set_level(spdlog::level::debug);
		m_logger_ptr->flush_on(spdlog::level::debug);
	}
	else if (L == 'I') { // info
		m_logger_ptr->set_level(spdlog::level::info);
		m_logger_ptr->flush_on(spdlog::level::info);
	}
	else if (L == 'W') { // warn
		m_logger_ptr->set_level(spdlog::level::warn);
		m_logger_ptr->flush_on(spdlog::level::warn);
	}
	else if (L == 'E') { // error
		m_logger_ptr->set_level(spdlog::level::err);
		m_logger_ptr->flush_on(spdlog::level::err);
	}
	else if (L == 'C') { // critical
		m_logger_ptr->set_level(spdlog::level::critical);
		m_logger_ptr->flush_on(spdlog::level::critical);
	}
	else {

		std::cout << "level set error " << level << std::endl;

	}


}

void spdlog::SPDLOG::setLogLevel(level::level_enum log_level)
{
	m_logger_ptr->set_level(log_level);
	m_logger_ptr->flush_on(log_level);
}

spdlog::SPDLOG& spdlog::SPDLOG::getInstance()
{
	static SPDLOG instance;
	return instance;
}