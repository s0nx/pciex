// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 Petr Vyazovik <xen@f-m.fm>

#include "pciex.h"
#include "linux-sysfs.h"

namespace pci {

static constexpr RegMap<Type0Cfg, uint32_t, type0_compat_reg_cnt> t0_reg_map {{
    {{Type0Cfg::vid,             2},
     {Type0Cfg::dev_id,          2},
     {Type0Cfg::command,         2},
     {Type0Cfg::status,          2},

     {Type0Cfg::revision,        1},
     {Type0Cfg::class_code,      3},

     {Type0Cfg::cache_line_size, 1},
     {Type0Cfg::latency_timer,   1},
     {Type0Cfg::header_type,     1},
     {Type0Cfg::bist,            1},

     {Type0Cfg::bar0,            4},
     {Type0Cfg::bar1,            4},
     {Type0Cfg::bar2,            4},
     {Type0Cfg::bar3,            4},
     {Type0Cfg::bar4,            4},
     {Type0Cfg::bar5,            4},

     {Type0Cfg::cardbus_cis_ptr, 4},
     {Type0Cfg::subsys_vid,      2},
     {Type0Cfg::subsys_dev_id,   2},
     {Type0Cfg::exp_rom_bar,     4},

     {Type0Cfg::cap_ptr,         1},

     {Type0Cfg::itr_line,        1},
     {Type0Cfg::itr_pin,         1},

     {Type0Cfg::min_gnt,         1},
     {Type0Cfg::max_lat,         1}}
}};

static constexpr RegMap<Type1Cfg, uint32_t, type1_compat_reg_cnt> t1_reg_map {{
    {{Type1Cfg::vid,              2},
     {Type1Cfg::dev_id,           2},
     {Type1Cfg::command,          2},
     {Type1Cfg::status,           2},

     {Type1Cfg::revision,         1},
     {Type1Cfg::class_code,       3},

     {Type1Cfg::cache_line_size,  1},
     {Type1Cfg::prim_lat_timer,   1},
     {Type1Cfg::header_type,      1},
     {Type1Cfg::bist,             1},

     {Type1Cfg::bar0,             4},
     {Type1Cfg::bar1,             4},

     {Type1Cfg::prim_bus_num,     1},
     {Type1Cfg::sec_bus_num,      1},
     {Type1Cfg::sub_bus_num,      1},
     {Type1Cfg::sec_lat_timer,    1},

     {Type1Cfg::io_base,          1},
     {Type1Cfg::io_limit,         1},
     {Type1Cfg::sec_status,       2},

     {Type1Cfg::mem_base,         2},
     {Type1Cfg::mem_limit,        2},

     {Type1Cfg::pref_mem_base,    2},
     {Type1Cfg::pref_mem_limit,   2},

     {Type1Cfg::pref_base_upper,  4},
     {Type1Cfg::pref_limit_upper, 4},

     {Type1Cfg::io_base_upper,    2},
     {Type1Cfg::io_limit_upper,   2},

     {Type1Cfg::cap_ptr,          1},

     {Type1Cfg::exp_rom_bar,      4},

     {Type1Cfg::itr_line,         1},
     {Type1Cfg::itr_pin,          1},
     {Type1Cfg::bridge_ctl,       2}}
}};

PciDevBase::PciDevBase(uint64_t d_bdf, cfg_space_type cfg_len, pci_dev_type dev_type,
                       const fs::path &dev_path, std::unique_ptr<uint8_t []> cfg_buf) :
    dom_(d_bdf >> 24 & 0xffff),
    bus_(d_bdf >> 16 & 0xff),
    dev_(d_bdf >> 8 & 0xff),
    func_(d_bdf & 0xff),
    ids_names_(IDS_TYPES_CNT),
    dev_id_str_(fmt::format("[{:02x}:{:02x}.{:x}]", bus_, dev_, func_)),
    is_pcie_(false),
    cfg_type_(cfg_len),
    type_(dev_type),
    cfg_space_(std::move(cfg_buf)),
    sys_path_(dev_path)
    {}

void PciDevBase::parse_capabilities()
{
    auto reg_status = reinterpret_cast<const RegStatus *>
                      (cfg_space_.get() + e_to_type(Type0Cfg::status));
    if (!reg_status->cap_list)
        return;

    auto reg_cap_ptr = reinterpret_cast<const RegCapPtr *>
                      (cfg_space_.get() + e_to_type(Type0Cfg::cap_ptr));
    auto next_cap_off = reg_cap_ptr->ptr & 0xfc;

    while (next_cap_off != 0) {
            auto compat_cap = reinterpret_cast<const CompatCapHdr *>
                              (cfg_space_.get() + next_cap_off);
            auto cap_type = CompatCapID{compat_cap->cap_id};
            if (cap_type == CompatCapID::pci_express)
                is_pcie_ = true;

            caps_.emplace_back(CapType::compat, compat_cap->cap_id, 0, next_cap_off);
            next_cap_off = compat_cap->next_cap;
    }

    if (is_pcie_) {
        next_cap_off = ext_cap_cfg_off;
        while (next_cap_off != 0) {
            auto ext_cap = reinterpret_cast<const ExtCapHdr *>
                           (cfg_space_.get() + next_cap_off);
            auto cap_type = ExtCapID{ext_cap->cap_id};
            if (cap_type != ExtCapID::null_cap)
                caps_.emplace_back(CapType::extended, ext_cap->cap_id, ext_cap->cap_ver,
                                   next_cap_off);
            next_cap_off = ext_cap->next_cap;
        }
    }
}

void PciDevBase::dump_capabilities() noexcept
{
    logger.info("[{:02x}:{:02x}.{:x}]: {} capabilities >>>", bus_, dev_, func_, caps_.size());
    for (size_t i = 0; auto &cap : caps_) {
        auto cap_type = std::get<0>(cap);

        if (cap_type == CapType::compat) {
            auto compat_cap_type = CompatCapID{std::get<1>(cap)};
            logger.raw("[#{:2} {:#03x}] -> '{}'", i++, std::get<3>(cap),
                       CompatCapName(compat_cap_type));
        } else {
            auto ext_cap_type = ExtCapID{std::get<1>(cap)};
            logger.raw("[#{:2} {:#03x}] -> (EXT, ver {}) '{}'", i++, std::get<3>(cap),
                       std::get<2>(cap), ExtCapName(ext_cap_type));
        }
    }
}

void PciDevBase::GetResources() noexcept
{
    auto dev_resources = sysfs::get_resources(sys_path_);
    sysfs::dump_resources(dev_resources, dev_id_str_);
    resources_ = std::move(dev_resources);
}

void PciDevBase::ParseBars() noexcept
{
    auto num_bars = (type_ == pci_dev_type::TYPE0) ? 6 : 2;
    for (int i = 0; i < num_bars; i++) {
        auto [start, end, flags] = resources_[i];

        if (flags == 0) {
            assert(start == 0 && end == 0);

            bar_res_[i].type_ = ResourceType::EMPTY;
            continue;
        } else {
            if (flags & PCIResIO)
                bar_res_[i].type_ = ResourceType::IO;
            if (flags & PCIResMEM)
                bar_res_[i].type_ = ResourceType::MEMORY;

            if (flags & PCIResPrefetch)
                bar_res_[i].is_prefetchable_ = true;

            if (flags & PCIResMem64)
                bar_res_[i].is_64bit_ = true;
        }

        bar_res_[i].phys_addr_ = start;
        bar_res_[i].len_ = end - start + 1;
    }
}

void PciDevBase::ParseIDs(PciIdParser &parser)
{
    auto vid    = get_vendor_id();
    auto dev_id = get_device_id();
    auto cc     = get_class_code();

    ids_names_[VENDOR] = parser.vendor_name_lookup(vid);
    ids_names_[DEVICE] = parser.device_name_lookup(vid, dev_id);

    std::tie(ids_names_[CLASS],
             ids_names_[SUBCLASS],
             ids_names_[PROG_IFACE]) = parser.class_info_lookup(cc);
}

uint32_t PciDevBase::get_vendor_id() const noexcept
{
    return get_reg_compat(Type0Cfg::vid, t0_reg_map);
}

uint32_t PciDevBase::get_device_id() const noexcept
{
    return get_reg_compat(Type0Cfg::dev_id, t0_reg_map);
}

uint32_t PciDevBase::get_command() const noexcept
{
    return get_reg_compat(Type0Cfg::command, t0_reg_map);
}

uint32_t PciDevBase::get_status() const noexcept
{
    return get_reg_compat(Type0Cfg::status, t0_reg_map);
}

uint32_t PciDevBase::get_rev_id() const noexcept
{
    return get_reg_compat(Type0Cfg::revision, t0_reg_map);
}

uint32_t PciDevBase::get_class_code() const noexcept
{
    return get_reg_compat(Type0Cfg::class_code, t0_reg_map);
}

uint32_t PciDevBase::get_cache_line_size() const noexcept
{
    return get_reg_compat(Type0Cfg::cache_line_size, t0_reg_map);
}

uint32_t PciDevBase::get_lat_timer() const noexcept
{
    return get_reg_compat(Type0Cfg::latency_timer, t0_reg_map);
}

uint32_t PciDevBase::get_header_type() const noexcept
{
    return get_reg_compat(Type0Cfg::header_type, t0_reg_map);
}

uint32_t PciDevBase::get_bist() const noexcept
{
    return get_reg_compat(Type0Cfg::bist, t0_reg_map);
}

uint32_t PciDevBase::get_cap_ptr() const noexcept
{
    return get_reg_compat(Type0Cfg::cap_ptr, t0_reg_map);
}

uint32_t PciDevBase::get_itr_line() const noexcept
{
    return get_reg_compat(Type0Cfg::itr_line, t0_reg_map);
}

uint32_t PciDevBase::get_itr_pin() const noexcept
{
    return get_reg_compat(Type0Cfg::itr_pin, t0_reg_map);
}

//
// Type0 device registers
//

uint32_t PciType0Dev::get_bar0() const noexcept
{
    return get_reg_compat(Type0Cfg::bar0, t0_reg_map);
}

uint32_t PciType0Dev::get_bar1() const noexcept
{
    return get_reg_compat(Type0Cfg::bar1, t0_reg_map);
}

uint32_t PciType0Dev::get_bar2() const noexcept
{
    return get_reg_compat(Type0Cfg::bar2, t0_reg_map);
}

uint32_t PciType0Dev::get_bar3() const noexcept
{
    return get_reg_compat(Type0Cfg::bar3, t0_reg_map);
}

uint32_t PciType0Dev::get_bar4() const noexcept
{
    return get_reg_compat(Type0Cfg::bar4, t0_reg_map);
}

uint32_t PciType0Dev::get_bar5() const noexcept
{
    return get_reg_compat(Type0Cfg::bar5, t0_reg_map);
}

uint32_t PciType0Dev::get_cardbus_cis() const noexcept
{
    return get_reg_compat(Type0Cfg::cardbus_cis_ptr, t0_reg_map);
}

uint32_t PciType0Dev::get_subsys_vid() const noexcept
{
    return get_reg_compat(Type0Cfg::subsys_vid, t0_reg_map);
}

uint32_t PciType0Dev::get_subsys_dev_id() const noexcept
{
    return get_reg_compat(Type0Cfg::subsys_dev_id, t0_reg_map);
}

uint32_t PciType0Dev::get_exp_rom_bar() const noexcept
{
    return get_reg_compat(Type0Cfg::exp_rom_bar, t0_reg_map);
}

uint32_t PciType0Dev::get_min_gnt() const noexcept
{
    return get_reg_compat(Type0Cfg::min_gnt, t0_reg_map);
}

uint32_t PciType0Dev::get_max_lat() const noexcept
{
    return get_reg_compat(Type0Cfg::max_lat, t0_reg_map);
}

void PciType0Dev::ParseIDs(PciIdParser &parser)
{
    PciDevBase::ParseIDs(parser);

    auto vid           = get_vendor_id();
    auto dev_id        = get_device_id();
    auto subsys_vid    = get_subsys_vid();
    auto subsys_dev_id = get_subsys_dev_id();

    // Subsystem name is identified by a pair of <Subsystem Vendor ID, Subsystem Device ID>
    // If nothing has been found, subsystem name would be subsystem vendor ID name.
    ids_names_[SUBSYS_NAME] = parser.subsys_name_lookup(vid, dev_id, subsys_vid, subsys_dev_id);

    if (ids_names_[SUBSYS_NAME].empty())
        ids_names_[SUBSYS_VENDOR] = parser.vendor_name_lookup(subsys_vid);
}

void PciType0Dev::print_data() const noexcept {
    auto vid    = get_vendor_id();
    auto dev_id = get_device_id();
    logger.info("[{:04}:{:02x}:{:02x}.{:x}] -> TYPE 0: cfg_size {:4} vendor {:2x} | dev {:2x}",
               dom_, bus_, dev_, func_, e_to_type(cfg_type_), vid, dev_id);
}

//
// Type1 device registers
//

uint32_t PciType1Dev::get_bar0() const noexcept
{
    return get_reg_compat(Type1Cfg::bar0, t1_reg_map);
}

uint32_t PciType1Dev::get_bar1() const noexcept
{
    return get_reg_compat(Type1Cfg::bar1, t1_reg_map);
}
uint32_t PciType1Dev::get_prim_bus_num() const noexcept
{
    return get_reg_compat(Type1Cfg::prim_bus_num, t1_reg_map);
}
uint32_t PciType1Dev::get_sec_bus_num() const noexcept
{
    return get_reg_compat(Type1Cfg::sec_bus_num, t1_reg_map);
}
uint32_t PciType1Dev::get_sub_bus_num() const noexcept
{
    return get_reg_compat(Type1Cfg::sub_bus_num, t1_reg_map);
}
uint32_t PciType1Dev::get_sec_lat_timer() const noexcept
{
    return get_reg_compat(Type1Cfg::sec_lat_timer, t1_reg_map);
}

uint32_t PciType1Dev::get_io_base() const noexcept
{
    return get_reg_compat(Type1Cfg::io_base, t1_reg_map);
}

uint32_t PciType1Dev::get_io_limit() const noexcept
{
    return get_reg_compat(Type1Cfg::io_limit, t1_reg_map);
}

uint32_t PciType1Dev::get_sec_status() const noexcept
{
    return get_reg_compat(Type1Cfg::sec_status, t1_reg_map);
}

uint32_t PciType1Dev::get_mem_base() const noexcept
{
    return get_reg_compat(Type1Cfg::mem_base, t1_reg_map);
}

uint32_t PciType1Dev::get_mem_limit() const noexcept
{
    return get_reg_compat(Type1Cfg::mem_limit, t1_reg_map);
}

uint32_t PciType1Dev::get_pref_mem_base() const noexcept
{
    return get_reg_compat(Type1Cfg::pref_mem_base, t1_reg_map);
}

uint32_t PciType1Dev::get_pref_mem_limit() const noexcept
{
    return get_reg_compat(Type1Cfg::pref_mem_limit, t1_reg_map);
}

uint32_t PciType1Dev::get_pref_base_upper() const noexcept
{
    return get_reg_compat(Type1Cfg::pref_base_upper, t1_reg_map);
}

uint32_t PciType1Dev::get_pref_limit_upper() const noexcept
{
    return get_reg_compat(Type1Cfg::pref_limit_upper, t1_reg_map);
}

uint32_t PciType1Dev::get_io_base_upper() const noexcept
{
    return get_reg_compat(Type1Cfg::io_base_upper, t1_reg_map);
}

uint32_t PciType1Dev::get_io_limit_upper() const noexcept
{
    return get_reg_compat(Type1Cfg::io_limit_upper, t1_reg_map);
}

uint32_t PciType1Dev::get_exp_rom_bar() const noexcept
{
    return get_reg_compat(Type1Cfg::exp_rom_bar, t1_reg_map);
}

uint32_t PciType1Dev::get_bridge_ctl() const noexcept
{
    return get_reg_compat(Type1Cfg::bridge_ctl, t1_reg_map);
}

void PciType1Dev::print_data() const noexcept {
    auto dev_id = get_device_id();
    auto vid = *reinterpret_cast<uint16_t *>(cfg_space_.get() + e_to_type(Type1Cfg::vid));
    logger.info("[{:04}:{:02x}:{:02x}.{:x}] -> TYPE 1: cfg_size {:4} vendor {:2x} | dev {:2x}",
               dom_, bus_, dev_, func_, e_to_type(cfg_type_), vid, dev_id);
}

} // namespace pci
