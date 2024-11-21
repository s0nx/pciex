// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdint>
#include <string>

namespace cfg {

enum class OperationMode
{
    Live,
    SnapshotCapture,
    SnapshotView
};

constexpr auto OpModeName(const OperationMode mode) noexcept
{
    switch (mode) {
    case OperationMode::Live:
        return "Live";
    case OperationMode::SnapshotCapture:
        return "Capture snapshot";
    case OperationMode::SnapshotView:
        return "View snapshot";
    default:
        return "";
    }
}

bool OpModeNeedsElPriv(const OperationMode mode);

struct CmdLOpts
{
    OperationMode mode_ {OperationMode::Live};
    std::string   snapshot_path_;

    void Dump();
};

void ParseCmdLineOptions(CmdLOpts &cmdl_opts, int argc, char *argv[]);

} // namespace cfg

