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

    auto [width, height] = ui::GetCanvasSizeEstimate(topology, ui::ElemReprMode::Compact);
    auto topo_canvas = ui::MakeTopologyComp(width, height, topology);

    auto topo_canvas_bordered = ui::MakeBorderedHoverComp(topo_canvas);

    auto pci_regs_component = std::make_shared<ui::PCIRegsComponent>(topo_canvas);
    int vert_split_off = 60;

    auto main_component_split = ftxui::ResizableSplit({
        .main = topo_canvas_bordered,
        .back = pci_regs_component,
        .direction = ftxui::Direction::Left,
        .main_size = &vert_split_off,
        .separator_func = [] { return ftxui::separatorHeavy(); }
    });

    bool show_help = false;

    main_component_split |= ftxui::CatchEvent([&](ftxui::Event ev) {
        if (ev.is_character()) {
            if (ev.character()[0] == '?')
                show_help = show_help ? false : true;
        }

        // handle pane selection
        if (ev == ftxui::Event::F1)
            topo_canvas_bordered->TakeFocus();
        else if (ev == ftxui::Event::F2 || ev == ftxui::Event::F3)
            pci_regs_component->TakeFocus();

        // handle pane resize
        if (ev == ftxui::Event::AltH)
            ui::SeparatorShift(ui::UiElemShiftDir::LEFT, &vert_split_off);
        if (ev == ftxui::Event::AltL)
            ui::SeparatorShift(ui::UiElemShiftDir::RIGHT, &vert_split_off);
        if (ev == ftxui::Event::AltJ || ev == ftxui::Event::AltK)
            pci_regs_component->TakeFocus();

        return false;
    });

    auto help_component = ui::GetHelpScreenComp();
    help_component |= ftxui::CatchEvent([&](ftxui::Event ev) {
        if (ev == ftxui::Event::Escape ||
            (ev.is_character() &&
             (ev.character()[0] == '?' || ev.character()[0] == 'q')))
        {
                show_help = show_help ? false : true;
        }

        return false;
    });

    main_component_split |= ftxui::Modal(help_component, &show_help);

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.Loop(main_component_split);

    return 0;
}
