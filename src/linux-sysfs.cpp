// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2025 Petr Vyazovik <xen@f-m.fm>

#include "linux-sysfs.h"
#include "log.h"

#include <fstream>
#include <format>

extern Logger logger;

namespace sysfs {

constexpr std::string_view pci_devs_path {"/sys/bus/pci/devices"};
constexpr std::string_view pci_bus_path {"/sys/class/pci_bus"};

// Scan buses in system and determine if any of them are root buses.
// Root buses are usually reported by firmware in some way, for example via ACPI tables.
// Example bus entries in /sys/class/pci_bus:
// 0000:00 -> ../../devices/pci0000:00/pci_bus/0000:00              <- root bus
// 0000:02 -> ../../devices/pci0000:00/0000:00:06.0/pci_bus/0000:02 <- 'regular' bus
std::vector<BusDesc>
SysfsProvider::GetBusDescriptors()
{
    std::vector<BusDesc> bus_vt;

    if (!fs::directory_entry(pci_bus_path).exists()) {
        logger.log(Verbosity::WARN, "{} doesn't exist", pci_bus_path);
        return {};
    }

    logger.log(Verbosity::INFO, "Scanning {}...", pci_bus_path);

    for (const auto &bus_dir_e : fs::directory_iterator {pci_bus_path}) {
        if (!fs::is_symlink(bus_dir_e)) {
            logger.log(Verbosity::WARN, "bus entry is not a symlink");
            return {};
        } else {
            auto bus_entry = fs::read_symlink(bus_dir_e);
            uint32_t dom, bus;
            auto res = sscanf(bus_entry.filename().c_str(), "%4u:%2x", &dom, &bus);
            if (res != 2) {
                logger.log(Verbosity::WARN, "Failed to parse bus symlink");
                return {};
            }

            bool is_root_bus = false;
            auto bs = bus_entry.string();
            size_t pos = std::string::npos;
            int    cnt = 0;
            while (cnt != 3) {
                pos = bs.rfind('/', pos);
                if (pos == std::string::npos) {
                    logger.log(Verbosity::WARN, "Failed to determine if the bus is root bus");
                    return {};
                }
                pos -= 1;
                cnt++;
            }

            if (bs[pos + 2] == 'p')
                is_root_bus = true;
            logger.log(Verbosity::INFO, "Got bus entry: [{:04}:{:02x}] is root: {}",
                        dom, bus, is_root_bus);
            bus_vt.emplace_back(dom, bus, is_root_bus ? 1 : 0);
        }
    }

    return bus_vt;
}

using cfg_space_desc = std::pair<std::unique_ptr<uint8_t []>, uint32_t>;

static cfg_space_desc GetCfgSpaceBuf(const fs::path &sysfs_dev_entry)
{
    auto config = fs::directory_entry(sysfs_dev_entry / "config");
    auto cfg_size = config.file_size();

    auto cfg_fd = std::fopen(config.path().c_str(), "r");
    if (!cfg_fd)
        throw std::runtime_error(std::format("Failed to open {}", config.path().string()));

    std::unique_ptr<uint8_t[]> ptr { new (std::nothrow) uint8_t [cfg_size] };
    if (!ptr)
    {
        std::fclose(cfg_fd);
        throw std::runtime_error(std::format("Failed to allocate cfg buffer for {}",
                                             config.path().string()));
    }

    const std::size_t read = std::fread(ptr.get(), cfg_size, 1, cfg_fd);
    if (read != 1)
    {
        std::fclose(cfg_fd);
        throw std::runtime_error(std::format("Failed to read cfg buffer for {}",
                                             config.path().string()));
    }

    std::fclose(cfg_fd);
    return std::make_pair(std::move(ptr), static_cast<int32_t>(cfg_size));
}

// It's not possible to determine the size of the resource requested by
// device after the address has been written into the BAR.
// It should either be kept during configuration or new configuration should
// be perfromed by writting all 1's to the register and reading back the value.
// Sysfs 'resource' file is used to get the size. It is also used to corrrectly interpret
// the BAR contents later.
static std::vector<DevResourceDesc>
GetPCIDevResources(const fs::path &sysfs_dev_entry)
{
    // Depending on device type and kernel configuration, namely CONFIG_PCI_IOV,
    // amount of lines in 'resource' file might differ.
    // If kernel has been configured with 'PCI IOV' support there would either
    // 13 (for Type 0 device) or 17 (for Type 1 device) entries.
    // With 'CONFIG_PCI_IOV' not set, there would either 7 (Type 0) or 11 (Type 1) entries.
    //
    //        ┌─        ┌─ [0 - 5]   - BARs resources
    //        │  type 0 ┤  [  6  ]   - expansion ROM resource
    //        │         └─ [7 - 12]  - IOV resources (CONFIG_PCI_IOV enabled)
    // type 1 ┤            [13] (7)  - IO behind bridge
    //        │            [14] (8)  - memory behind bridge
    //        │            [15] (9)  - prefetchable memory behind bridge
    //        └─           [16] (10) - < empty >

    auto resource = fs::directory_entry(sysfs_dev_entry / "resource");
    if (!resource.exists()) {
        logger.log(Verbosity::WARN, "'{}' doesn't exist", resource.path().c_str());
        return {};
    }

    std::ifstream res_file(resource.path().c_str(), std::ios::in);
    if (!res_file.is_open()) {
        logger.log(Verbosity::WARN, "Failed to open '{}'", resource.path().c_str());
        return {};
    }

    std::vector<DevResourceDesc> resources;

    std::string res_entry;
    while (std::getline(res_file, res_entry)) {
        uint64_t start, end, flags;
        auto res = std::sscanf(res_entry.c_str(), "%lx %lx %lx", &start, &end, &flags);
        if (res != 3) {
            logger.log(Verbosity::WARN, "Failed to parse resource for '{}'", resource.path().c_str());
            return {};
        }

        resources.emplace_back(start, end, flags);
    }
    return resources;
}

static std::string
GetDriver(const fs::path &sysfs_dev_entry)
{
    auto driver_link = fs::directory_entry(sysfs_dev_entry / "driver");
    if (!driver_link.exists()) {
        logger.log(Verbosity::INFO, "Driver is not loaded for {}", sysfs_dev_entry.c_str());
        return {};
    }

    if (!fs::is_symlink(driver_link.path())) {
        logger.log(Verbosity::WARN, "'driver' is not a symlink for {}", sysfs_dev_entry.c_str());
        return {};
    } else {
        auto drv_path = fs::read_symlink(driver_link.path());
        return drv_path.filename().string();
    }
}

static uint16_t
GetNumaNode(const fs::path &sysfs_dev_entry)
{
    auto numa_node = fs::directory_entry(sysfs_dev_entry / "numa_node");
    if (!numa_node.exists()) {
        logger.log(Verbosity::INFO, "Can't get NUMA info for {}", sysfs_dev_entry.c_str());
        return -1;
    }

    int32_t node_num;
    std::ifstream in(numa_node.path().c_str(), std::ios::in);
    if (!in.is_open()) {
        logger.log(Verbosity::INFO, "Can't read NUMA info for {}", sysfs_dev_entry.c_str());
        return -1;
    } else {
        in >> node_num;
    }

    if (node_num < 0)
        return std::numeric_limits<uint16_t>::max();

    return node_num;
}

static uint16_t
GetIommuGroup(const fs::path &sysfs_dev_entry)
{
    auto iommu_group = fs::directory_entry(sysfs_dev_entry / "iommu_group");
    if (!iommu_group.exists()) {
        logger.log(Verbosity::INFO, "iommu_group entry is missing for {}", sysfs_dev_entry.c_str());
        return {};
    }

    if (!fs::is_symlink(iommu_group.path())) {
        logger.log(Verbosity::INFO, "'iommu_group' is not a symlink for {}", sysfs_dev_entry.c_str());
        return {};
    } else {
        auto group = fs::read_symlink(iommu_group.path());
        return std::stoi(group.filename().string());
    }
}

std::vector<DeviceDesc>
SysfsProvider::GetPCIDevDescriptors()
{
    std::vector<DeviceDesc> devices;

    logger.log(Verbosity::INFO, "Scanning {}...", pci_devs_path);

    for (const auto &pci_dev_dir_e : fs::directory_iterator {pci_devs_path}) {
        uint32_t dom, bus, dev, func;
        auto res = std::sscanf(pci_dev_dir_e.path().filename().c_str(),
                               "%4u:%2x:%2x.%u", &dom, &bus, &dev, &func);
        if (res != 4) {
            throw std::runtime_error(std::format("Failed to parse BDF for {}\n",
                                     pci_dev_dir_e.path().string()));
        } else {
            logger.log(Verbosity::INFO, "Got -> [{:04}:{:02x}:{:02x}.{:x}]", dom, bus, dev, func);

            uint64_t d_bdf = func | (dev << 8) | (bus << 16) | (dom << 24);
            auto [data, cfg_len] = GetCfgSpaceBuf(pci_dev_dir_e.path());

            // try to acquire resources
            auto resources = GetPCIDevResources(pci_dev_dir_e.path());
            if (resources.empty())
                throw std::runtime_error(std::format("Failed to acquire resources for {}\n",
                                         pci_dev_dir_e.path().string()));

            auto driver_name = GetDriver(pci_dev_dir_e.path());
            auto numa_node = GetNumaNode(pci_dev_dir_e.path());
            auto iommu_group = GetIommuGroup(pci_dev_dir_e.path());

            devices.emplace_back(d_bdf, cfg_len, std::move(data), std::move(resources),
                                 driver_name, numa_node, iommu_group, pci_dev_dir_e.path());
        }
    }
    return devices;
}

void
SysfsProvider::SaveState([[maybe_unused]]const std::vector<DeviceDesc> &devs,
                         [[maybe_unused]]const std::vector<BusDesc> &buses)
{
    throw std::runtime_error(std::format("{} provider doesn't support state saving",
                             GetProviderName()));
}

} // namespace sysfs
