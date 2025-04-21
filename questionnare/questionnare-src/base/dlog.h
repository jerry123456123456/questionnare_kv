#ifndef LOG_H
#define LOG_H
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#ifndef SPDLOG_TRACE_ON
#define SPDLOG_TRACE_ON
#endif

#ifndef SPDLOG_DEBUG_ON
#define SPDLOG_DEBUG_ON
#endif


class DLog{
public:
    static DLog* GetInstance(){
        static DLog dlogger;
        return &dlogger;
    }
    std::shared_ptr<spdlog::logger> getLogger(){
        return log_;
    }
    static void SetLevel(char *log_level);

private:
    DLog(){
        //创建一个包含多个日志的sink列表
        std::vector<spdlog::sink_ptr> sinkList;
    #if 1   //输出日志到控制台
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();  //输出到控制台带颜色支持
        consoleSink->set_level(level_);
        consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%@,%!][%l] : %v");   //这个是日志的输出格式
        sinkList.push_back(consoleSink);
    #endif
        //输出日志到文件
        auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/daily.log", 23, 59);
        dailySink->set_level(level_);
        dailySink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%@,%!][%l] : %v");
        sinkList.push_back(dailySink);

        log_ = std::make_shared<spdlog::logger>("both", begin(sinkList), end(sinkList));   //创建一个新的 spdlog::logger 对象，名称为 "both"，并将 sinkList 中的所有 sink 传递给它。这将使得日志同时输出到控制台和文件中。
        //注册到全局管理器中
        spdlog::register_logger(log_);

        //每隔一秒刷新一次
        spdlog::flush_every(std::chrono::seconds(1));  //`std::chrono::seconds(1)` 表示一个持续时间为1秒的时间间隔
    }
    ~DLog(){}
private:
    std::shared_ptr<spdlog::logger> log_;
    static spdlog::level::level_enum level_;
};


//用定义的方式调用C++标准库spdlog里面的接口，记录不同级别的日志信息
#define LogTrace(...) SPDLOG_LOGGER_CALL(DLog::GetInstance()->getLogger().get(), spdlog::level::trace, __VA_ARGS__)
#define LogDebug(...) SPDLOG_LOGGER_CALL(DLog::GetInstance()->getLogger().get(), spdlog::level::debug, __VA_ARGS__)
#define LogInfo(...) SPDLOG_LOGGER_CALL(DLog::GetInstance()->getLogger().get(), spdlog::level::info, __VA_ARGS__)
#define LogWarn(...) SPDLOG_LOGGER_CALL(DLog::GetInstance()->getLogger().get(), spdlog::level::warn, __VA_ARGS__)
#define LogError(...) SPDLOG_LOGGER_CALL(DLog::GetInstance()->getLogger().get(), spdlog::level::err, __VA_ARGS__)
#define LogCritical(...) SPDLOG_LOGGER_CALL(DLog::GetInstance()->getLogger().get(), spdlog::level::critical, __VA_ARGS__)
#endif