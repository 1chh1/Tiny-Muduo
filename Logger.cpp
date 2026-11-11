#include "Logger.h"
#include "Timestamp.h"
#include <iostream>

// 获取日志唯一的实例对象
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    m_LogLevel = level;
}

// 写日志 [级别信息] [当前时间] [msg]
void Logger::log(std::string msg)
{
    switch(m_LogLevel)
    {
        case INFO:
           std::cout  << "[INFO] ";
           break;
        case ERROR:
           std::cout  << "[ERROR] ";
           break;
        case FATAL:
           std::cout  << "[FATAL] ";
           break;
        case DEBUG:
           std::cout  << "[DEBUG] ";
           break;
        default:
            break;
    }

    // 打印时间和msg
    std::cout << "TIME : " << Timestamp::now().toString() << ' ' << "Massage : " << msg << std::endl;
}