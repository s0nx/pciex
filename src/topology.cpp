// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 Petr Vyazovik <xen@f-m.fm>

#include "pciex.h"
#include "linux-sysfs.h"

namespace pci {

void PCITopologyCtx::populate()
{
    auto devices = sysfs::scan_pci_devices();

    for (auto &dev : devices) {
        auto d_bdf    = std::get<0>(dev);
        auto cfg_buf  = std::move(std::get<1>(dev));
        auto cfg_len  = std::get<2>(dev);
        auto dev_path = std::get<3>(dev);

        auto h_type = reinterpret_cast<uint8_t *>
                     (cfg_buf.get() + e_to_type(Type0Cfg::header_type));
        auto dev_type = *h_type & 0x1 ? pci_dev_type::TYPE1 : pci_dev_type::TYPE0;
        auto pci_dev = dev_creator_.create(d_bdf, cfg_space_type{cfg_len},
                                           dev_type, dev_path, std::move(cfg_buf));
        pci_dev->parse_capabilities();
        pci_dev->dump_capabilities();
        pci_dev->parse_bars();
        auto drv_name = sysfs::get_driver(dev_path);
        logger.info("{} driver: {}", pci_dev->dev_id_str_,
                    drv_name.empty() ? "<none>" : drv_name);
        devs_.push_back(std::move(pci_dev));
    }

    std::ranges::sort(devs_, [](const auto &a, const auto &b) {
        uint64_t d_bdf_a = a->func_ | (a->dev_ << 8) | (a->bus_ << 16) | (a->dom_ << 24);
        uint64_t d_bdf_b = b->func_ | (b->dev_ << 8) | (b->bus_ << 16) | (b->dom_ << 24);
        return d_bdf_a < d_bdf_b;
    });

    auto res = sysfs::scan_buses();
}

void PCITopologyCtx::dump_data() const noexcept
{
    for (const auto &el : devs_)
        el->print_data();
}

} // namespace pci
