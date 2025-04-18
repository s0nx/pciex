// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024-2025 Petr Vyazovik <xen@f-m.fm>

#include "config.h"
#include "log.h"
#include "pciex_version.h"
#include "util.h"

#include <CLI/CLI.hpp>

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

} // namespace cfg

