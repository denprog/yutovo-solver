#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

namespace yutovo_service
{

class Logger
{
private:
    Logger(const std::string& path, const std::string& name, bool in_console, bool in_file);

public:
    Logger(Logger const&) = delete;
    void operator=(Logger const&) = delete;

    static Logger* GetInstance(const std::string& path, const std::string& name, bool in_console, bool in_file);

    void Info(const char* message);
    void Debug(const char* message);
    void Warning(const char* message);
    void Error(const char* message);

    template<typename... Args>
    void Info(fmt::format_string<Args...> format, Args... args)
    {
        log->info(format, std::forward<Args>(args)...);
        log->flush();
    }

    template<typename... Args>
    void Debug(fmt::format_string<Args...> format, Args... args)
    {
        log->debug(format, std::forward<Args>(args)...);
        log->flush();
    }

    template<typename... Args>
    void Warning(fmt::format_string<Args...> format, Args... args)
    {
        log->warn(format, std::forward<Args>(args)...);
        log->flush();
    }

    template<typename... Args>
    void Error(fmt::format_string<Args...> format, Args... args)
    {
        log->error(format, std::forward<Args>(args)...);
        log->flush();
    }

private:
    std::shared_ptr<spdlog::logger> log;
};

class LogFormatter : public spdlog::formatter
{
public:
    LogFormatter();

    virtual void format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest);
    virtual std::unique_ptr<spdlog::formatter> clone() const;

private:
    std::tm get_time(const spdlog::details::log_msg &msg);

private:
    std::vector<std::unique_ptr<spdlog::details::flag_formatter>> formatters;
};

}

#endif
