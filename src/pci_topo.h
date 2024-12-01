// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <map>

#include "ids_parse.h"
#include "provider_iface.h"

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
    Create(uint64_t d_bdf, cfg_space_type cfg_len, pci_dev_type dev_type,
           ProviderArg &p_arg, std::unique_ptr<uint8_t []> cfg_buf)
    {
        if (dev_type == pci_dev_type::TYPE0)
            return std::make_shared<PciType0Dev>(d_bdf, cfg_len, dev_type, p_arg,
                                                 std::move(cfg_buf));
        else
            return std::make_shared<PciType1Dev>(d_bdf, cfg_len, dev_type, p_arg,
                                                 std::move(cfg_buf));
    }
};

struct PCITopologyCtx
{
    bool                                     live_mode_;
    PciObjCreator                            dev_creator_;
    PciIdParser                              iparser_;
    std::vector<std::shared_ptr<PciDevBase>> devs_;
    std::map<uint16_t, PCIBus>               buses_;

    PCITopologyCtx(bool live_mode) :
        live_mode_(live_mode),
        dev_creator_(),
        iparser_(),
        devs_(),
        buses_()
    {}

    void Populate(Provider &);
    void DumpData() const noexcept;
    void Capture(Provider &, Provider &);

    //XXX: DEBUG
    void PrintBus(const PCIBus &, int off);
};

} // namespace pci
