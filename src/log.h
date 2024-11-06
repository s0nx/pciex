// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <fmt/printf.h>

namespace fs = std::filesystem;

enum class Verbosity
{
    FATAL,
    ERR,
    WARN,
    INFO,
    RAW
};

constexpr auto VerbName(const Verbosity level)
{
    switch (level) {
        case Verbosity::FATAL:
            return "[FATAL]";
        case Verbosity::ERR:
            return "[  ERR]";
        case Verbosity::WARN:
            return "[ WARN]";
        case Verbosity::INFO:
            return "[ INFO]";
        case Verbosity::RAW:
            return "      |";
        default:
            return "";
    }
}

constexpr Verbosity LoggerVerbosity { Verbosity::RAW };
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
            throw std::runtime_error(fmt::format("Failed to open log file: {}", log_file_path.c_str()));
    }

    ~Logger()
    {
        if (log_file_ != nullptr)
            fclose(log_file_);
    }

    template <class... Args>
    void log(const Verbosity verb_lvl, fmt::format_string<Args...> s, Args&&... args)
    {
        if (LoggerVerbosity >= verb_lvl) {
            fmt::print(log_file_, "{:>7} {}\n", VerbName(verb_lvl),
                       fmt::format(s, std::forward<Args>(args)...));
            std::fflush(log_file_);
        }
    }

};
