// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2025 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdio>
#include <print>

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
constexpr char logs_dir[] { "/tmp/pciex/logs" };

struct Logger
{
    std::FILE *log_file_;

    Logger() : log_file_(nullptr) {}
    ~Logger();

    void init();

    template <class... Args>
    void log(const Verbosity verb_lvl, std::format_string<Args...> s, Args&&... args)
    {
        if (log_file_ != nullptr && LoggerVerbosity >= verb_lvl) {
            std::print(log_file_, "{:>7} {}\n", VerbName(verb_lvl),
                       std::format(s, std::forward<Args>(args)...));
            std::fflush(log_file_);
        }
    }
};
