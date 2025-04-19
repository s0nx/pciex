// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024-2025 Petr Vyazovik <xen@f-m.fm>

#include "config.h"
#include "log.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>

#include <chrono>
#include <format>

extern cfg::PCIexCfg pciex_cfg;

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
                        std::format("Failed to set logs dirctory ownership, err {}",
                                    errno));
        }
    }

    return logs_dir_path;
}

static std::string GenLogFname()
{
    using namespace std::chrono;

    auto time_now_sec = time_point_cast<seconds>(system_clock::now());
    auto zt_now = zoned_time{current_zone(), time_now_sec};
    return std::format("pciex_{:%Y_%m_%d_%T}.log", zt_now);
}

void Logger::init()
{
    if (pciex_cfg.common.logging_enabled) {
        auto logs_dir_path = CreateLogsDir();
        auto log_file_path = logs_dir_path / GenLogFname();
        log_file_ = std::fopen(log_file_path.c_str(), "w");
        if (!log_file_)
            throw std::runtime_error(
                    std::format("Failed to open log file {}, err {}",
                                log_file_path.c_str(), errno));

        logger_verbosity_ = Verbosity{pciex_cfg.common.default_log_level};
    }
}

Logger::~Logger()
{
    if (log_file_ != nullptr)
        fclose(log_file_);
}

