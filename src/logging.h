#pragma once

#include <clap-rpc/global.hpp>

#include <format>
#include <iostream>
#include <source_location>
#include <thread>

CLAP_RPC_BEGIN_NAMESPACE

enum LogLevel { INFO = 0, DEBUG, WARNING, ERROR };

template <typename... Args>
struct Log
{
    static inline LogLevel sCurrentLevel = INFO;
    static inline bool SShowThread = true;
    static inline bool sShowSourceLocation = false;
    explicit Log(LogLevel level, std::format_string<Args...> fmt, Args &&...args,
        const std::source_location &loc = std::source_location::current())
    {
        if (level < sCurrentLevel)
            return;

        std::clog << "clap::rpc ";

        const char *lStr = nullptr;
        switch (level) {
        case INFO:
            lStr = "[INFO] ";
            break;
        case DEBUG:
            lStr = "[DEBUG] ";
            break;
        case WARNING:
            lStr = "[WARNING] ";
            break;
        case ERROR:
            lStr = "[ERROR] ";
            break;
        }
        std::clog << lStr;

        if (SShowThread)
            std::clog << "[Thread " << std::this_thread::get_id() << "] ";

        if (sShowSourceLocation) {
            const auto fn = std::string_view(loc.file_name());
            const auto shortFile = fn.substr(fn.find_last_of('/') + 1);
            std::clog << shortFile << " ('" << loc.function_name() << "'): ";
        }

        std::clog << std::format(std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...)
                  << '\n';
    }
};
template <typename... Args>
Log(LogLevel, std::format_string<Args...>, Args &&...) -> Log<Args...>;

CLAP_RPC_END_NAMESPACE
