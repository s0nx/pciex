// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 Petr Vyazovik <xen@f-m.fm>

#pragma once

// sysfs interface to gather PCI device information

#include <vector>
#include <cstdio>
#include "log.h"

extern Logger logger;

constexpr std::string_view pci_devs_path {"/sys/bus/pci/devices"};
constexpr std::string_view pci_bus_path {"/sys/class/pci_bus"};

namespace sysfs {

struct CfgEx : public CommonEx
{
    using CommonEx::CommonEx;
};

// dom+BDF, cfg space buf, cfg space len, path to device in sysfs
typedef std::tuple<uint64_t, std::unique_ptr<uint8_t []>, int32_t, fs::path> dev_desc;
std::vector<dev_desc> scan_pci_devices();

typedef std::pair<std::unique_ptr<uint8_t []>, int32_t> cfg_space_desc;
cfg_space_desc get_cfg_space_buf(const fs::path &);

typedef std::tuple<uint64_t, uint64_t, uint64_t> res_desc;
// Get PCI device resources from "resource" file
std::vector<res_desc> get_resources(const fs::path &);
void dump_resources(const std::vector<res_desc> &, const std::string &dev_id = {}) noexcept;

std::string get_driver(const fs::path &);
int32_t     get_numa_node(const fs::path &);
uint32_t    get_iommu_group(const fs::path &);

// dom, bus, is root bus
typedef std::tuple<uint16_t, uint8_t, bool> bus_desc;
std::vector<bus_desc> scan_buses();

} // namespace sysfs

