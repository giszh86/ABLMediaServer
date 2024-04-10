#include "spdloghead.h"

#include <iostream>




void spdlog::SPDLOG::init(std::string log_file_path, std::string logger_name, std::string level, size_t max_file_size, size_t max_files, bool mt_security)
{
	try {
		if (mt_security) {
			m_logger_ptr = spdlog::rotating_logger_mt(logger_name, log_file_path, max_file_size, max_files);
		}
		else {
			m_logger_ptr = spdlog::rotating_logger_st(logger_name, log_file_path, max_file_size, max_files);
		}
		setLogLevel(level);
		//m_logger_ptr->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s %!:%#] %v"); //设置格式:https://spdlog.docsforge.com/v1.x/3.custom-formatting/
		m_logger_ptr->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s:%#] %v"); //设置格式:https://spdlog.docsforge.com/v1.x/3.custom-formatting/

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