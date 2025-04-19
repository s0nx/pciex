// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2025 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdio>
#include <print>

enum class Verbosity : uint8_t
{
    FATAL = 0x1,
    ERR   = 0x2,
    WARN  = 0x3,
    INFO  = 0x4,
    RAW   = 0x5
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

constexpr char logs_dir[] { "/tmp/pciex/logs" };

struct Logger
{
    std::FILE *log_file_;
    Verbosity logger_verbosity_;

    Logger() : log_file_(nullptr) {}
    ~Logger();

    void init();

    template <class... Args>
    void log(const Verbosity verb_lvl, std::format_string<Args...> s, Args&&... args)
    {
        if (log_file_ != nullptr && logger_verbosity_ >= verb_lvl) {
            std::print(log_file_, "{:>7} {}\n", VerbName(verb_lvl),
                       std::format(s, std::forward<Args>(args)...));
            std::fflush(log_file_);
        }
    }
};
