// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 Petr Vyazovik <xen@f-m.fm>

#include "linux-sysfs.h"

namespace sysfs {

std::vector<dev_desc> scan_pci_devices()
{
    std::vector<dev_desc> devices;

    logger.info("Scanning {}...", pci_devs_path);

    for (const auto &pci_dev_dir_e : fs::directory_iterator {pci_devs_path}) {
        uint32_t dom, bus, dev, func;
        auto res = std::sscanf(pci_dev_dir_e.path().filename().c_str(),
                               "%4u:%2x:%2x.%u", &dom, &bus, &dev, &func);
        if (res != 4) {
            throw sysfs::CfgEx(fmt::format("Failed to parse BDF for {}\n",
                                           pci_dev_dir_e.path().string()));
        } else {
            logger.info("Got -> [{:04}:{:02x}:{:02x}.{:x}]", dom, bus, dev, func);

            uint64_t d_bdf = func | (dev << 8) | (bus << 16) | (dom << 24);
            auto [data, cfg_len] = sysfs::get_cfg_space_buf(pci_dev_dir_e.path());

            devices.emplace_back(d_bdf, std::move(data), cfg_len, pci_dev_dir_e.path());
        }
    }
    return devices;
}

cfg_space_desc get_cfg_space_buf(const fs::path &sysfs_dev_entry)
{
    auto config = fs::directory_entry(sysfs_dev_entry / "config");
    auto cfg_size = config.file_size();

    auto cfg_fd = std::fopen(config.path().c_str(), "r");
    if (!cfg_fd)
        throw CfgEx(fmt::format("Failed to open {}", config.path().string()));

    std::unique_ptr<uint8_t[]> ptr { new (std::nothrow) uint8_t [cfg_size] };
    if (!ptr)
    {
        std::fclose(cfg_fd);
        throw CfgEx(fmt::format("Failed to allocate cfg buffer for {}", config.path().string()));
    }

    const std::size_t read = std::fread(ptr.get(), cfg_size, 1, cfg_fd);
    if (read != 1)
    {
        std::fclose(cfg_fd);
        throw CfgEx(fmt::format("Failed to read cfg buffer for {}", config.path().string()));
    }

    std::fclose(cfg_fd);
    return std::make_pair(std::move(ptr), static_cast<int32_t>(cfg_size));
}

std::vector<res_desc> get_resources(const fs::path &sysfs_dev_entry)
{
    auto resource = fs::directory_entry(sysfs_dev_entry / "resource");
    if (!resource.exists()) {
        logger.warn("'{}' doesn't exist", resource.path().c_str());
        return {};
    }

    std::ifstream res_file(resource.path().c_str(), std::ios::in);
    if (!res_file.is_open()) {
        logger.warn("Failed to open '{}'", resource.path().c_str());
        return {};
    }

    std::vector<res_desc> resources;

    std::string res_entry;
    while (std::getline(res_file, res_entry)) {
        uint64_t start, end, flags;
        auto res = std::sscanf(res_entry.c_str(), "%lx %lx %lx", &start, &end, &flags);
        if (res != 3) {
            logger.warn("Failed to parse resource for '{}'", resource.path().c_str());
            return {};
        }

        resources.emplace_back(start, end, flags);
    }
    return resources;
}

void dump_resources(const std::vector<res_desc> &res, const std::string &dev_id) noexcept
{
    logger.info("{} -> dump resources ({}): >>>", dev_id, res.size());
    for (int i = 0; const auto &res_entry : res) {
        logger.raw("[{:2}] {:#016x} {:#016x} {:#016x}", i++, std::get<0>(res_entry),
                   std::get<1>(res_entry), std::get<2>(res_entry));
    }
}

std::string get_driver(const fs::path &sysfs_dev_entry)
{
    auto driver_link = fs::directory_entry(sysfs_dev_entry / "driver");
    if (!driver_link.exists()) {
        logger.info("Driver is not loaded for {}", sysfs_dev_entry.c_str());
        return {};
    }

    if (!fs::is_symlink(driver_link.path())) {
        logger.warn("'driver' is not a symlink for {}", sysfs_dev_entry.c_str());
        return {};
    } else {
        auto drv_path = fs::read_symlink(driver_link.path());
        return drv_path.filename().string();
    }
}

int32_t get_numa_node(const fs::path &sysfs_dev_entry)
{
    auto numa_node = fs::directory_entry(sysfs_dev_entry / "numa_node");
    if (!numa_node.exists()) {
        logger.info("Can't get NUMA info for {}", sysfs_dev_entry.c_str());
        return -1;
    }

    int32_t node_num;
    std::ifstream in(numa_node.path().c_str(), std::ios::in);
    if (!in.is_open()) {
        logger.info("Can't read NUMA info for {}", sysfs_dev_entry.c_str());
        return -1;
    } else {
        in >> node_num;
    }

    return node_num;
}

uint32_t get_iommu_group(const fs::path &sysfs_dev_entry)
{
    auto iommu_group = fs::directory_entry(sysfs_dev_entry / "iommu_group");
    if (!iommu_group.exists()) {
        logger.info("iommu_group entry is missing for {}", sysfs_dev_entry.c_str());
        return {};
    }

    if (!fs::is_symlink(iommu_group.path())) {
        logger.info("'iommu_group' is not a symlink for {}", sysfs_dev_entry.c_str());
        return {};
    } else {
        auto group = fs::read_symlink(iommu_group.path());
        return std::stoi(group.filename().string());
    }
}

// Scan buses in system and determine if any of them are root buses.
// Root buses are usually reported by firmware in some way, for example via ACPI tables.
// Example bus entries in /sys/class/pci_bus:
// 0000:00 -> ../../devices/pci0000:00/pci_bus/0000:00              <- root bus
// 0000:02 -> ../../devices/pci0000:00/0000:00:06.0/pci_bus/0000:02 <- 'regular' bus
std::vector<bus_desc> scan_buses()
{
    std::vector<bus_desc> bus_vt;

    if (!fs::directory_entry(pci_bus_path).exists()) {
        logger.warn("{} doesn't exist", pci_bus_path);
        return {};
    }

    logger.info("Scanning {}...", pci_bus_path);

    for (const auto &bus_dir_e : fs::directory_iterator {pci_bus_path}) {
        if (!fs::is_symlink(bus_dir_e)) {
            logger.warn("bus entry is not a symlink");
            return {};
        } else {
            auto bus_entry = fs::read_symlink(bus_dir_e);
            uint32_t dom, bus;
            auto res = sscanf(bus_entry.filename().c_str(), "%4u:%2x", &dom, &bus);
            if (res != 2) {
                logger.warn("Failed to parse bus symlink");
                return {};
            }

            bool is_root_bus = false;
            auto bs = bus_entry.string();
            size_t pos = std::string::npos;
            int    cnt = 0;
            while (cnt != 3) {
                pos = bs.rfind('/', pos);
                if (pos == std::string::npos) {
                    logger.warn("Failed to determine if the bus is root bus");
                    return {};
                }
                pos -= 1;
                cnt++;
            }

            if (bs[pos + 2] == 'p')
                is_root_bus = true;
            logger.info("Got bus entry: [{:04}:{:02x}] is root: {}",
                        dom, bus, is_root_bus);
            bus_vt.emplace_back(dom, bus, is_root_bus);
        }
    }

    return bus_vt;
}

} // namespace sysfs
