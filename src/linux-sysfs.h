// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include "provider_iface.h"

// sysfs interface to gather PCI device information
namespace fs = std::filesystem;

namespace sysfs {

struct SysfsProvider : public Provider
{
    std::string GetProviderName() const override { return "SysFS"; }

    std::vector<BusDesc>         GetBusDescriptors() override;
    // <dom+BDF, cfg space len, cfg space buf, path to device in sysfs>
    std::vector<DeviceDesc>      GetPCIDevDescriptors() override;

    void SaveState(const std::vector<DeviceDesc> &devs,
                   const std::vector<BusDesc> &buses) override;
};

} // namespace sysfs

