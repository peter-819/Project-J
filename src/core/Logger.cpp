#include <Jpch.h>
#include "Logger.h"

Logger::LoggerPtr Logger::Create(){
	spdlog::set_pattern("%^[%T] %@ %n: %v%$");
	return spdlog::stdout_color_mt("JProject");
}

Logger::LoggerPtr& Logger::Get(){
	static auto mLogger = Create();
	return mLogger;
}

void Logger::InitGlobally(){
    spdlog::set_pattern("%^[%T] %@ %n: %v%$");
}