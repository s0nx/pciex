// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include <unistd.h>

#include "log.h"
#include "util.h"
#include "ui/screen.h"

#include <ftxui/component/screen_interactive.hpp>

Logger logger;
vm::VmallocStats vm_info;

int main()
{
    if (getuid()) {
        fmt::print("'pciex' must be run with root privileges. Exiting.\n");
        std::exit(EXIT_FAILURE);
    }

    if (std::endian::native != std::endian::little) {
        fmt::print("Non little-endian platforms are not supported by now. Exiting\n");
        std::exit(EXIT_FAILURE);
    }

    try {
        logger.init();
    } catch (std::exception &ex) {
        fmt::print("{}", ex.what());
        std::exit(EXIT_FAILURE);
    }

    if (sys::IsKptrSet()) {
        try {
            vm_info.Parse();
        } catch (std::exception &ex) {
            logger.log(Verbosity::ERR, "Exception occured while parsing /proc/vmallocinfo: {}", ex.what());
        }
    } else {
        logger.log(Verbosity::WARN, "vmalloced addresses are hidden\n");
    }

    if (vm_info.InfoAvailable())
        vm_info.DumpStats();

    pci::PCITopologyCtx topology;
    try {
        topology.populate();
    } catch (std::exception &ex) {
        logger.log(Verbosity::FATAL, "{}", ex.what());
        std::exit(EXIT_FAILURE);
    }

    topology.dump_data();

    std::unique_ptr<ui::ScreenCompCtx> screen_comp_ctx;
    ftxui::Component                   main_comp;

    try {
        screen_comp_ctx.reset(new ui::ScreenCompCtx(topology));
        main_comp = screen_comp_ctx->Create();
    } catch (std::exception &ex) {
        logger.log(Verbosity::FATAL, "{}", ex.what());
        std::exit(EXIT_FAILURE);
    }

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.Loop(main_comp);

    return 0;
}
