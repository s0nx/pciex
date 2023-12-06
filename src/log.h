#pragma once

#include <filesystem>
#include <fstream>

#include "util.h"
#include <fmt/printf.h>

namespace fs = std::filesystem;

enum class Verbosity
{
    FATAL,
    ERR,
    WARN,
    INFO
};

constexpr auto VerbName(const Verbosity level)
{
    switch (level) {
        case Verbosity::FATAL:
            return "FATAL";
        case Verbosity::ERR:
            return "ERR";
        case Verbosity::WARN:
            return "WARN";
        case Verbosity::INFO:
            return "INFO";
        default:
            return "";
    }
}

struct LoggerEx : public CommonEx
{
    using CommonEx::CommonEx;
};

extern Verbosity LoggerVerbosity;
constexpr char log_dir[] { "/var/log/pciex" };

struct Logger
{
    std::FILE *log_file_;

    Logger() : log_file_(nullptr) {}

    void init()
    {
        fs::directory_entry log_dir_entry {log_dir};
        fs::path log_dir_path(log_dir);
        if (!log_dir_entry.exists())
            fs::create_directory(log_dir_path);

        auto log_file_path = log_dir_path / "pciex.log";
        log_file_ = std::fopen(log_file_path.c_str(), "w");
        if (!log_file_)
            throw LoggerEx(fmt::format("Failed to open log file: {}", log_file_path.c_str()));
    }

    ~Logger()
    {
        if (log_file_ != nullptr)
            fclose(log_file_);
    }

    template <class... Args>
    void fatal(fmt::format_string<Args...> s, Args&&... args)
    {
        if (LoggerVerbosity >= Verbosity::FATAL)
            fmt::print(log_file_, "[{:>5}] {}\n", VerbName(Verbosity::FATAL),
                       fmt::format(s, std::forward<Args>(args)...));
    }

    template <class... Args>
    void err(fmt::format_string<Args...> s, Args&&... args)
    {
        if (LoggerVerbosity >= Verbosity::ERR)
            fmt::print(log_file_, "[{:>5}] {}\n", VerbName(Verbosity::ERR),
                       fmt::format(s, std::forward<Args>(args)...));
    }

    template <class... Args>
    void warn(fmt::format_string<Args...> s, Args&&... args)
    {
        if (LoggerVerbosity >= Verbosity::WARN)
            fmt::print(log_file_, "[{:>5}] {}\n", VerbName(Verbosity::WARN),
                       fmt::format(s, std::forward<Args>(args)...));
    }

    template <class... Args>
    void info(fmt::format_string<Args...> s, Args&&... args)
    {
        if (LoggerVerbosity >= Verbosity::INFO)
            fmt::print(log_file_, "[{:>5}] {}\n", VerbName(Verbosity::INFO),
                       fmt::format(s, std::forward<Args>(args)...));
    }

    template <class... Args>
    void raw(fmt::format_string<Args...> s, Args&&... args)
    {
        if (LoggerVerbosity >= Verbosity::INFO)
            fmt::print(log_file_, "      | {}\n",
                       fmt::format(s, std::forward<Args>(args)...));
    }

};
