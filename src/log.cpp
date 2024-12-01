// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#include "log.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>

#include <fmt/chrono.h>

namespace fs = std::filesystem;

// Create logs directory if it doesn't exist
// XXX: logs are stored inidividually because the tool can be run with or without
// elevated privileges
static fs::path CreateLogsDir()
{
    uint32_t uid, gid;

    // check if sudo was used to launch pciex, obtain uid/gid
    const char* sudo_uid_env = std::getenv("SUDO_UID");
    const char* sudo_gid_env = std::getenv("SUDO_GID");

    if (sudo_uid_env != nullptr) {
        uid = std::atoi(sudo_uid_env);
        gid = std::atoi(sudo_gid_env);
        if (uid == 0 || gid == 0)
            throw std::runtime_error("Failed to convert uid/gid str to int");
    } else {
        uid = getuid();
        gid = getgid();
    }

    fs::directory_entry logs_dir_entry {logs_dir};
    fs::path logs_dir_path(logs_dir);
    if (!logs_dir_entry.exists()) {
        fs::create_directories(logs_dir_path);

        if (sudo_uid_env != nullptr) {
            if (chown(logs_dir, uid, gid))
                throw std::runtime_error(
                        fmt::format("Failed to set logs dirctory ownership, err {}",
                                    errno));
        }
    }

    return logs_dir_path;
}

static std::string GenLogFname()
{
    auto tm = std::time(nullptr);
    return fmt::format("pciex_{:%Y_%m_%d_%T}.log", fmt::localtime(tm));
}

void Logger::init()
{
    auto logs_dir_path = CreateLogsDir();
    auto log_file_path = logs_dir_path / GenLogFname();
    log_file_ = std::fopen(log_file_path.c_str(), "w");
    if (!log_file_)
        throw std::runtime_error(
                fmt::format("Failed to open log file {}, err {}",
                            log_file_path.c_str(), errno));
}

Logger::~Logger()
{
    if (log_file_ != nullptr)
        fclose(log_file_);
}

