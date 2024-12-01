// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include <unistd.h>

#include "config.h"
#include "log.h"
#include "linux-sysfs.h"
#include "snapshot.h"
#include "util.h"
#include "ui/screen.h"

#include <ftxui/component/screen_interactive.hpp>

cfg::CmdLOpts    cmdline_options;
vm::VmallocStats vm_info;
Logger           logger;

using Providers = std::pair<std::unique_ptr<Provider>, std::unique_ptr<Provider>>;
static Providers GetProvidersForOpMode(const cfg::CmdLOpts &opts);

int main(int argc, char *argv[])
{
    try {
        cfg::ParseCmdLineOptions(cmdline_options, argc, argv);

        logger.init();

        cmdline_options.Dump();

        if (cfg::OpModeNeedsElPriv(cmdline_options.mode_)) {
            if (getuid()) {
                fmt::print("'pciex' must be run with root privileges in [{}] mode. Exiting.\n",
                           cfg::OpModeName(cmdline_options.mode_));
                throw std::runtime_error("Insufficient execution privileges");
            }
        }

        if (std::endian::native != std::endian::little) {
            fmt::print("Non little-endian platforms are not supported by now. Exiting.\n");
            throw std::runtime_error("Unsupported endianness");
        }

        if (sys::IsKptrSet())
            vm_info.Parse();
        else
            logger.log(Verbosity::WARN, "vmalloced addresses are hidden\n");

        if (vm_info.InfoAvailable())
            vm_info.DumpStats();

        pci::PCITopologyCtx topology(cmdline_options.mode_ == cfg::OperationMode::Live);
        auto [capture_provider, store_provider] = GetProvidersForOpMode(cmdline_options);

        if (cmdline_options.mode_ == cfg::OperationMode::SnapshotCapture) {
            topology.Capture(*capture_provider, *store_provider);
        } else {
            topology.Populate(*capture_provider);
            topology.DumpData();

            std::unique_ptr<ui::ScreenCompCtx> screen_comp_ctx;
            ftxui::Component                   main_comp;

            try {
                screen_comp_ctx.reset(new ui::ScreenCompCtx(topology));
                main_comp = screen_comp_ctx->Create();
            } catch (std::exception &ex) {
                logger.log(Verbosity::FATAL, "Failed to initialize screen components: {}", ex.what());
                throw;
            }

            auto screen = ftxui::ScreenInteractive::Fullscreen();
            screen.Loop(main_comp);
        }
    } catch (std::exception &ex) {
        fmt::print("[{}] mode failure -> {}\nCheck log for details\n",
                   cfg::OpModeName(cmdline_options.mode_), ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static Providers GetProvidersForOpMode(const cfg::CmdLOpts &opts)
{
    try {
        std::unique_ptr<Provider> capture_provider;
        std::unique_ptr<Provider> store_provider;

        switch (opts.mode_) {
        case cfg::OperationMode::Live:
            capture_provider.reset(new sysfs::SysfsProvider());
            break;
        case cfg::OperationMode::SnapshotView:
            capture_provider.reset(new snapshot::SnapshotProvider(opts.snapshot_path_));
            break;
        case cfg::OperationMode::SnapshotCapture:
            capture_provider.reset(new sysfs::SysfsProvider());
            store_provider.reset(new snapshot::SnapshotProvider(opts.snapshot_path_));
        }

        return {std::move(capture_provider), std::move(store_provider)};
    } catch (std::exception &ex) {
        logger.log(Verbosity::FATAL, "Failed to initialize providers: {}", ex.what());
        throw;
    }
}
