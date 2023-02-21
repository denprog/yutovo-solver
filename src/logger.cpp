#include "logger.h"
#include <memory>
#include <iostream>
#include <experimental/filesystem>
#include <spdlog/sinks/stdout_sinks.h>

namespace yutovo_service
{

//Logger

Logger::Logger(const std::string& path, const std::string& name, bool in_console, bool in_file)
{
    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        if (in_console)
        {
            auto s = std::make_shared<spdlog::sinks::stdout_sink_st>();
            s->set_formatter(std::unique_ptr<spdlog::formatter>(new LogFormatter()));
            sinks.push_back(s);
        }

        std::string p;
        if (in_file)
        {
#ifdef _WIN32
            char szPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath)))
                p = std::string(szPath);
#endif
            p += path + name + ".log";
            sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(p, 0, 0, false, 10));
        }
        log = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));

        if (in_file)
            log->flush();
    }
    catch (spdlog::spdlog_ex& ex)
    {
        std::cout << ex.what();
    }

    spdlog::set_pattern("[%H:%M:%S.%e][%t][%n][%l] %v");
    spdlog::set_level(spdlog::level::debug);
}

Logger* Logger::GetInstance(const std::string& path, const std::string& name, bool in_console, bool in_file)
{
    static Logger log(path + "/", name, in_console, in_file);
    return &log;
}

void Logger::Info(const char* message)
{
    if (!log)
        return;
    log->info(message);
    log->flush();
}

void Logger::Debug(const char* message)
{
    if (!log)
        return;
    log->debug(message);
    log->flush();
}

void Logger::Warning(const char* message)
{
    if (!log)
        return;
    log->warn(message);
    log->flush();
}

void Logger::Error(const char* message)
{
    if (!log)
        return;
    log->error(message);
    log->flush();
}

//LogFormatter

LogFormatter::LogFormatter()
{
    std::string p = "log ";
    std::unique_ptr<spdlog::details::aggregate_formatter> p_format = spdlog::details::make_unique<spdlog::details::aggregate_formatter>();
    for (auto ch : p)
        p_format->add_ch(ch);
    formatters.push_back(std::move(p_format));

    formatters.push_back(spdlog::details::make_unique<spdlog::details::level_formatter<spdlog::details::null_scoped_padder>>(spdlog::details::padding_info{}));

    p = " ";
    p_format = spdlog::details::make_unique<spdlog::details::aggregate_formatter>();
    for (auto ch : p)
        p_format->add_ch(ch);
    formatters.push_back(std::move(p_format));

    formatters.push_back(spdlog::details::make_unique<spdlog::details::v_formatter<spdlog::details::null_scoped_padder>>(spdlog::details::padding_info{}));
}

void LogFormatter::format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest)
{
    std::tm t = get_time(msg);

    for (auto &f : formatters)
        f->format(msg, t, dest);

    if (dest.size() > 0 && dest[dest.size() - 1] != '\r' && dest[dest.size() - 1] != '\n')
        spdlog::details::fmt_helper::append_string_view(spdlog::details::os::default_eol, dest);
}

std::unique_ptr<spdlog::formatter> LogFormatter::clone() const
{
    return std::unique_ptr<spdlog::formatter>(new LogFormatter());
}

std::tm LogFormatter::get_time(const spdlog::details::log_msg &msg)
{
    return spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
}

}
