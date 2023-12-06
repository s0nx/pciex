#include <string>
#include <iostream>
#include <bit>
#include <unistd.h>

#include "pciex.h"
#include "ids_parse.h"
#include "log.h"

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
    pci::PciIdParser iparser;

    int i = 0;
    for (const auto &dev : topology.devs_) {
        auto vid    = dev->get_vendor_id();
        auto dev_id = dev->get_device_id();
        auto vname = iparser.vendor_name_lookup(vid);
        auto dev_name = iparser.device_name_lookup(vid, dev_id);
        logger.raw("[ #{} ]", i++);
        logger.raw("[{:04}:{:02}:{:02x}.{}] -> cfg_size {:4} vendor {:2x} [{}] | dev {:2x} [{}]",
                   dev->dom_, dev->bus_, dev->dev_, dev->func_, e_to_type(dev->cfg_type_),
                   vid, vname, dev_id, dev_name);

        auto cc = dev->get_class_code();
        logger.raw("CC: |{:04x}|", cc);

        auto [class_name, subclass_name, prog_iface] = iparser.class_info_lookup(cc);
        logger.raw("---");
        logger.raw("- {}", class_name);
        logger.raw(" \\- {}", subclass_name);
        logger.raw("   \\- {}", prog_iface);
        logger.raw("---");
    }


}
