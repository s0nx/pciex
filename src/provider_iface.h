// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

using   OpaqueBuf = std::unique_ptr<uint8_t []>;
using ProviderArg = std::variant<std::filesystem::path, OpaqueBuf>;

using DevResourceDesc = std::tuple<uint64_t, uint64_t, uint64_t>;
constexpr uint32_t dev_res_desc_size = 24;

// Intermidiate PCI device descriptor
struct DeviceDesc
{
    uint64_t                     dbdf_;          // domain + BDF
    uint16_t                     cfg_space_len_; // config space length
    OpaqueBuf                    cfg_space_;     // buffer holding a copy of config space
    std::vector<DevResourceDesc> resources_;
    std::string                  driver_name_;
    uint16_t                     numa_node_;
    uint16_t                     iommu_group_;
    ProviderArg                  arg_;
};

// dom, bus, is root bus
using BusDesc = std::tuple<uint16_t, uint16_t, uint16_t>;
constexpr uint32_t bus_desc_size = 6;

struct Provider
{
    virtual
    std::string GetProviderName() const = 0;
    virtual
    std::vector<BusDesc> GetBusDescriptors() = 0;
    virtual
    std::vector<DeviceDesc> GetPCIDevDescriptors() = 0;

    virtual
    bool ShouldParseV2PBarMappingInfo() = 0;

    virtual
    void SaveState(const std::vector<DeviceDesc> &devs,
                   const std::vector<BusDesc> &buses) = 0;

};
