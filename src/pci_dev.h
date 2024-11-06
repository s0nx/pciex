// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <array>

#include "ids_parse.h"

namespace pci {

enum class cfg_space_type
{
    LEGACY = 256,
    ECS    = 4096
};

enum class pci_dev_type
{
    TYPE0,
    TYPE1
};

// capability types
enum class CapType
{
    compat,
    extended
};

// extended capabilities offset within configuration space
constexpr uint32_t ext_cap_cfg_off = 0x100;

enum ids_types
{
    VENDOR,
    DEVICE,
    CLASS,
    SUBCLASS,
    PROG_IFACE,

    SUBSYS_NAME,
    SUBSYS_VENDOR,

    IDS_TYPES_CNT
};

// Flags in the last value of the line in resource file.
// TODO: should really be in sysfs module
constexpr uint32_t       PCIResIO = 0x100;
constexpr uint32_t      PCIResMEM = 0x200;
constexpr uint32_t PCIResPrefetch = 0x2000;
constexpr uint32_t    PCIResMem64 = 0x100000;

// <cap type: compat or ext, capability ID, version, offset within config space>
typedef std::tuple<CapType, uint16_t, uint8_t, uint16_t> CapDesc;

enum class ResourceType
{
    MEMORY,
    IO,
    EMPTY
};

struct PciDevBarResource
{
    ResourceType type_;
    uint64_t     phys_addr_;
    uint64_t     len_;
    bool         is_64bit_;
    bool         is_prefetchable_;
};

struct PciDevBase
{
    uint16_t        dom_;
    uint8_t         bus_;
    uint8_t         dev_;
    uint8_t         func_;

    std::vector<std::string_view> ids_names_;

    std::string     dev_id_str_;

    bool            is_pcie_;
    cfg_space_type  cfg_type_;
    pci_dev_type    type_;

    std::unique_ptr<uint8_t[]> cfg_space_;

    // sysfs device path
    const std::filesystem::path sys_path_;

    // Array of capabity descriptors
    // <cap type: compat or ext, capability ID, version, offset within config space>
    std::vector<CapDesc> caps_;
    uint8_t compat_caps_num_;
    uint8_t extended_caps_num_;

    // device resources info obtained via sysfs
    std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> resources_;

    // BARs resources
    std::array<PciDevBarResource, 6> bar_res_;

    PciDevBase() = delete;
    PciDevBase(uint64_t d_bdf, cfg_space_type cfg_len, pci_dev_type dev_type,
               const std::filesystem::path &dev_path, std::unique_ptr<uint8_t []> cfg_buf);

    template <typename E>
    constexpr uint32_t get_reg_compat(E e, auto &map) const noexcept
    {
        auto dword_num = e_to_type(e) / 4;
        auto dword_off = e_to_type(e) % 4;
        auto reg_len = map.at(e);

        auto dword = *reinterpret_cast<uint32_t *>(cfg_space_.get() + dword_num * 4);
        if (reg_len == 4)
            return dword;

        dword >>= dword_off * 8;
        return dword & ((1 << reg_len * 8) - 1);
    }

    void parse_capabilities();
    void dump_capabilities() noexcept;
    void GetResources() noexcept;
    void ParseBars() noexcept;
    virtual void ParseIDs(PciIdParser &parser);

    // Common registers for both Type 0 / Type 1 devices
    uint32_t get_vendor_id() const noexcept;
    uint32_t get_device_id() const noexcept;
    uint32_t get_command() const noexcept;
    uint32_t get_status() const noexcept;
    uint32_t get_rev_id() const noexcept;
    uint32_t get_class_code() const noexcept;
    uint32_t get_cache_line_size() const noexcept;
    uint32_t get_lat_timer() const noexcept;
    uint32_t get_header_type() const noexcept;
    uint32_t get_bist() const noexcept;
    uint32_t get_cap_ptr() const noexcept;
    uint32_t get_itr_line() const noexcept;
    uint32_t get_itr_pin() const noexcept;
    virtual uint32_t get_exp_rom_bar() const noexcept = 0;

    virtual void print_data() const noexcept = 0;
};

struct PciType0Dev : public PciDevBase
{
    using PciDevBase::PciDevBase;

    uint32_t get_bar0() const noexcept;
    uint32_t get_bar1() const noexcept;
    uint32_t get_bar2() const noexcept;
    uint32_t get_bar3() const noexcept;
    uint32_t get_bar4() const noexcept;
    uint32_t get_bar5() const noexcept;
    uint32_t get_cardbus_cis() const noexcept;
    uint32_t get_subsys_vid() const noexcept;
    uint32_t get_subsys_dev_id() const noexcept;
    uint32_t get_exp_rom_bar() const noexcept override;
    uint32_t get_min_gnt() const noexcept;
    uint32_t get_max_lat() const noexcept;

    void ParseIDs(PciIdParser &parser) override;
    void print_data() const noexcept;
};

struct PciType1Dev : public PciDevBase
{
    using PciDevBase::PciDevBase;

    uint32_t get_bar0() const noexcept;
    uint32_t get_bar1() const noexcept;
    uint32_t get_prim_bus_num() const noexcept;
    uint32_t get_sec_bus_num() const noexcept;
    uint32_t get_sub_bus_num() const noexcept;
    uint32_t get_sec_lat_timer() const noexcept;
    uint32_t get_io_base() const noexcept;
    uint32_t get_io_limit() const noexcept;
    uint32_t get_sec_status() const noexcept;
    uint32_t get_mem_base() const noexcept;
    uint32_t get_mem_limit() const noexcept;
    uint32_t get_pref_mem_base() const noexcept;
    uint32_t get_pref_mem_limit() const noexcept;
    uint32_t get_pref_base_upper() const noexcept;
    uint32_t get_pref_limit_upper() const noexcept;
    uint32_t get_io_base_upper() const noexcept;
    uint32_t get_io_limit_upper() const noexcept;
    uint32_t get_exp_rom_bar() const noexcept override;
    uint32_t get_bridge_ctl() const noexcept;

    void print_data() const noexcept;
};

} // namespace pci
