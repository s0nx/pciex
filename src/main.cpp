// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 Petr Vyazovik <xen@f-m.fm>

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

    //pci::PciIdParser iparser;
    //int i = 0;
    //for (const auto &dev : topology.devs_) {
    //    auto vid    = dev->get_vendor_id();
    //    auto dev_id = dev->get_device_id();
    //    auto vname = iparser.vendor_name_lookup(vid);
    //    auto dev_name = iparser.device_name_lookup(vid, dev_id);
    //    logger.raw("[ #{} ]", i++);
    //    logger.raw("[{:04}:{:02}:{:02x}.{}] -> cfg_size {:4} vendor {:2x} [{}] | dev {:2x} [{}]",
    //               dev->dom_, dev->bus_, dev->dev_, dev->func_, e_to_type(dev->cfg_type_),
    //               vid, vname, dev_id, dev_name);

    //    auto cc = dev->get_class_code();
    //    logger.raw("CC: |{:04x}|", cc);

    //    auto [class_name, subclass_name, prog_iface] = iparser.class_info_lookup(cc);
    //    logger.raw("---");
    //    logger.raw("- {}", class_name);
    //    logger.raw(" \\- {}", subclass_name);
    //    logger.raw("   \\- {}", prog_iface);
    //    logger.raw("---");
    //}

    //logger.info("Dumping buses info: >>>");
    //for (const auto &bus : topology.buses_) {
    //    if (bus.second.is_root_) {
    //        logger.raw("[ R {:04}:{:02x} ]", bus.second.dom_, bus.second.bus_nr_);

    //        int curr_poff = 1;
    //        topology.print_bus(bus.second, curr_poff);
    //    }
    //}

    auto [width, height] = ui::GetCanvasSizeEstimate(topology, ui::ElemReprMode::Compact);
    auto topo_canvas = ui::MakeTopologyComp(width, height, topology);

    auto make_box_lambda = [&](size_t dimx, size_t dimy, std::string title, std::string text) {
        auto elem = ftxui::window(ftxui::text(title) | ftxui::hcenter | ftxui::bold,
                                  ftxui::text(text) | ftxui::hcenter ) |
                                  ftxui::size(ftxui::WIDTH, ftxui::EQUAL, dimx) |
                                  ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, dimy);
        return elem;
    };

    auto box_renderer = ftxui::Renderer([&] {
        return ftxui::vbox({
            make_box_lambda(5, 5,  "reg1", "0xffbf"),
            make_box_lambda(10, 5, "reg2", "0xdd"),
            make_box_lambda(10, 3, "reg3", "0x11ef"),
            make_box_lambda(7, 8,  "reg4", "0x87ab")
        });
    });

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
