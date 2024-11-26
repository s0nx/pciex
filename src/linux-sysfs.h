// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include "provider_iface.h"

// sysfs interface to gather PCI device information
namespace fs = std::filesystem;

namespace sysfs {

struct SysfsProvider : public Provider
{
    std::vector<BusDesc>         GetBusDescriptors() const override;
    // <dom+BDF, cfg space len, cfg space buf, path to device in sysfs>
    std::vector<DeviceDesc>      GetPCIDevDescriptors() const override;

    // Get PCI device resources from "resource" file
    std::vector<DevResourceDesc>
    GetPCIDevResources(const ProviderArg &arg) const override;
    std::string_view
    GetDriver(const ProviderArg &arg) const override;
    int32_t
    GetNumaNode(const ProviderArg &arg) const override;
    uint32_t
    GetIommuGroup(const ProviderArg &arg) const override;
};

} // namespace sysfs

