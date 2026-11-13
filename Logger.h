#pragma once

#include "noncopyable.h"
#include <string>

// 定义宏
#define LOG_INFO(LogmsgFormat, ...) \
        do{ \
            Logger& logger = Logger::instance(); \
            logger.setLogLevel(INFO); \
            char buf[1024] = {0}; \
            snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
            logger.log(buf); \
        }while(0)

#define LOG_ERROR(LogmsgFormat, ...) \
    do{ \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)

#define LOG_FATAL(LogmsgFormat, ...) \
    do{ \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)

#ifdef DEBUG_WORK
    #define LOG_DEBUG(LogmsgFormat, ...) \
        do{ \
            Logger& logger = Logger::instance(); \
            logger.setLogLevel(DEBUG); \
            char buf[1024] = {0}; \
            snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
            logger.log(buf); \
        }while(0)
#else
    #define LOG_DEBUG(LogmsgFormat, ...)
#endif

/*
日志的级别 : 
INFO  : 正常的日志输出
ERROR : 不影响软件继续执行的错误
FATAL : 影响软件继续执行的严重错误
DEBUG : 调试的日志输出，正常运行时不显示
*/

enum LogLevel{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

// 输出一个日志类
class Logger : noncopyable{
private:
    int m_LogLevel;
    Logger() = default;

public:
    // 获取日志唯一的实例对象
    static Logger& instance();

    // 设置日志级别
    void setLogLevel(int level);

    // 写日志
    void log(std::string msg);
};