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

// Common config
struct PCIexCommonCfg
{
    bool      logging_enabled {false};
    // default logging verbosity level
    uint8_t default_log_level {0x1};

    // PCI ids database default location
    std::string hwdata_db_path {"/usr/share/hwdata/pci.ids"};
};

// TUI config
struct PCIexTUICfg
{
    // Device elements on the left device tree pane should be displayed in verbose
    // mode by default (or compact otherwise).
    bool dt_dflt_draw_verbose {false};

    // Highlighted device registers would be preserved on device switch.
    // When switching back to this device, registers highlighting state would be restored.
    bool keep_dev_selected_regs {false};
};

struct PCIexCfg
{
    PCIexCommonCfg common;
    PCIexTUICfg       tui;
};

// XXX: when adding new parameters to PCIexCfg don't forget to
// check if explicit validation is needed
void ParseConfig(PCIexCfg &cfg);

} // namespace cfg

