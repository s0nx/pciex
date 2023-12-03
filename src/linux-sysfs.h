#pragma once

// sysfs interface to gather PCI device information

#include <vector>
#include <cstdio>
#include "log.h"

extern Logger logger;

constexpr char pci_dev_path[] { "/sys/bus/pci/devices" };

namespace sysfs {

struct CfgEx : public CommonEx
{
    using CommonEx::CommonEx;
};

// dom+BDF, cfg space buf, cfg space len, path to device in sysfs
typedef std::tuple<uint64_t, std::unique_ptr<uint8_t []>, int32_t, fs::path> dev_desc;
std::vector<dev_desc> scan_pci_bus();

typedef std::pair<std::unique_ptr<uint8_t []>, int32_t> cfg_space_desc;
cfg_space_desc get_cfg_space_buf(const fs::path &);

typedef std::tuple<uint64_t, uint64_t, uint64_t> res_desc;
// Get PCI device resources from "resource" file
std::vector<res_desc> get_resources(const fs::path &);
void dump_resources(const std::vector<res_desc> &, const std::string &dev_id = {}) noexcept;

std::string get_driver(const fs::path &);
int32_t     get_numa_node(const fs::path &);
uint32_t    get_iommu_group(const fs::path &);

} // namespace sysfs

