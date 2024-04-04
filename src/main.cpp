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

    if (!sys::is_kptr_set())
        logger.warn("vmalloced addresses are hidden\n");

    vm::VmallocStats vm_map_stats;
    vm_map_stats.parse();
    vm_map_stats.dump_stats();

    auto pa_start = 0x000000603d1d9000;
    auto pa_end = pa_start + 0x1000;
    vm_map_stats.get_mapping_in_range(pa_start, pa_end);

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
    int right_size = 60;

    auto container_split = ftxui::ResizableSplit({
        .main = topo_canvas,
        .back = pci_regs_component,
        .direction = ftxui::Direction::Left,
        .main_size = &right_size,
        .separator_func = [] { return ftxui::separatorDouble(); }
    });

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.Loop(container_split);

    return 0;
}
