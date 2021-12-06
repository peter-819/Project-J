#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger{
    public:
    using LoggerPtr = std::shared_ptr<spdlog::logger>;
    static LoggerPtr& Get();
    static LoggerPtr Create(); 
    static void InitGlobally();
};

#define JLOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define JLOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define JLOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define JLOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define JLOG_FATAL(...)    SPDLOG_FATAL(__VA_ARGS__)
