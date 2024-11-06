// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <map>
#include <vector>
#include <cstdint>
#include <filesystem>

#include "ids_parse.h"

namespace pci {

struct PCIBus
{
    uint16_t dom_;
    uint16_t bus_nr_;
    bool     is_root_ {false};
    std::vector<std::shared_ptr<PciDevBase>> devs_;

    PCIBus(uint16_t dom, uint16_t nr, bool is_root)
        : dom_(dom), bus_nr_(nr), is_root_(is_root) {}
};

class PciObjCreator
{
public:
    std::shared_ptr<PciDevBase>
    create(uint64_t d_bdf, cfg_space_type cfg_len, pci_dev_type dev_type,
           const std::filesystem::path &dev_path, std::unique_ptr<uint8_t []> cfg_buf)
    {
        if (dev_type == pci_dev_type::TYPE0)
            return std::make_shared<PciType0Dev>(d_bdf, cfg_len, dev_type, dev_path,
                                                 std::move(cfg_buf));
        else
            return std::make_shared<PciType1Dev>(d_bdf, cfg_len, dev_type, dev_path,
                                                 std::move(cfg_buf));
    }
};

struct PCITopologyCtx
{
    PciObjCreator dev_creator_;
    PciIdParser iparser;
    std::vector<std::shared_ptr<PciDevBase>> devs_;
    std::map<uint16_t, PCIBus> buses_;

    void populate();
    void dump_data() const noexcept;

    //XXX: DEBUG
    void print_bus(const PCIBus &, int off);
};

} // namespace pci
