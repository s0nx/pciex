// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include <string>
#include <iostream>
#include <bit>
#include <unistd.h>

#include "pciex.h"
#include "ids_parse.h"
#include "log.h"

#include "ui/screen.h"

Verbosity LoggerVerbosity = Verbosity::INFO;
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
            logger.err("Exception occured while parsing /proc/vmallocinfo: {}", ex.what());
        }
    } else {
        logger.warn("vmalloced addresses are hidden\n");
    }

    if (vm_info.InfoAvailable())
        vm_info.DumpStats();

    pci::PCITopologyCtx topology;
    try {
        topology.populate();
    } catch (std::exception &ex) {
        logger.fatal("{}", ex.what());
        std::exit(EXIT_FAILURE);
    }

    topology.dump_data();

    auto [width, height] = ui::GetCanvasSizeEstimate(topology, ui::ElemReprMode::Compact);
    auto topo_canvas = ui::MakeTopologyComp(width, height, topology);

    auto pci_regs_component = std::make_shared<ui::PCIRegsComponent>(topo_canvas);
    int right_pane_size = 60;

    auto main_component_split = ftxui::ResizableSplit({
        .main = topo_canvas,
        .back = pci_regs_component,
        .direction = ftxui::Direction::Left,
        .main_size = &right_pane_size,
        .separator_func = [] { return ftxui::separatorDouble(); }
    });

    bool show_help = false;
    auto hide_modal = [&] { show_help = false; };

    main_component_split |= ftxui::CatchEvent([&](ftxui::Event ev) {
        if (ev.is_character()) {
            if (ev.character()[0] == '?')
                show_help = show_help ? false : true;
        }

        return false;
    });

    auto help_component = ui::GetHelpScreenComp(hide_modal);
    help_component |= ftxui::CatchEvent([&](ftxui::Event ev) {
        if (ev.is_character()) {
            if (ev.character()[0] == '?')
                show_help = show_help ? false : true;
        }

        return false;
    });

    main_component_split |= ftxui::Modal(help_component, &show_help);

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.Loop(main_component_split);

    return 0;
}
