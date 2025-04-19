// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024-2025 Petr Vyazovik <xen@f-m.fm>

#include "config.h"
#include "log.h"
#include "pciex_version.h"
#include "util.h"

#include <CLI/CLI.hpp>
#include <glaze/glaze.hpp>

extern Logger logger;

namespace cfg {

// Strip filename component from the given path and check if the resulting
// file is a directory
class PreceedPathValidator : public CLI::Validator {
  public:
    PreceedPathValidator()
        : CLI::Validator("PATH")
    {
        func_ = [](std::string &filename) {
            std::error_code ec;
            std::filesystem::path path {filename.c_str()};

            // strip filename component
            path.remove_filename();

            auto stat = std::filesystem::status(path, ec);
            if (ec)
                return "Directory doesn't exist: " + path.string();

            switch (stat.type()) {
            case std::filesystem::file_type::none:
            case std::filesystem::file_type::not_found:
                return "Directory doesn't exist: " + path.string();
            case std::filesystem::file_type::directory:
                return std::string{};
            default:
                return "Preceeding path is not a directory: " + path.string();
            }
        };
    }
};

void ParseCmdLineOptions(CmdLOpts &cmdl_opts, int argc, char *argv[])
{
    CLI::App app{"PCI topology explorer", "pciex"};
    app.get_formatter()->column_width(40);
    //app.get_formatter()->right_column_width(40);

    // dedicated group needed to make `-c` and `-s` mutually exclusive
    auto sgrp = app.add_option_group("+Snapshots");
    // hide help in group
    sgrp->set_help_flag();
    sgrp->require_option(1);
    sgrp->add_option_function<std::string>(
            "-c,--capture-snapshot",
            [&](const std::string &val) {
                cmdl_opts.snapshot_path_ = val;
                cmdl_opts.mode_ = OperationMode::SnapshotCapture;
            },
            "capture PCI topology snapshot")
        ->option_text("< path/to/snapshot >")
        //XXX: existing validators like CLI::ExistingFile are wrapped
        // around with CLI::Validator to redefine description to
        // avoid cryptic parsing errors descriptions:
        // default:
        // --capture-snapshot: 1 required TEXT:NOT FILE:PATH missing
        // after description has been redefined:
        // --capture-snapshot: 1 required TEXT missing
        ->check(!CLI::Validator(CLI::ExistingFile, {}))
        ->check(CLI::Validator(PreceedPathValidator(), {}));

    sgrp->add_option_function<std::string>(
            "-s,--view-snapshot ",
            [&](const std::string &val) {
                cmdl_opts.snapshot_path_ = val;
                cmdl_opts.mode_ = OperationMode::SnapshotView;
            },
            "examine previously captured PCI topology snapshot")
        ->option_text("< path/to/snapshot >")
        ->check(CLI::Validator(CLI::ExistingFile, {}));

    sgrp->add_flag("-l,--live", "examine PCI topology");

    app.add_flag("-v, --version",
            [](std::int64_t) {
                std::print("{} {}\n", pciex_current_version, pciex_current_hash);
                throw CLI::Success();
            }, "Print version and exit");

    app.required(false);


    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // app.exit(...) returns cli11-specific error
        auto _ = app.exit(e);
        std::exit(EXIT_FAILURE);
    }

}

static constexpr CTMap<OperationMode, bool, 3> OpModePrivMap {{
    {
        {OperationMode::Live,            true},
        {OperationMode::SnapshotCapture, true},
        {OperationMode::SnapshotView,    false}
    }
}};

bool OpModeNeedsElPriv(const OperationMode mode)
{
    return OpModePrivMap.at(mode);
}

void CmdLOpts::Dump()
{
    logger.log(Verbosity::INFO, "mode: {}, snapshot path: {}, requires elevated privileges: {}",
               OpModeName(mode_),
               snapshot_path_,
               OpModeNeedsElPriv(mode_));
}

static bool CommonConfigIsValid(const PCIexCommonCfg &common_cfg)
{
    // check log level
    const auto &level = common_cfg.default_log_level;
    if (level != std::clamp(level, e_to_type(Verbosity::FATAL),
                                   e_to_type(Verbosity::RAW))) {
        std::print("cfg.common: Default logging level should be in range [{} to {}]\n",
                   e_to_type(Verbosity::FATAL), e_to_type(Verbosity::RAW));
        return false;
    }

    // check if hwdata db file exist
    std::filesystem::directory_entry hwdata_db_dir_e {common_cfg.hwdata_db_path};
    if (!hwdata_db_dir_e.exists()) {
        std::print("cfg.common: hwdata db [{}] doesn't exist\n", common_cfg.hwdata_db_path);
        return false;
    }

    return true;
}

static bool TUIConfigIsValid([[maybe_unused]] const PCIexTUICfg &common_cfg)
{
    return true;
}

static bool ConfigIsValid(const PCIexCfg &cfg)
{
    if (!CommonConfigIsValid(cfg.common))
        return false;

    if (!TUIConfigIsValid(cfg.tui))
        return false;

    return true;
}

constexpr std::string_view cfg_file_path { "/etc/pciex/config.json" };

void ParseConfig(PCIexCfg &cfg)
{
    std::filesystem::directory_entry cfg_dir_e {cfg_file_path};
    if (!cfg_dir_e.exists()) {
        std::print("No user-defined config. Using default one.\n");
        return;
    }

    PCIexCfg ext_cfg{};
    std::string buffer{};
    auto ec = glz::read_file_json(ext_cfg, cfg_file_path, buffer);
    if (ec) {
        auto err_msg =
            std::format("Failed to parse JSON config file {} -> {}\n", cfg_file_path,
                        glz::format_error(ec, buffer));
        throw std::runtime_error(err_msg);
    }

    // Some of the fields in config file need explicit validation
    if (!ConfigIsValid(ext_cfg))
        throw std::runtime_error("Config validation has failed");

    cfg = ext_cfg;
}

} // namespace cfg

