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

// Intermidiate PCI device descriptor
struct DeviceDesc
{
    uint64_t    dbdf_;          // domain + BDF
    uint16_t    cfg_space_len_; // config space length
    OpaqueBuf   cfg_space_;     // buffer holding a copy of config space
    ProviderArg arg_;
};

using DevResourceDesc = std::tuple<uint64_t, uint64_t, uint64_t>;
// dom, bus, is root bus
using BusDesc = std::tuple<uint16_t, uint8_t, uint8_t>;

struct Provider
{
    virtual
    std::vector<BusDesc> GetBusDescriptors() const = 0;
    virtual
    std::vector<DeviceDesc> GetPCIDevDescriptors() const = 0;

    virtual
    std::vector<DevResourceDesc> GetPCIDevResources(const ProviderArg &arg) const = 0;
    virtual
    std::string_view GetDriver(const ProviderArg &arg) const = 0;
    virtual
    int32_t GetNumaNode(const ProviderArg &arg) const = 0;
    virtual
    uint32_t GetIommuGroup(const ProviderArg &arg) const = 0;
};
