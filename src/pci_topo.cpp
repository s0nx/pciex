// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#include "pci_dev.h"
#include "pci_topo.h"
#include "pci_regs.h"
#include "log.h"
#include "util.h"

#include <fmt/format.h>

extern Logger logger;

namespace pci {

void PCITopologyCtx::Populate(const Provider &provider)
{
    auto devices = provider.GetPCIDevDescriptors();

    for (auto &dev_desc : devices) {
        const auto h_type = reinterpret_cast<uint8_t *>
                     (dev_desc.cfg_space_.get() + e_to_type(Type0Cfg::header_type));
        const auto dev_type = *h_type & 0x1 ? pci_dev_type::TYPE1 : pci_dev_type::TYPE0;
        auto pci_dev = dev_creator_.Create(dev_desc.dbdf_,
                                           cfg_space_type{dev_desc.cfg_space_len_},
                                           dev_type,
                                           dev_desc.arg_,
                                           std::move(dev_desc.cfg_space_));
        pci_dev->ParseCapabilities();
        pci_dev->DumpCapabilities();
        pci_dev->AssignResources(provider.GetPCIDevResources(dev_desc.arg_));
        pci_dev->DumpResources();
        pci_dev->ParseBars();
        pci_dev->ParseBarsV2PMappings();
        pci_dev->ParseIDs(iparser_);
        auto drv_name = provider.GetDriver(dev_desc.arg_);
        logger.log(Verbosity::INFO, "{} driver: {}", pci_dev->dev_id_str_,
                    drv_name.empty() ? "<none>" : drv_name);
        devs_.push_back(std::move(pci_dev));
    }

    std::ranges::sort(devs_, [](const auto &a, const auto &b) {
        uint64_t d_bdf_a = a->func_ | (a->dev_ << 8) | (a->bus_ << 16) | (a->dom_ << 24);
        uint64_t d_bdf_b = b->func_ | (b->dev_ << 8) | (b->bus_ << 16) | (b->dom_ << 24);
        return d_bdf_a < d_bdf_b;
    });

    auto bus_descs = provider.GetBusDescriptors();

    for (auto &bus : bus_descs) {
        PCIBus pci_bus(std::get<0>(bus), std::get<1>(bus), std::get<2>(bus));
        std::ranges::copy_if(devs_, std::back_inserter(pci_bus.devs_), [&](const auto &dev) {
                return dev->bus_ == std::get<1>(bus);
        });
        auto res = buses_.insert(std::make_pair(std::get<1>(bus), std::move(pci_bus)));
        if (!res.second)
            throw std::runtime_error(fmt::format("Failed to initialize bus {:02x}",
                                     std::get<1>(bus)));
    }

}

void PCITopologyCtx::DumpData() const noexcept
{
    for (const auto &el : devs_)
        el->print_data();
}

void PCITopologyCtx::PrintBus(const PCIBus &bus, int off)
{
    for (const auto &dev : bus.devs_) {
        if (dev->type_ == pci::pci_dev_type::TYPE1) {
            auto fmt_str = fmt::format("{:\t>{}} \\--> {}", "", off, dev->dev_id_str_);
            logger.log(Verbosity::RAW, "{}", fmt_str);

            auto type1_dev = dynamic_cast<PciType1Dev *>(dev.get());
            auto sec_bus = buses_.find(type1_dev->get_sec_bus_num());
            if (sec_bus != buses_.end()) {
                PrintBus(sec_bus->second, off + 1);
            }

        } else {
            auto fmt_str = fmt::format("{:\t>{}} \\--> {}", "", off, dev->dev_id_str_);
            logger.log(Verbosity::RAW, "{}", fmt_str);
        }
    }

}

} // namespace pci
