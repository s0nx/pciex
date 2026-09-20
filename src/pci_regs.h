// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2026 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdint>
#include <string>

// Type 0 device configuration header register offsets
enum class Type0Cfg
{
    vid             = 0x0, // Vendor ID - 2 bytes
    dev_id          = 0x2, // Device ID - 2 bytes
    command         = 0x4, // Command - 2 bytes
    status          = 0x6, // Status - 2 bytes

    revision        = 0x8, // Revision ID - 1 byte
    class_code      = 0x9, // Class Code - 3 bytes

    cache_line_size = 0xc, // Cache Line Size - 1 byte
    latency_timer   = 0xd, // Latency Timer - 1 byte (not used by PCIe)
    header_type     = 0xe, // Header Type - 1 byte
    bist            = 0xf, // BIST - 1 byte

    bar0            = 0x10, // Base Address
    bar1            = 0x14,
    bar2            = 0x18,
    bar3            = 0x1c,
    bar4            = 0x20,
    bar5            = 0x24,

    cardbus_cis_ptr = 0x28, // Cardbus CIS Pointer (not used by PCIe)
    subsys_vid      = 0x2c, // Subsystem Vendor ID - 2 bytes
    subsys_dev_id   = 0x2e, // Subsysten Device ID - 2 bytes
    exp_rom_bar     = 0x30, // Expansion ROM Base Address - 4 bytes

    cap_ptr         = 0x34, // Capabilities Pointer - 1 byte

    // 0x35 - 0x37 - reserved
    // 0x38 - 0x3b - reserved

    itr_line        = 0x3c, // Interrupt Line - 1 byte
    itr_pin         = 0x3d, // Interrupt Pin - 1 byte

    min_gnt         = 0x3e, // Min_Gnt (not used by PCIe)
    max_lat         = 0x3f  // Max_Lat (not used by PCIe)
};

constexpr uint32_t type0_compat_reg_cnt = 25;

constexpr auto Type0RegName(const Type0Cfg reg) noexcept
{
    switch (reg) {
    case Type0Cfg::vid:
        return "Vendor ID";
    case Type0Cfg::dev_id:
        return "Device ID";
    case Type0Cfg::command:
        return "Command";
    case Type0Cfg::status:
        return "Status";
    case Type0Cfg::revision:
        return "Revision";
    case Type0Cfg::class_code:
        return "Class Code";
    case Type0Cfg::cache_line_size:
        return "Cache Line size";
    case Type0Cfg::latency_timer:
        return "Latency Timer";
    case Type0Cfg::header_type:
        return "Header Type";
    case Type0Cfg::bist:
        return "BIST";
    case Type0Cfg::bar0:
        return "BAR 0";
    case Type0Cfg::bar1:
        return "BAR 1";
    case Type0Cfg::bar2:
        return "BAR 2";
    case Type0Cfg::bar3:
        return "BAR 3";
    case Type0Cfg::bar4:
        return "BAR 4";
    case Type0Cfg::bar5:
        return "BAR 5";
    case Type0Cfg::cardbus_cis_ptr:
        return "Cardbus CIS Pointer";
    case Type0Cfg::subsys_vid:
        return "Subsystem Vendor ID";
    case Type0Cfg::subsys_dev_id:
        return "Subsystem ID";
    case Type0Cfg::exp_rom_bar:
        return "Expansion ROM BAR";
    case Type0Cfg::cap_ptr:
        return "Capabilities Pointer";
    case Type0Cfg::itr_line:
        return "Interrupt Line";
    case Type0Cfg::itr_pin:
        return "Interrupt Pin";
    case Type0Cfg::min_gnt:
        return "Min_Gnt";
    case Type0Cfg::max_lat:
        return "Max_Lat";
    default:
        return "";
    }
}

// Type 1 device configuration header offsets
enum class Type1Cfg
{
    vid              = 0x0, // Vendor ID - 2 bytes
    dev_id           = 0x2, // Device ID - 2 bytes
    command          = 0x4, // Command - 2 bytes
    status           = 0x6, // Status - 2 bytes

    revision         = 0x8, // Revision ID - 1 byte
    class_code       = 0x9, // Class Code - 3 bytes

    cache_line_size  = 0xc, // Cache Line Size - 1 byte
    prim_lat_timer   = 0xd, // Latency Timer - 1 byte (not used by PCIe)
    header_type      = 0xe, // Header Type - 1 byte
    bist             = 0xf, // BIST - 1 byte

    bar0             = 0x10, // Base Address
    bar1             = 0x14,

    prim_bus_num     = 0x18, // Primary Bus Number - 1 byte
    sec_bus_num      = 0x19, // Secondary Bus Number - 1 byte
    sub_bus_num      = 0x1a, // Subordinate Bus Number - 1 byte
    sec_lat_timer    = 0x1b, // (not used by PCIe)

    io_base          = 0x1c, // I/O Base - 1 byte
    io_limit         = 0x1d, // I/O Limit - 1 byte
    sec_status       = 0x1e, // Secondary Status - 2 bytes

    mem_base         = 0x20, // Memory Base - 2 bytes
    mem_limit        = 0x22, // Memory Limit - 2 bytes

    pref_mem_base    = 0x24, // Prefetchable Memory Base - 2 bytes
    pref_mem_limit   = 0x26, // Prefetchable Memory Limit - 2 bytes

    pref_base_upper  = 0x28, // Prefetchable Mmemory Base (upper 32 bits) - 4 bytes
    pref_limit_upper = 0x2c, // Prefetchable Memory Limit (upper 32 bits) - 4 bytes

    io_base_upper    = 0x30, // I/O Base (upper 16 bits) - 2 bytes
    io_limit_upper   = 0x32, // I/O Limit (upper 16 bits) - 2 bytes

    cap_ptr          = 0x34, // Capabilities Pointer - 1 byte
    // 0x35 - 0x37 - reserved
    exp_rom_bar      = 0x38, // Expansion ROM Base Address - 4 bytes

    itr_line         = 0x3c, // Interrupt Line - 1 byte
    itr_pin          = 0x3d, // Interrupt Pin - 1 byte

    bridge_ctl       = 0x3e // Bridge Control - 2 bytes
};

constexpr uint32_t type1_compat_reg_cnt = 32;

constexpr auto Type1RegName(const Type1Cfg reg) noexcept
{
    switch (reg) {
    case Type1Cfg::vid:
        return "Vendor ID";
    case Type1Cfg::dev_id:
        return "Device ID";
    case Type1Cfg::command:
        return "Command";
    case Type1Cfg::status:
        return "Status";
    case Type1Cfg::revision:
        return "Revision";
    case Type1Cfg::class_code:
        return "Class Code";
    case Type1Cfg::cache_line_size:
        return "Cache Line size";
    case Type1Cfg::prim_lat_timer:
        return "Prim Lat Timer";
    case Type1Cfg::header_type:
        return "Header Type";
    case Type1Cfg::bist:
        return "BIST";
    case Type1Cfg::bar0:
        return "BAR 0";
    case Type1Cfg::bar1:
        return "BAR 1";
    case Type1Cfg::prim_bus_num:
        return "Prim Bus Number";
    case Type1Cfg::sec_bus_num:
        return "Sec Bus Number";
    case Type1Cfg::sub_bus_num:
        return "Sub Bus Number";
    case Type1Cfg::sec_lat_timer:
        return "Sec Lat Timer";
    case Type1Cfg::io_base:
        return "I/O Base";
    case Type1Cfg::io_limit:
        return "I/O Limit";
    case Type1Cfg::sec_status:
        return "Secondary Status";
    case Type1Cfg::mem_base:
        return "Memory Base";
    case Type1Cfg::mem_limit:
        return "Memory Limit";
    case Type1Cfg::pref_mem_base:
        return "Prefetchable Memory Base";
    case Type1Cfg::pref_mem_limit:
        return "Prefetchable Memory Limit";
    case Type1Cfg::pref_base_upper:
        return "Prefetchable Base Upper 32 Bits";
    case Type1Cfg::pref_limit_upper:
        return "Prefetchable Limit Upper 32 Bits";
    case Type1Cfg::io_base_upper:
        return "I/O Base Upper 16 Bits";
    case Type1Cfg::io_limit_upper:
        return "I/O Limit Upper 16 Bits";
    case Type1Cfg::cap_ptr:
        return "Capabilities Pointer";
    case Type1Cfg::exp_rom_bar:
        return "Expansion ROM BAR";
    case Type1Cfg::itr_line:
        return "Interrupt Line";
    case Type1Cfg::itr_pin:
        return "Interrupt Pin";
    case Type1Cfg::bridge_ctl:
        return "Bridge Control";
    default:
        return "";
    }
}

struct RegVendorID
{
    uint16_t vid;
} __attribute__((packed));
static_assert(sizeof(RegVendorID) == 2);

struct RegDeviceID
{
    uint16_t dev_id;
} __attribute__((packed));
static_assert(sizeof(RegDeviceID) == 2);

struct RegCommand
{
    uint16_t    io_space_ena : 1;
    uint16_t   mem_space_ena : 1;
    uint16_t  bus_master_ena : 1;
    uint16_t           rsvd0 : 3;
    uint16_t parity_err_resp : 1;
    uint16_t           rsvd1 : 1;
    uint16_t        serr_ena : 1;
    uint16_t           rsvd2 : 1;
    uint16_t     itr_disable : 1;
    uint16_t           rsvd3 : 5;
} __attribute__((packed));
static_assert(sizeof(RegCommand) == 2);

struct RegStatus
{
    uint16_t          imm_readiness : 1;
    uint16_t                  rsvd0 : 2;
    uint16_t             itr_status : 1;
    uint16_t               cap_list : 1;
    uint16_t                  rsvd1 : 3;
    uint16_t master_data_parity_err : 1;
    uint16_t                  rsvd2 : 2;
    uint16_t        signl_tgt_abort : 1;
    uint16_t     received_tgt_abort : 1;
    uint16_t  recevied_master_abort : 1;
    uint16_t          signl_sys_err : 1;
    uint16_t    detected_parity_err : 1;
} __attribute__((packed));
static_assert(sizeof(RegStatus) == 2);

struct RegRevID
{
    uint8_t rev_id;
} __attribute__((packed));
static_assert(sizeof(RegRevID) == 1);

struct RegClassCode
{
    uint8_t prog_iface;
    uint8_t sub_class_code;
    uint8_t base_class_code;
} __attribute__((packed));
static_assert(sizeof(RegClassCode) == 3);

struct RegCacheLineSize
{
    uint8_t cl_size;
} __attribute__((packed));
static_assert(sizeof(RegCacheLineSize) == 1);

struct RegLatTimer
{
    uint8_t lat_tmr;
} __attribute__((packed));
static_assert(sizeof(RegLatTimer) == 1);

struct RegHdrType
{
    uint8_t hdr_layout : 7;
    uint8_t     is_mfd : 1;
} __attribute__((packed));
static_assert(sizeof(RegHdrType) == 1);

struct RegBIST
{
    uint8_t   cpl_code : 4;
    uint8_t       rsvd : 2;
    uint8_t start_bist : 1;
    uint8_t   bist_cap : 1;
} __attribute__((packed));
static_assert(sizeof(RegBIST) == 1);

struct RegCapPtr
{
    uint8_t ptr;
} __attribute__((packed));
static_assert(sizeof(RegCapPtr) == 1);

struct RegItrLine
{
    uint8_t line;
} __attribute__((packed));
static_assert(sizeof(RegItrLine) == 1);

struct RegItrPin
{
    uint8_t pin;
} __attribute__((packed));
static_assert(sizeof(RegItrPin) == 1);

struct RegBARMem
{
    uint32_t space_type : 1;
    uint32_t       type : 2;
    uint32_t   prefetch : 1;
    uint32_t       addr : 28;
} __attribute__((packed));
static_assert(sizeof(RegBARMem) == 4);

struct RegBARIo
{
    uint32_t space_type : 1;
    uint32_t       rsvd : 1;
    uint32_t       addr : 30;
} __attribute__((packed));
static_assert(sizeof(RegBARIo) == 4);

struct RegCardbusCIS
{
    uint32_t ptr;
} __attribute__((packed));
static_assert(sizeof(RegCardbusCIS) == 4);

struct RegSubsysVID
{
    uint16_t vid;
} __attribute__((packed));
static_assert(sizeof(RegSubsysVID) == 2);

struct RegSubsysID
{
    uint16_t id;
} __attribute__((packed));
static_assert(sizeof(RegSubsysID) == 2);

struct RegExpROMBar
{
    uint32_t  ena : 1;
    uint32_t rsvd : 10;
    uint32_t  bar : 21;
} __attribute__((packed));
static_assert(sizeof(RegExpROMBar) == 4);

struct RegMinGnt
{
    uint8_t data;
} __attribute__((packed));
static_assert(sizeof(RegMinGnt) == 1);

struct RegMaxLat
{
    uint8_t data;
} __attribute__((packed));
static_assert(sizeof(RegMaxLat) == 1);

struct RegPrimBusNum
{
    uint8_t num;
} __attribute__((packed));
static_assert(sizeof(RegPrimBusNum) == 1);

struct RegSecBusNum
{
    uint8_t num;
} __attribute__((packed));
static_assert(sizeof(RegSecBusNum) == 1);

struct RegSubBusNum
{
    uint8_t num;
} __attribute__((packed));
static_assert(sizeof(RegSubBusNum) == 1);

struct RegIOBase
{
    uint8_t  cap : 4;
    uint8_t addr : 4;
} __attribute__((packed));
static_assert(sizeof(RegIOBase) == 1);

struct RegIOLimit
{
    uint8_t  cap : 4;
    uint8_t addr : 4;
} __attribute__((packed));
static_assert(sizeof(RegIOLimit) == 1);

struct RegSecStatus
{
    uint16_t                rsvd : 5;
    uint16_t           mhz66_cap : 1;
    uint16_t               rsvd1 : 1;
    uint16_t  fast_b2b_trans_cap : 1;
    uint16_t master_data_par_err : 1;
    uint16_t       devsel_timing : 2;
    uint16_t  signaled_tgt_abort : 1;
    uint16_t      recv_tgt_abort : 1;
    uint16_t   recv_master_abort : 1;
    uint16_t        recv_sys_err : 1;
    uint16_t   detect_parity_err : 1;
} __attribute__((packed));
static_assert(sizeof(RegSecStatus) == 2);

// Used for Memory Base/Limit, Prefetchable Memory Base/Limit registers
struct RegMemBL
{
    uint16_t rsvd : 4;
    uint16_t addr : 12;
} __attribute__((packed));
static_assert(sizeof(RegMemBL) == 2);

struct RegPrefMemBL
{
    uint16_t  cap : 4;
    uint16_t addr : 12;
} __attribute__((packed));
static_assert(sizeof(RegPrefMemBL) == 2);

struct RegIOUpperBL
{
    uint16_t addr;
} __attribute__((packed));
static_assert(sizeof(RegIOUpperBL) == 2);

struct RegBridgeCtl
{
    uint16_t  parity_err_resp_ena : 1;
    uint16_t             serr_ena : 1;
    uint16_t              isa_ena : 1;
    uint16_t              vga_ena : 1;
    uint16_t     vga_16bit_decode : 1;
    uint16_t    master_abort_mode : 1;
    uint16_t        sec_bus_reset : 1;
    uint16_t   fast_b2b_trans_ena : 1;
    uint16_t     prim_discard_tmr : 1;
    uint16_t      sec_discard_tmr : 1;
    uint16_t   discard_tmr_status : 1;
    uint16_t discard_tmr_serr_ena : 1;
    uint16_t                 rsvd : 4;
} __attribute__((packed));
static_assert(sizeof(RegBridgeCtl) == 2);

struct CompatCapHdr
{
    uint8_t cap_id;
    uint8_t next_cap;
} __attribute__((packed));
static_assert(sizeof(CompatCapHdr) == 2);

struct ExtCapHdr
{
    uint16_t  cap_id;
    uint16_t  cap_ver : 4;
    uint16_t next_cap : 12;
} __attribute__((packed));
static_assert(sizeof(ExtCapHdr) == 4);

struct RegPciECap
{
    uint16_t       cap_ver : 4;
    uint16_t dev_port_type : 4;
    uint16_t     slot_impl : 1;
    uint16_t   itr_msg_num : 5;
    uint16_t         rsvd  : 2;
} __attribute__((packed));
static_assert(sizeof(RegPciECap) == 2);

constexpr auto PciEDevPortDescType0(const uint8_t val) noexcept
{
    switch (val) {
    case 0b0000:
        return "PCI Express Endpoint";
    case 0b0001:
        return "Legacy PCI Express Endpoint";
    case 0b1001:
        return "RCiEP";
    case 0b1010:
        return "RC Event Collector";
    default:
        return "< undefined >";
    }
}

constexpr auto PciEDevPortDescType1(const uint8_t val) noexcept
{
    switch (val) {
    case 0b0100:
        return "Root Port of PCIe RC";
    case 0b0101:
        return "Upstream Port of PCIe switch";
    case 0b0110:
        return "Downstream Port of PCIe switch";
    case 0b0111:
        return "PCIe -> PCI/PCIX bridge";
    case 0b1000:
        return "PCI/PCIX -> PCIe bridge";
    default:
        return "< undefined >";
    }
}

struct RegDevCap
{
    uint32_t max_pyld_size_supported : 3;
    uint32_t     phan_func_supported : 2;
    uint32_t ext_tag_field_supported : 1;
    uint32_t       ep_l0s_accept_lat : 3;
    uint32_t        ep_l1_accept_lat : 3;
    uint32_t                    rsvd : 3;
    uint32_t      role_based_err_rep : 1;
    uint32_t    cap_slot_pwr_lim_val : 8;
    uint32_t  cap_slot_pwr_lim_scale : 2;
    uint32_t                 flr_cap : 1;
    uint32_t                   rsvd1 : 3;
} __attribute__((packed));
static_assert(sizeof(RegDevCap) == 4);

constexpr auto EpL0sAcceptLatDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0b000:
        return "64 ns";
    case 0b001:
        return "128 ns";
    case 0b010:
        return "256 ns";
    case 0b011:
        return "512 ns";
    case 0b100:
        return "1 us";
    case 0b101:
        return "2 us";
    case 0b110:
        return "4 us";
    case 0b111:
        return "no limit";
    default:
        return "< undefined >";
    }
}

constexpr auto EpL1AcceptLatDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0b000:
        return "1 us";
    case 0b001:
        return "2 us";
    case 0b010:
        return "4 us";
    case 0b011:
        return "8 us";
    case 0b100:
        return "16 us";
    case 0b101:
        return "32 us";
    case 0b110:
        return "64 us";
    case 0b111:
        return "no limit";
    default:
        return "< undefined >";
    }
}

constexpr auto CapSlotPWRScale(const uint8_t val) noexcept
{
    switch (val) {
    case 0b00:
        return "1x";
    case 0b01:
        return "0.1x";
    case 0b10:
        return "0.01x";
    case 0b11:
        return "0.001x";
    default:
        return "< undefined >";
    }
}

struct RegDevCtl
{
    uint16_t     correct_err_rep_ena : 1;
    uint16_t   non_fatal_err_rep_ena : 1;
    uint16_t       fatal_err_rep_ena : 1;
    uint16_t unsupported_req_rep_ena : 1;
    uint16_t       relaxed_order_ena : 1;
    uint16_t           max_pyld_size : 3;
    uint16_t       ext_tag_field_ena : 1;
    uint16_t           phan_func_ena : 1;
    uint16_t        aux_power_pm_ena : 1;
    uint16_t            no_snoop_ena : 1;
    uint16_t       max_read_req_size : 3;
    uint16_t brd_conf_retry_init_flr : 1;
} __attribute__((packed));
static_assert(sizeof(RegDevCtl) == 2);

struct RegDevStatus
{
    uint16_t         corr_err_detected : 1;
    uint16_t    non_fatal_err_detected : 1;
    uint16_t        fatal_err_detected : 1;
    uint16_t  unsupported_req_detected : 1;
    uint16_t          aux_pwr_detected : 1;
    uint16_t             trans_pending : 1;
    uint16_t emerg_pwr_reduct_detected : 1;
    uint16_t                      rsvd : 9;
} __attribute__((packed));
static_assert(sizeof(RegDevStatus) == 2);

struct RegLinkCap
{
    uint32_t               max_link_speed : 4;
    uint32_t               max_link_width : 6;
    uint32_t                 aspm_support : 2;
    uint32_t                 l0s_exit_lat : 3;
    uint32_t                  l1_exit_lat : 3;
    uint32_t                  clk_pwr_mng : 1;
    uint32_t       surpr_down_err_rep_cap : 1;
    uint32_t dlink_layer_link_act_rep_cap : 1;
    uint32_t           link_bw_notify_cap : 1;
    uint32_t               aspm_opt_compl : 1;
    uint32_t                         rsvd : 1;
    uint32_t                     port_num : 8;
} __attribute__((packed));
static_assert(sizeof(RegLinkCap) == 4);

constexpr auto LinkWidthDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0b0000001:
        return "x1";
    case 0b000010:
        return "x2";
    case 0b000100:
        return "x4";
    case 0b001000:
        return "x8";
    case 0b001100:
        return "x12";
    case 0b010000:
        return "x16";
    case 0b100000:
        return "x32";
    default:
        return "< undefined >";
    }
}

constexpr auto LinkCapL0sExitLat(const uint8_t val) noexcept
{
    switch (val) {
    case 0b000:
        return "< 64 ns";
    case 0b001:
        return "64 ns - 128 ns";
    case 0b010:
        return "128 ns - 256 ns";
    case 0b011:
        return "256 ns - 512 ns";
    case 0b100:
        return "512 ns - 1 us";
    case 0b101:
        return "1 us - 2 us";
    case 0b110:
        return "2 us - 4 us";
    case 0b111:
        return "> 4 us";
    default:
        return "< undefined >";
    }
}

constexpr auto LinkCapL1ExitLat(const uint8_t val) noexcept
{
    switch (val) {
    case 0b000:
        return "< 1 us";
    case 0b001:
        return "1 us - 2 us";
    case 0b010:
        return "2 us - 4 us";
    case 0b011:
        return "4 us - 8 us";
    case 0b100:
        return "8 us - 16 us";
    case 0b101:
        return "16 us - 32 us";
    case 0b110:
        return "32 us - 64 us";
    case 0b111:
        return "> 64 us";
    default:
        return "< undefined >";
    }
}

struct RegLinkCtl
{
    uint16_t                 aspm_ctl : 2;
    uint16_t                    rsvd0 : 1;
    uint16_t                      rcb : 1;
    uint16_t             link_disable : 1;
    uint16_t             retrain_link : 1;
    uint16_t          common_clk_conf : 1;
    uint16_t                ext_synch : 1;
    uint16_t               clk_pm_ena : 1;
    uint16_t    hw_auto_width_disable : 1;
    uint16_t      link_bw_mng_itr_ena : 1;
    uint16_t link_auto_bw_mng_itr_ena : 1;
    uint16_t                    rsvd1 : 3;
    uint16_t            drs_signl_ctl : 1;
} __attribute__((packed));
static_assert(sizeof(RegLinkCtl) == 2);

constexpr auto LinkCtlDrsSigCtlDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0b00:
        return "DRS not reported";
    case 0b01:
        return "DRS itr enabled";
    case 0b10:
        return "DRS -> FRS signaling enabled";
    default:
        return "< undefined >";
    }
}

struct RegLinkStatus
{
    uint16_t          curr_link_speed : 4;
    uint16_t    negotiated_link_width : 6;
    uint16_t                     rsvd : 1;
    uint16_t            link_training : 1;
    uint16_t            slot_clk_conf : 1;
    uint16_t data_link_layer_link_act : 1;
    uint16_t       link_bw_mng_status : 1;
    uint16_t      link_auto_bw_status : 1;
} __attribute__((packed));
static_assert(sizeof(RegLinkStatus) == 2);

struct RegSlotCap
{
    uint32_t       attn_btn_pres : 1;
    uint32_t        pwr_ctl_pres : 1;
    uint32_t       mrl_sens_pres : 1;
    uint32_t       attn_ind_pres : 1;
    uint32_t        pwr_ind_pres : 1;
    uint32_t      hot_plug_surpr : 1;
    uint32_t        hot_plug_cap : 1;
    uint32_t    slot_pwr_lim_val : 8;
    uint32_t  slot_pwr_lim_scale : 2;
    uint32_t   em_interlock_pres : 1;
    uint32_t no_cmd_cmpl_support : 1;
    uint32_t       phys_slot_num : 13;
} __attribute__((packed));
static_assert(sizeof(RegSlotCap) == 4);

constexpr auto SlotCapPWRLimitDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0xf0:
        return "250 W";
    case 0xf1:
        return "275 W";
    case 0xf2:
        return "300 W";
    default:
        if (val >= 0xf3)
            return "> 300 W";
        else
            return "< undefined >";
    }
}

struct RegSlotCtl
{
    uint16_t             attn_btn_pres_ena : 1;
    uint16_t        pwr_fault_detected_ena : 1;
    uint16_t          mrl_sens_changed_ena : 1;
    uint16_t       pres_detect_changed_ena : 1;
    uint16_t              cmd_cmpl_itr_ena : 1;
    uint16_t              hot_plug_itr_ena : 1;
    uint16_t                  attn_ind_ctl : 2;
    uint16_t                   pwr_ind_ctl : 2;
    uint16_t                   pwr_ctl_ctl : 1;
    uint16_t              em_interlock_ctl : 1;
    uint16_t dlink_layer_state_changed_ena : 1;
    uint16_t         auto_slow_prw_lim_dis : 1;
    uint16_t                          rsvd : 2;
} __attribute__((packed));
static_assert(sizeof(RegSlotCtl) == 2);

constexpr auto SlotCtlIndCtrlDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0b00:
        return "Rsvd";
    case 0b01:
        return "On";
    case 0b10:
        return "Blink";
    case 0b11:
        return "Off";
    default:
        return "< undefined >";
    }
}

struct RegSlotStatus
{
    uint16_t             attn_btn_pres : 1;
    uint16_t        pwr_fault_detected : 1;
    uint16_t          mrl_sens_changed : 1;
    uint16_t       pres_detect_changed : 1;
    uint16_t                  cmd_cmpl : 1;
    uint16_t            mrl_sens_state : 1;
    uint16_t         pres_detect_state : 1;
    uint16_t       em_interlock_status : 1;
    uint16_t dlink_layer_state_changed : 1;
    uint16_t                      rsvd : 7;
} __attribute__((packed));
static_assert(sizeof(RegSlotStatus) == 2);

struct RegRootCtl
{
    uint16_t sys_err_on_correct_err_ena : 1;
    uint16_t sys_err_on_non_fat_err_ena : 1;
    uint16_t     sys_err_on_fat_err_ena : 1;
    uint16_t                pme_itr_ena : 1;
    uint16_t             crs_sw_vis_ena : 1;
    uint16_t                       rsvd : 11;
} __attribute__((packed));
static_assert(sizeof(RegRootCtl) == 2);

struct RegRootCap
{
    uint16_t crs_sw_vis : 1;
    uint16_t       rsvd : 15;
} __attribute__((packed));
static_assert(sizeof(RegRootCap) == 2);

struct RegRootStatus
{
    uint32_t  pme_req_id : 16;
    uint32_t  pme_status : 1;
    uint32_t pme_pending : 1;
    uint32_t        rsvd : 14;
} __attribute__((packed));
static_assert(sizeof(RegRootStatus) == 4);

struct RegDevCap2
{
    uint32_t  cmpl_timeout_rng_support : 4;
    uint32_t  cmpl_timeout_dis_support : 1;
    uint32_t           ari_fwd_support : 1;
    uint32_t   atomic_op_route_support : 1;
    uint32_t atomic_op_32_cmpl_support : 1;
    uint32_t atomic_op_64_cmpl_support : 1;
    uint32_t      cas_128_cmpl_support : 1;
    uint32_t    no_ro_ena_prpr_passing : 1;
    uint32_t               ltr_support : 1;
    uint32_t          tph_cmpl_support : 2;
    uint32_t                ln_sys_cls : 2;
    uint32_t    tag_10bit_cmpl_support : 1;
    uint32_t     tag_10bit_req_support : 1;
    uint32_t            obff_supported : 2;
    uint32_t     ext_fmt_field_support : 1;
    uint32_t  end_end_tlp_pref_support : 1;
    uint32_t      max_end_end_tlp_pref : 2;
    uint32_t  emerg_pwr_reduct_support : 2;
    uint32_t emerg_pwr_reduct_init_req : 1;
    uint32_t                      rsvd : 4;
    uint32_t               frs_support : 1;
} __attribute__((packed));
static_assert(sizeof(RegDevCap2) == 4);

constexpr auto DevCap2LNSysCLSDesc(const uint8_t val) noexcept
{
    switch (val) {
    case 0b00:
        return "[not supported]";
    case 0b01:
        return "LN compl 64b CLs";
    case 0b10:
        return "LN compl 128b CLs";
    case 0b11:
        return "rsvd";
    default:
        return "< undefined >";
    }
}

struct RegDevCtl2
{
    uint16_t       cmpl_timeout_val : 4;
    uint16_t       cmpl_timeout_dis : 1;
    uint16_t            ari_fwd_ena : 1;
    uint16_t      atomic_op_req_ena : 1;
    uint16_t    atomic_op_egr_block : 1;
    uint16_t            ido_req_ena : 1;
    uint16_t           ido_cmpl_ena : 1;
    uint16_t                ltr_ena : 1;
    uint16_t   emerg_pwr_reduct_req : 1;
    uint16_t      tag_10bit_req_ena : 1;
    uint16_t               obff_ena : 2;
    uint16_t end_end_tlp_pref_block : 1;
} __attribute__((packed));
static_assert(sizeof(RegDevCtl2) == 2);

constexpr auto CmplTimeoutValueDesc(const uint8_t val)
{
    switch (val) {
    case 0b0000:
        return "50 us - 50 ms";
    case 0b0001:
        return "50 us - 100 us";
    case 0b0010:
        return "1 ms - 10 ms";
    case 0b0101:
        return "16 ms - 55 ms";
    case 0b0110:
        return "65 ms - 210 ms";
    case 0b1001:
        return "260 ms - 900 ms";
    case 0b1010:
        return "1 s - 3.5 s";
    case 0b1101:
        return "4 s - 13 s";
    case 0b1110:
        return "17 s - 64 s";
    default:
        return "[rsvd]";
    }
}

constexpr auto DevCtl2ObffDesc(const uint8_t val)
{
    switch (val) {
    case 0b00:
        return "disabled";
    case 0b01:
        return "enabled [msg sign A]";
    case 0b10:
        return "enabled [msg sign B]";
    case 0b11:
        return "enabled [#WAKE sign]";
    default:
        return "[rsvd]";
    }
}

struct RegDevStatus2
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegDevStatus2) == 2);

struct RegLinkCap2
{
    uint32_t                         rsvd0 : 1;
    uint32_t           supported_speed_vec : 7;
    uint32_t             crosslink_support : 1;
    uint32_t low_skp_os_gen_supp_speed_vec : 7;
    uint32_t low_skp_os_rec_supp_speed_vec : 7;
    uint32_t     retmr_pres_detect_support : 1;
    uint32_t two_retmr_pres_detect_support : 1;
    uint32_t                         rsvd1 : 6;
    uint32_t                   drs_support : 1;
} __attribute__((packed));
static_assert(sizeof(RegLinkCap2) == 4);

struct RegLinkCtl2
{
    uint16_t            tgt_link_speed : 4;
    uint16_t          enter_compliance : 1;
    uint16_t         hw_auto_speed_dis : 1;
    uint16_t            select_de_emph : 1;
    uint16_t              trans_margin : 3;
    uint16_t      enter_mod_compliance : 1;
    uint16_t            compliance_sos : 1;
    uint16_t compliance_preset_de_emph : 4;
} __attribute__((packed));
static_assert(sizeof(RegLinkCtl2) == 2);

constexpr auto LinkSpeedBitDesc(const uint8_t val)
{
    switch (val) {
    case 0b00:
        return "2.5 GT/s";
    case 0b01:
        return "5.0 GT/s";
    case 0b10:
        return "8.0 GT/s";
    case 0b11:
        return "16.0 GT/s";
    default:
        return "< undefined >";
    }
}

struct RegLinkStatus2
{
    uint16_t curr_de_emph_lvl      : 1;
    uint16_t eq_8gts_compl         : 1;
    uint16_t eq_8gts_ph1_success   : 1;
    uint16_t eq_8gts_ph2_success   : 1;
    uint16_t eq_8gts_ph3_success   : 1;
    uint16_t link_eq_req_8gts      : 1;
    uint16_t retmr_pres_detect     : 1;
    uint16_t two_retmr_pres_detect : 1;
    uint16_t crosslink_resolution  : 2;
    uint16_t rsvd                  : 2;
    uint16_t downstream_comp_pres  : 3;
    uint16_t drs_msg_recv          : 1;
} __attribute__((packed));
static_assert(sizeof(RegLinkStatus2) == 2);

constexpr auto CrosslinkResDesc(const uint8_t val)
{
    switch (val) {
    case 0b00:
        return "[not supported]";
    case 0b01:
        return "upstream port";
    case 0b10:
        return "downstream port";
    case 0b11:
        return "crosslink negotiation not completed";
    default:
        return "< undefined >";
    }
}

constexpr auto DownstreamCompPresDesc(const uint8_t val)
{
    switch (val) {
    case 0b000:
        return "link down [pres not determined]";
    case 0b001:
        return "link down [comp not present]";
    case 0b010:
        return "link down [comp present]";
    case 0b100:
        return "link up [comp present]";
    case 0b101:
        return "link up [comp present + DRS]";
    case 0b011:
    case 0b110:
    case 0b111:
        return "[rsvd]";
    default:
        return "< undefined >";
    }
}

struct RegSlotCap2
{
    uint32_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegSlotCap2) == 4);

struct RegSlotCtl2
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegSlotCtl2) == 2);

struct RegSlotStatus2
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegSlotStatus2) == 2);

// PCIe capability strucuture
struct PciECap
{
    CompatCapHdr   hdr;          // 0x0
    RegPciECap     pcie_cap_reg; // 0x2
    RegDevCap      dev_cap;      // 0x4
    RegDevCtl      dev_ctl;      // 0x8
    RegDevStatus   dev_status;   // 0xa
    RegLinkCap     link_cap;     // 0xc
    RegLinkCtl     link_ctl;     // 0x10
    RegLinkStatus  link_status;  // 0x12
    RegSlotCap     slot_cap;     // 0x14
    RegSlotCtl     slot_ctl;     // 0x18
    RegSlotStatus  slot_status;  // 0x1a
    RegRootCtl     root_ctl;     // 0x1c
    RegRootCap     root_cap;     // 0x1e
    RegRootStatus  root_status;  // 0x20

    RegDevCap2     dev_cap2;     // 0x24
    RegDevCtl2     dev_ctl2;     // 0x28
    RegDevStatus2  dev_status2;  // 0x2a
    RegLinkCap2    link_cap2;    // 0x2c
    RegLinkCtl2    link_ctl2;    // 0x30
    RegLinkStatus2 link_status2; // 0x32
    RegSlotCap2    slot_cap2;    // 0x34
    RegSlotCtl2    slot_ctl2;    // 0x38
    RegSlotStatus2 slot_status2; // 0x3a
} __attribute__((packed, aligned(4)));
static_assert(sizeof(PciECap) == 0x3c);

struct RegMSIxMsgCtrl
{
    uint16_t table_size : 11;
    uint16_t rsvd       : 3;
    uint16_t func_mask  : 1;
    uint16_t msix_ena   : 1;
} __attribute__((packed));
static_assert(sizeof(RegMSIxMsgCtrl) == 0x2);

struct RegMSIxTblOffId
{
    uint32_t tbl_bar_entry : 3;
    uint32_t tbl_off       : 29;
} __attribute__((packed));
static_assert(sizeof(RegMSIxTblOffId) == 0x4);

struct RegMSIxPBAOffId
{
    uint32_t pba_bar_entry : 3;
    uint32_t pba_off       : 29;
} __attribute__((packed));
static_assert(sizeof(RegMSIxPBAOffId) == 0x4);

// PCI MSI-X capability structure
struct PciMSIxCap
{
    CompatCapHdr    hdr;        // 0x0
    RegMSIxMsgCtrl  msg_ctrl;   // 0x2
    RegMSIxTblOffId tbl_off_id; // 0x4
    RegMSIxPBAOffId pba_off_id; // 0x8
} __attribute__((packed));
static_assert(sizeof(PciMSIxCap) == 0xc);

// various formatting helpers
enum class LinkSpeedRepType
{
    max,
    current,
    target
};

std::string LinkSpeedDesc(LinkSpeedRepType rep_type, const uint8_t link_speed,
                          const RegLinkCap2 &link_cap2);
std::string CmplTimeoutRangesDesc(const RegDevCap2 &dev_cap2);
std::string SuppLinkSpeedDesc(const uint8_t link_speed_vector);


struct RegPMCap
{
    uint16_t version                 : 3;
    uint16_t pme_clk                 : 1;
    uint16_t imm_readiness_on_ret_d0 : 1;
    uint16_t dsi                     : 1;
    uint16_t aux_cur                 : 3;
    uint16_t d1_support              : 1;
    uint16_t d2_support              : 1;
    uint16_t pme_support             : 5;
} __attribute__((packed));
static_assert(sizeof(RegPMCap) == 0x2);

struct RegPMCtlStatus
{
    uint32_t pwr_state     : 2;
    uint32_t rsvd0         : 1;
    uint32_t no_soft_reset : 1;
    uint32_t rsvd1         : 4;
    uint32_t pme_en        : 1;
    uint32_t data_select   : 4;
    uint32_t data_scale    : 2;
    uint32_t pme_status    : 1;
    uint32_t rsvd2         : 6;
    uint32_t rsvd3         : 1;
    uint32_t rsvd4         : 1;
    uint32_t data          : 8;
} __attribute__((packed));
static_assert(sizeof(RegPMCtlStatus) == 0x4);

struct PciPMCap
{
    CompatCapHdr   hdr;          // 0x0
    RegPMCap       pmcap;        // 0x2
    RegPMCtlStatus pmcs;         // 0x4
} __attribute__((packed));
static_assert(sizeof(PciPMCap) == 0x8);

struct CompatCapVendorSpec
{
    CompatCapHdr   hdr;          // 0x0
    uint8_t        len;          // 0x2
    //uint8_t      data[];       // variable length
} __attribute__((packed));
static_assert(sizeof(CompatCapVendorSpec) == 0x3);

// MSI capability layouts
struct RegMSIMsgCtrl
{
    uint16_t msi_ena                 : 1;
    uint16_t multi_msg_capable       : 3;
    uint16_t multi_msg_ena           : 3;
    uint16_t addr_64_bit_capable     : 1;
    uint16_t per_vector_mask_capable : 1;
    uint16_t ext_msg_data_capable    : 1;
    uint16_t ext_msg_data_ena        : 1;
    uint16_t rsvd                    : 5;
} __attribute__((packed));
static_assert(sizeof(RegMSIMsgCtrl) == 0x2);

struct MSI32CompatCap
{
    CompatCapHdr           hdr; // 0x0
    RegMSIMsgCtrl           mc; // 0x2
    uint32_t          msg_addr; // 0x4
    uint16_t          msg_data; // 0x8
    uint16_t      ext_msg_data; // 0xa
} __attribute__((packed));
static_assert(sizeof(MSI32CompatCap) == 0xc);

struct MSI32PVMCompatCap
{
    CompatCapHdr           hdr; // 0x0
    RegMSIMsgCtrl           mc; // 0x2
    uint32_t          msg_addr; // 0x4
    uint16_t          msg_data; // 0x8
    uint16_t      ext_msg_data; // 0xa
    uint32_t         mask_bits; // 0xc
    uint32_t      pending_bits; // 0x10
} __attribute__((packed));
static_assert(sizeof(MSI32PVMCompatCap) == 0x14);

struct MSI64CompatCap
{
    CompatCapHdr             hdr; // 0x0
    RegMSIMsgCtrl             mc; // 0x2
    uint32_t            msg_addr; // 0x4
    uint32_t      msg_addr_upper; // 0x8
    uint16_t            msg_data; // 0xc
    uint16_t        ext_msg_data; // 0xe
} __attribute__((packed));
static_assert(sizeof(MSI64CompatCap) == 0x10);

struct MSI64PVMCompatCap
{
    CompatCapHdr             hdr; // 0x0
    RegMSIMsgCtrl             mc; // 0x2
    uint32_t            msg_addr; // 0x4
    uint32_t      msg_addr_upper; // 0x8
    uint16_t            msg_data; // 0xc
    uint16_t        ext_msg_data; // 0xe
    uint32_t           mask_bits; // 0x10
    uint32_t        pending_bits; // 0x14
} __attribute__((packed));
static_assert(sizeof(MSI64PVMCompatCap) == 0x18);

enum class CompatCapID
{
    null_cap                   = 0x0,
    pci_pm_iface               = 0x1,
    agp                        = 0x2,
    vpd                        = 0x3,
    slot_ident                 = 0x4,
    msi                        = 0x5,
    compat_pci_hot_swap        = 0x6,
    pci_x                      = 0x7,
    hyper_transport            = 0x8,
    vendor_spec                = 0x9,
    dbg_port                   = 0xa,
    compat_pci_central_res_ctl = 0xb,
    pci_hot_plug               = 0xc,
    pci_brd_sub_vid            = 0xd,
    agp_x8                     = 0xe,
    secure_dev                 = 0xf,
    pci_express                = 0x10,
    msix                       = 0x11,
    sata_data_idx_conf         = 0x12,
    af                         = 0x13,
    enhanced_alloc             = 0x14,
    flat_portal_brd            = 0x15
};

constexpr auto CompatCapName(const CompatCapID cap_id)
{
    switch(cap_id) {
    case CompatCapID::null_cap:
        return "<null>";
    case CompatCapID::pci_pm_iface:
        return "PCI Power Management Interface";
    case CompatCapID::agp:
        return "AGP";
    case CompatCapID::vpd:
        return "Vital Product Data";
    case CompatCapID::slot_ident:
        return "Slot Identification";
    case CompatCapID::msi:
        return "MSI";
    case CompatCapID::compat_pci_hot_swap:
        return "CompatPCI Hot Swap";
    case CompatCapID::pci_x:
        return "PCI-X";
    case CompatCapID::hyper_transport:
        return "HyperTransport";
    case CompatCapID::vendor_spec:
        return "Vendor Specific";
    case CompatCapID::dbg_port:
        return "Debug port";
    case CompatCapID::compat_pci_central_res_ctl:
        return "CompatPCI central resource control";
    case CompatCapID::pci_hot_plug:
        return "PCI Hot-Plug";
    case CompatCapID::pci_brd_sub_vid:
        return "PCI Bridge Subsystem Vendor ID";
    case CompatCapID::agp_x8:
        return "AGP 8x";
    case CompatCapID::secure_dev:
        return "Secure Device";
    case CompatCapID::pci_express:
        return "PCI Express";
    case CompatCapID::msix:
        return "MSI-X";
    case CompatCapID::sata_data_idx_conf:
        return "Serial ATA Data/Index conf";
    case CompatCapID::af:
        return "Advanced Features";
    case CompatCapID::enhanced_alloc:
        return "Enhanced Allocation";
    case CompatCapID::flat_portal_brd:
        return "Flattening Portal Bridge";
    default:
        return "";
    }
}

enum class ExtCapID
{
    null_cap                 = 0x0,
    aer                      = 0x1,
    vc_no_mfvc               = 0x2,
    dev_serial               = 0x3,
    power_budget             = 0x4,
    rc_link_decl             = 0x5,
    rc_internal_link_ctl     = 0x6,
    rc_ev_collector_ep_assoc = 0x7,
    mfvc                     = 0x8,
    vc_mfvc_pres             = 0x9,
    rcrb                     = 0xa,
    vendor_spec_ext_cap      = 0xb,
    cac                      = 0xc,
    acs                      = 0xd,
    ari                      = 0xe,
    ats                      = 0xf,
    sriov                    = 0x10,
    mriov                    = 0x11,
    mcast                    = 0x12,
    page_req_iface           = 0x13,
    amd_rsvd                 = 0x14,
    res_bar                  = 0x15,
    dpa                      = 0x16,
    tph_req                  = 0x17,
    ltr                      = 0x18,
    sec_pcie                 = 0x19,
    pmux                     = 0x1a,
    pasid                    = 0x1b,
    lnr                      = 0x1c,
    dpc                      = 0x1d,
    l1_pm_substates          = 0x1e,
    ptm                      = 0x1f,
    pcie_over_mphy           = 0x20,
    frs_q                    = 0x21,
    readiness_tr             = 0x22,
    dvsec                    = 0x23,
    vf_res_bar               = 0x24,
    data_link_feat           = 0x25,
    phys_16gt                = 0x26,
    lane_marg_rx             = 0x27,
    hierarchy_id             = 0x28,
    npem                     = 0x29,
    phys_32gt                = 0x2a,
    alter_proto              = 0x2b,
    sfi                      = 0x2c
};

constexpr auto ExtCapName(const ExtCapID cap_id)
{
    switch(cap_id) {
    case ExtCapID::null_cap:
        return "<null>";
    case ExtCapID::aer:
        return "Advanced Error Reporting (AER)";
    case ExtCapID::vc_no_mfvc:
        return "Virtual Channel (MFVC-)";
    case ExtCapID::dev_serial:
        return "Device Serial Number";
    case ExtCapID::power_budget:
        return "Power Budgeting";
    case ExtCapID::rc_link_decl:
        return "RC Link Declaration";
    case ExtCapID::rc_internal_link_ctl:
        return "RC Internal Link Control";
    case ExtCapID::rc_ev_collector_ep_assoc:
        return "RC Event Collector EP Association";
    case ExtCapID::mfvc:
        return "Multi-Function VC";
    case ExtCapID::vc_mfvc_pres:
        return "Virtual Channel (MFVC+)";
    case ExtCapID::rcrb:
        return "RC Register Block";
    case ExtCapID::vendor_spec_ext_cap:
        return "Vendor-Specific Ext Cap";
    case ExtCapID::cac:
        return "Configuration Access Correlation";
    case ExtCapID::acs:
        return "ACS";
    case ExtCapID::ari:
        return "ARI";
    case ExtCapID::ats:
        return "ATS";
    case ExtCapID::sriov:
        return "SR-IOV";
    case ExtCapID::mriov:
        return "MR-IOV";
    case ExtCapID::mcast:
        return "Multicast";
    case ExtCapID::page_req_iface:
        return "Page Request Interface";
    case ExtCapID::amd_rsvd:
        return "Reserved for AMD";
    case ExtCapID::res_bar:
        return "Resizable BAR";
    case ExtCapID::dpa:
        return "DPA";
    case ExtCapID::tph_req:
        return "TPH Requester";
    case ExtCapID::ltr:
        return "LTR";
    case ExtCapID::sec_pcie:
        return "Secondary PCIe";
    case ExtCapID::pmux:
        return "PMUX";
    case ExtCapID::pasid:
        return "PASID";
    case ExtCapID::lnr:
        return "LNR";
    case ExtCapID::dpc:
        return "DPC";
    case ExtCapID::l1_pm_substates:
        return "L1 PM Substates";
    case ExtCapID::ptm:
        return "PTM";
    case ExtCapID::pcie_over_mphy:
        return "PCIe over M-PHY";
    case ExtCapID::frs_q:
        return "FRS Queueing";
    case ExtCapID::readiness_tr:
        return "Readiness Time Reporting";
    case ExtCapID::dvsec:
        return "DVSEC";
    case ExtCapID::vf_res_bar:
        return "VF Resizable BAR";
    case ExtCapID::data_link_feat:
        return "Data Link Feature";
    case ExtCapID::phys_16gt:
        return "Phys Layer 16 GT/s";
    case ExtCapID::lane_marg_rx:
        return "Lane Margining at Receiver";
    case ExtCapID::hierarchy_id:
        return "Hierarchy ID";
    case ExtCapID::npem:
        return "NPEM";
    case ExtCapID::phys_32gt:
        return "Phys Layer 32 GT/s";
    case ExtCapID::alter_proto:
        return "Alternate Protocol";
    case ExtCapID::sfi:
        return "SFI";
    default:
        return "";
    }
}

struct RegLinkCtl3
{
    uint32_t perform_eq               : 1;
    uint32_t link_eq_req_itr_ena      : 1;
    uint32_t rsvd0                    : 7;
    uint32_t lower_skp_os_gen_vec_ena : 7;
    uint32_t rsvd1                    : 16;
} __attribute__((packed));
static_assert(sizeof(RegLinkCtl3) == 4);

constexpr auto EnableLowerSKPOSGenVecDesc(const uint8_t val)
{
    switch (val) {
    case 0b0001:
        return "2.5 GT/s";
    case 0b0010:
        return "5.0 GT/s";
    case 0b0100:
        return "8.0 GT/s";
    case 0b1000:
        return "16.0 GT/s";
    default:
        return "< rsvd >";
    }
}

struct RegLaneErrStatus
{
    uint32_t lane_err_status;
} __attribute__((packed));
static_assert(sizeof(RegLaneErrStatus) == 4);

struct RegLaneEqCtl
{
    uint16_t ds_port_8gts_trans_pres  : 4;
    uint16_t ds_port_8gts_recv_pres_h : 3;
    uint16_t rsvd0                    : 1;
    uint16_t us_port_8gts_trans_pres  : 4;
    uint16_t us_port_8gts_recv_pres_h : 3;
    uint16_t rsvd1                    : 1;
} __attribute__((packed));
static_assert(sizeof(RegLaneEqCtl) == 2);

constexpr auto TransPresHint8gtsDesc(const uint8_t val)
{
    switch (val) {
    case 0b0000:
        return "P0";
    case 0b0001:
        return "P1";
    case 0b0010:
        return "P2";
    case 0b0011:
        return "P3";
    case 0b0100:
        return "P4";
    case 0b0101:
        return "P5";
    case 0b0110:
        return "P6";
    case 0b0111:
        return "P7";
    case 0b1000:
        return "P8";
    case 0b1001:
        return "P9";
    case 0b1010:
        return "P10";
    default:
        return "< rsvd >";
    }
}

constexpr auto RecvPresHint8gtsDesc(const uint8_t val)
{
    switch (val) {
    case 0b000:
        return "-6 dB";
    case 0b001:
        return "-7 dB";
    case 0b010:
        return "-8 dB";
    case 0b011:
        return "-9 dB";
    case 0b100:
        return "-10 dB";
    case 0b101:
        return "-11 dB";
    case 0b110:
        return "-12 dB";
    default:
        return "< rsvd >";
    }
}

// Secondary PCIe ext capability strucuture
// XXX: this structure might also contain a number
// of @RegLaneEqCtl register, but the actual number
// is unknown in advance
struct SecPciECap
{
    ExtCapHdr        hdr;           // 0x0
    RegLinkCtl3      link_ctl3;     // 0x4
    RegLaneErrStatus lane_err_stat; // 0x8
} __attribute__((packed));
static_assert(sizeof(SecPciECap) == 0xc);


struct RegDataLinkFeatCap
{
    uint32_t local_data_link_feat_supp : 23;
    uint32_t rsvd0                     : 8;
    uint32_t data_link_feat_xchg_ena   : 1;
} __attribute__((packed));
static_assert(sizeof(RegDataLinkFeatCap) == 4);

struct RegDataLinkFeatStatus
{
    uint32_t rem_data_link_feat_supp       : 23;
    uint32_t rsvd0                         : 8;
    uint32_t rem_data_link_feat_supp_valid : 1;
} __attribute__((packed));
static_assert(sizeof(RegDataLinkFeatStatus) == 4);

struct DataLinkFeatureCap
{
    ExtCapHdr             hdr;             // 0x0
    RegDataLinkFeatCap    dlink_feat_cap;  // 0x4
    RegDataLinkFeatStatus dlink_feat_stat; // 0x8
} __attribute__((packed));
static_assert(sizeof(DataLinkFeatureCap) == 0xc);

struct RegARICapability
{
    uint16_t mfvc_func_grp_cap : 1;
    uint16_t acs_func_grp_cap  : 1;
    uint16_t rsvd0             : 6;
    uint16_t next_func_num     : 8;
} __attribute__((packed));
static_assert(sizeof(RegARICapability) == 2);

struct RegARIControl
{
    uint16_t mfvc_func_grps_ena : 1;
    uint16_t acs_func_grps_ena  : 1;
    uint16_t rsvd0              : 2;
    uint16_t func_grp           : 3;
    uint16_t rsvd1              : 9;
} __attribute__((packed));
static_assert(sizeof(RegARIControl) == 2);

struct ARICap
{
    ExtCapHdr        hdr;     // 0x0
    RegARICapability ari_cap; // 0x4
    RegARIControl    ari_ctl; // 0x6
} __attribute__((packed));
static_assert(sizeof(ARICap) == 0x8);

struct RegPASIDCapability
{
    uint16_t rsvd0                : 1;
    uint16_t exec_perm_supp       : 1;
    uint16_t privileged_mode_supp : 1;
    uint16_t rsvd1                : 5;
    uint16_t max_pasid_width      : 5;
    uint16_t rsvd2                : 3;
} __attribute__((packed));
static_assert(sizeof(RegPASIDCapability) == 2);

struct RegPASIDControl
{
    uint16_t pasid_ena           : 1;
    uint16_t exec_perm_ena       : 1;
    uint16_t privileged_mode_ena : 1;
    uint16_t rsvd0               : 13;
} __attribute__((packed));
static_assert(sizeof(RegPASIDControl) == 2);

struct PASIDCap
{
    ExtCapHdr          hdr;       // 0x0
    RegPASIDCapability pasid_cap; // 0x4
    RegPASIDControl    pasid_ctl; // 0x6
} __attribute__((packed));
static_assert(sizeof(PASIDCap) == 0x8);

struct RegAERUncorrStatus
{
    uint32_t                               rsvd : 4;
    uint32_t              dlink_prot_err_status : 1;
    uint32_t               supr_down_err_status : 1;
    uint32_t                              rsvd1 : 6;
    uint32_t           poisoned_tlp_recv_status : 1;
    uint32_t          flow_ctl_proto_err_status : 1;
    uint32_t                    comp_tmo_status : 1;
    uint32_t                 compl_abort_status : 1;
    uint32_t                  unexp_comp_status : 1;
    uint32_t               recv_overflow_status : 1;
    uint32_t               malformed_tlp_status : 1;
    uint32_t                    ecrc_err_status : 1;
    uint32_t              unsupp_req_err_status : 1;
    uint32_t               acs_violation_status : 1;
    uint32_t         uncorr_internal_err_status : 1;
    uint32_t              mc_blocked_tlp_status : 1;
    uint32_t    atomic_op_egress_blocked_status : 1;
    uint32_t        tlp_pref_blocked_err_status : 1;
    uint32_t poisoned_tlp_egress_blocked_status : 1;
    uint32_t                              rsvd2 : 5;
} __attribute__((packed));
static_assert(sizeof(RegAERUncorrStatus) == 4);

struct RegAERUncorrMask
{
    uint32_t                             rsvd : 4;
    uint32_t              dlink_prot_err_mask : 1;
    uint32_t               supr_down_err_mask : 1;
    uint32_t                            rsvd1 : 6;
    uint32_t           poisoned_tlp_recv_mask : 1;
    uint32_t          flow_ctl_proto_err_mask : 1;
    uint32_t                    comp_tmo_mask : 1;
    uint32_t                 compl_abort_mask : 1;
    uint32_t                  unexp_comp_mask : 1;
    uint32_t               recv_overflow_mask : 1;
    uint32_t               malformed_tlp_mask : 1;
    uint32_t                    ecrc_err_mask : 1;
    uint32_t              unsupp_req_err_mask : 1;
    uint32_t               acs_violation_mask : 1;
    uint32_t         uncorr_internal_err_mask : 1;
    uint32_t              mc_blocked_tlp_mask : 1;
    uint32_t    atomic_op_egress_blocked_mask : 1;
    uint32_t        tlp_pref_blocked_err_mask : 1;
    uint32_t poisoned_tlp_egress_blocked_mask : 1;
    uint32_t                            rsvd2 : 5;
} __attribute__((packed));
static_assert(sizeof(RegAERUncorrMask) == 4);

struct RegAERUncorrSeverity
{
    uint32_t                            rsvd : 4;
    uint32_t              dlink_prot_err_sev : 1;
    uint32_t               supr_down_err_sev : 1;
    uint32_t                           rsvd1 : 6;
    uint32_t           poisoned_tlp_recv_sev : 1;
    uint32_t          flow_ctl_proto_err_sev : 1;
    uint32_t                    comp_tmo_sev : 1;
    uint32_t                 compl_abort_sev : 1;
    uint32_t                  unexp_comp_sev : 1;
    uint32_t               recv_overflow_sev : 1;
    uint32_t               malformed_tlp_sev : 1;
    uint32_t                    ecrc_err_sev : 1;
    uint32_t              unsupp_req_err_sev : 1;
    uint32_t               acs_violation_sev : 1;
    uint32_t         uncorr_internal_err_sev : 1;
    uint32_t              mc_blocked_tlp_sev : 1;
    uint32_t    atomic_op_egress_blocked_sev : 1;
    uint32_t        tlp_pref_blocked_err_sev : 1;
    uint32_t poisoned_tlp_egress_blocked_sev : 1;
    uint32_t                           rsvd2 : 5;
} __attribute__((packed));
static_assert(sizeof(RegAERUncorrSeverity) == 4);

struct RegAERCorrStatus
{
    uint16_t recv_err_status : 1;
    uint16_t rsvd : 5;
    uint16_t bad_tlp_status : 1;
    uint16_t bad_dllp_status : 1;
    uint16_t repl_num_rollover_status : 1;
    uint16_t rsvd1 : 3;
    uint16_t repl_tmr_tmo_status : 1;
    uint16_t adv_non_fatal_err_status : 1;
    uint16_t corr_int_err_status : 1;
    uint16_t hdr_log_overflow_status : 1;
    uint16_t rsvd2;
} __attribute__((packed));
static_assert(sizeof(RegAERCorrStatus) == 4);

struct RegAERCorrMask
{
    uint16_t recv_err_mask : 1;
    uint16_t rsvd : 5;
    uint16_t bad_tlp_mask : 1;
    uint16_t bad_dllp_mask : 1;
    uint16_t repl_num_rollover_mask : 1;
    uint16_t rsvd1 : 3;
    uint16_t repl_tmr_tmo_mask : 1;
    uint16_t adv_non_fatal_err_mask : 1;
    uint16_t corr_int_err_mask : 1;
    uint16_t hdr_log_overflow_mask : 1;
    uint16_t rsvd2;
} __attribute__((packed));
static_assert(sizeof(RegAERCorrMask) == 4);

struct RegAERAecCtl
{
    uint32_t             first_err_ptr : 5;
    uint32_t              ecrc_gen_cap : 1;
    uint32_t              ecrc_gen_ena : 1;
    uint32_t            ecrc_check_cap : 1;
    uint32_t            ecrc_check_ena : 1;
    uint32_t         multi_hdr_rec_cap : 1;
    uint32_t         multi_hdr_rec_ena : 1;
    uint32_t      tlp_pref_log_present : 1;
    uint32_t comp_tmo_pref_hdr_log_cap : 1;
    uint32_t                      rsvd : 19;
} __attribute__((packed));
static_assert(sizeof(RegAERAecCtl) == 4);

struct RegAERHdrLog
{
    uint32_t dw[4];
} __attribute__((packed));
static_assert(sizeof(RegAERHdrLog) == 0x10);

struct RegAERRootErrCmd
{
    uint32_t     fatal_err_rep_ena : 1;
    uint32_t non_fatal_err_rep_ena : 1;
    uint32_t      corr_err_rep_ena : 1;
    uint32_t                  rsvd : 29;
} __attribute__((packed));
static_assert(sizeof(RegAERRootErrCmd) == 0x4);

struct RegAERRootErrStatus
{
    uint32_t                  err_cor_recv : 1;
    uint32_t            multi_err_cor_recv : 1;
    uint32_t       err_fatal_nonfatal_recv : 1;
    uint32_t multi_err_fatal_nonfatal_recv : 1;
    uint32_t            first_uncorr_fatal : 1;
    uint32_t         nonfatal_err_msg_recv : 1;
    uint32_t            fatal_err_msg_recv : 1;
    uint32_t                          rsvd : 20;
    uint32_t           adv_err_int_msg_num : 5;
} __attribute__((packed));
static_assert(sizeof(RegAERRootErrStatus) == 0x4);

struct RegAERCorrErrSrcID
{
    uint16_t req_id;
} __attribute__((packed));
static_assert(sizeof(RegAERCorrErrSrcID) == 0x2);

struct RegAERErrSrcID
{
    uint16_t req_id;
} __attribute__((packed));
static_assert(sizeof(RegAERErrSrcID) == 0x2);

struct RegAERTlpPrefixLog
{
    uint32_t dw[4];
} __attribute__((packed));
static_assert(sizeof(RegAERTlpPrefixLog) == 0x10);

struct AERCap
{
    ExtCapHdr                        hdr; // 0x0
    RegAERUncorrStatus uncorr_err_status; // 0x4
    RegAERUncorrMask     uncorr_err_mask; // 0x8
    RegAERUncorrSeverity  uncorr_err_sev; // 0xc
    RegAERCorrStatus     corr_err_status; // 0x10
    RegAERCorrMask         corr_err_mask; // 0x14
    RegAERAecCtl         adv_err_cap_ctl; // 0x18
    RegAERHdrLog                 hdr_log; // 0x1c
    RegAERRootErrCmd        root_err_cmd; // 0x2c
    RegAERRootErrStatus  root_err_status; // 0x30
    RegAERCorrErrSrcID   corr_err_src_id; // 0x34
    RegAERErrSrcID            err_src_id; // 0x36
    RegAERTlpPrefixLog      tlp_pref_log; // 0x38
} __attribute__((packed));
static_assert(sizeof(AERCap) == 0x48);

struct RegVPDAddr
{
    uint16_t addr : 15;
    uint16_t    f : 1;
} __attribute__((packed));
static_assert(sizeof(RegVPDAddr) == 2);

struct RegVPDData
{
    uint32_t data;
} __attribute__((packed));
static_assert(sizeof(RegVPDData) == 4);

struct VPDCap
{
    CompatCapHdr hdr;   // 0x0
    RegVPDAddr  addr;   // 0x2
    RegVPDData  data;   // 0x4
} __attribute__((packed));
static_assert(sizeof(VPDCap) == 0x8);

// Only permitted in Type 1 (bridge) Configuration Space Headers
struct RegSubsysIDHdrRsvd
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegSubsysIDHdrRsvd) == 2);

// Reuses RegSubsysVID/RegSubsysID (same layout as the Type0Cfg fields).
struct SubsystemIDCap
{
    CompatCapHdr        hdr;   // 0x0
    RegSubsysIDHdrRsvd  rsvd;  // 0x2
    RegSubsysVID       ssvid;  // 0x4
    RegSubsysID          ssid; // 0x6
} __attribute__((packed));
static_assert(sizeof(SubsystemIDCap) == 0x8);

struct RegAFCap
{
    uint8_t  tp_cap : 1;
    uint8_t flr_cap : 1;
    uint8_t    rsvd : 6;
} __attribute__((packed));
static_assert(sizeof(RegAFCap) == 1);

struct RegAFCtl
{
    uint8_t flr_init : 1;
    uint8_t     rsvd : 7;
} __attribute__((packed));
static_assert(sizeof(RegAFCtl) == 1);

struct RegAFStatus
{
    uint8_t   tp : 1;
    uint8_t rsvd : 7;
} __attribute__((packed));
static_assert(sizeof(RegAFStatus) == 1);

struct RegAFLength
{
    uint8_t length;  // must be 06h
} __attribute__((packed));
static_assert(sizeof(RegAFLength) == 1);

struct CompatCapAF
{
    CompatCapHdr   hdr;     // 0x0
    RegAFLength length;     // 0x2
    RegAFCap       af_cap;  // 0x3
    RegAFCtl       af_ctl;  // 0x4
    RegAFStatus af_status;  // 0x5
} __attribute__((packed));
static_assert(sizeof(CompatCapAF) == 0x6);

struct RegPwrBudgetDataSelect
{
    uint32_t data_sel : 8;
    uint32_t     rsvd : 24;
} __attribute__((packed));
static_assert(sizeof(RegPwrBudgetDataSelect) == 4);

// Indirect: DWORD selected by RegPwrBudgetDataSelect, decoded by RegPwrBudgetDataValue.
struct RegPwrBudgetData
{
    uint32_t data;
} __attribute__((packed));
static_assert(sizeof(RegPwrBudgetData) == 4);

struct RegPwrBudgetDataValue
{
    uint32_t   base_power : 8;
    uint32_t   data_scale : 2;
    uint32_t pm_sub_state : 3;
    uint32_t     pm_state : 2;
    uint32_t         type : 3;
    uint32_t   power_rail : 3;
    uint32_t         rsvd : 11;
} __attribute__((packed));
static_assert(sizeof(RegPwrBudgetDataValue) == 4);

struct RegPwrBudgetCap
{
    uint32_t sys_allocated : 1;
    uint32_t          rsvd : 31;
} __attribute__((packed));
static_assert(sizeof(RegPwrBudgetCap) == 4);

struct PowerBudgetCap
{
    ExtCapHdr                   hdr;      // 0x0
    RegPwrBudgetDataSelect data_select;    // 0x4
    RegPwrBudgetData              data;    // 0x8
    RegPwrBudgetCap            pb_cap;     // 0xc
} __attribute__((packed));
static_assert(sizeof(PowerBudgetCap) == 0x10);

struct RegLTRMaxSnoopLat
{
    uint16_t value : 10;
    uint16_t scale : 3;
    uint16_t  rsvd : 3;
} __attribute__((packed));
static_assert(sizeof(RegLTRMaxSnoopLat) == 2);

struct RegLTRMaxNoSnoopLat
{
    uint16_t value : 10;
    uint16_t scale : 3;
    uint16_t  rsvd : 3;
} __attribute__((packed));
static_assert(sizeof(RegLTRMaxNoSnoopLat) == 2);

struct LTRCap
{
    ExtCapHdr               hdr;         // 0x0
    RegLTRMaxSnoopLat       max_snoop;   // 0x4
    RegLTRMaxNoSnoopLat  max_no_snoop;   // 0x6
} __attribute__((packed));
static_assert(sizeof(LTRCap) == 0x8);

struct RegRCLinkCap
{
    uint32_t       max_link_speed : 4;
    uint32_t       max_link_width : 6;
    uint32_t         aspm_support : 2;
    uint32_t         l0s_exit_lat : 3;
    uint32_t          l1_exit_lat : 3;
    uint32_t supp_link_speeds_vec : 7;
    uint32_t                 rsvd : 7;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkCap) == 4);

struct RegRCLinkCtl
{
    uint16_t  aspm_ctl : 2;
    uint16_t     rsvd0 : 5;
    uint16_t ext_synch : 1;
    uint16_t     rsvd1 : 8;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkCtl) == 2);

struct RegRCLinkStatus
{
    uint16_t       curr_link_speed : 4;
    uint16_t negotiated_link_width : 6;
    uint16_t                  rsvd : 6;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkStatus) == 2);

struct RCInternalLinkCtlCap
{
    ExtCapHdr           hdr;         // 0x0
    RegRCLinkCap        link_cap;    // 0x4
    RegRCLinkCtl        link_ctl;    // 0x8
    RegRCLinkStatus     link_status; // 0xa
} __attribute__((packed));
static_assert(sizeof(RCInternalLinkCtlCap) == 0xc);

struct RegRCECAssocBitmap
{
    uint32_t bitmap;
} __attribute__((packed));
static_assert(sizeof(RegRCECAssocBitmap) == 4);

// Present only when the header's cap_ver field is 2h or greater
struct RegRCECAssocBusNumbers
{
    uint32_t          rsvd : 8;
    uint32_t rcec_next_bus : 8;
    uint32_t rcec_last_bus : 8;
    uint32_t         rsvd1 : 8;
} __attribute__((packed));
static_assert(sizeof(RegRCECAssocBusNumbers) == 4);

struct RCEventCollectorEPAssocCap
{
    ExtCapHdr                   hdr;        // 0x0
    RegRCECAssocBitmap    assoc_bitmap;      // 0x4
    RegRCECAssocBusNumbers assoc_bus_nums;   // 0x8
} __attribute__((packed));
static_assert(sizeof(RCEventCollectorEPAssocCap) == 0xc);

struct RegLNRCap
{
    uint16_t  lnr_64_supported : 1;
    uint16_t lnr_128_supported : 1;
    uint16_t             rsvd0 : 6;
    uint16_t       lnr_reg_max : 5;
    uint16_t             rsvd1 : 3;
} __attribute__((packed));
static_assert(sizeof(RegLNRCap) == 2);

struct RegLNRCtl
{
    uint16_t       lnr_ena : 1;
    uint16_t       lnr_cls : 1;
    uint16_t         rsvd0 : 6;
    uint16_t lnr_reg_limit : 5;
    uint16_t         rsvd1 : 3;
} __attribute__((packed));
static_assert(sizeof(RegLNRCtl) == 2);

struct LNRCap
{
    ExtCapHdr  hdr;      // 0x0
    RegLNRCap  lnr_cap;  // 0x4
    RegLNRCtl  lnr_ctl;  // 0x6
} __attribute__((packed));
static_assert(sizeof(LNRCap) == 0x8);

struct RegPTMCap
{
    uint32_t       ptm_req_capable : 1;
    uint32_t      ptm_resp_capable : 1;
    uint32_t      ptm_root_capable : 1;
    uint32_t          eptm_capable : 1;
    uint32_t                 rsvd0 : 4;
    uint32_t local_clk_granularity : 8;
    uint32_t                 rsvd1 : 16;
} __attribute__((packed));
static_assert(sizeof(RegPTMCap) == 4);

struct RegPTMCtl
{
    uint32_t               ptm_ena : 1;
    uint32_t           root_select : 1;
    uint32_t                 rsvd0 : 6;
    uint32_t effective_granularity : 8;
    uint32_t                 rsvd1 : 16;
} __attribute__((packed));
static_assert(sizeof(RegPTMCtl) == 4);

struct PTMCap
{
    ExtCapHdr  hdr;      // 0x0
    RegPTMCap  ptm_cap;  // 0x4
    RegPTMCtl  ptm_ctl;  // 0x8
} __attribute__((packed));
static_assert(sizeof(PTMCap) == 0xc);

// Each 12-bit time field is Value[8:0] scaled by Multiplier[Scale] = 32^Scale (Scale in bits [11:9])
struct RegReadinessTimeReport1
{
    uint32_t reset_time : 12;
    uint32_t dl_up_time : 12;
    uint32_t       rsvd : 7;
    uint32_t      valid : 1;
} __attribute__((packed));
static_assert(sizeof(RegReadinessTimeReport1) == 4);

struct RegReadinessTimeReport2
{
    uint32_t         flr_time : 12;
    uint32_t d3hot_to_d0_time : 12;
    uint32_t             rsvd : 8;
} __attribute__((packed));
static_assert(sizeof(RegReadinessTimeReport2) == 4);

struct ReadinessTimeReportCap
{
    ExtCapHdr                   hdr;    // 0x0
    RegReadinessTimeReport1  rtr1;       // 0x4
    RegReadinessTimeReport2  rtr2;       // 0x8
} __attribute__((packed));
static_assert(sizeof(ReadinessTimeReportCap) == 0xc);

struct RegHierarchyIDStatus
{
    uint32_t msg_requester_id : 16;
    uint32_t             rsvd : 12;
    uint32_t        writeable : 1;
    uint32_t  vf_configurable : 1;
    uint32_t          pending : 1;
    uint32_t            valid : 1;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDStatus) == 4);

struct RegHierarchyIDData
{
    uint32_t sys_guid_authority_id : 8;
    uint32_t                  rsvd : 8;
    uint32_t          hierarchy_id : 16;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDData) == 4);

struct RegHierarchyIDGUID1
{
    uint32_t sys_guid_1 : 16;
    uint32_t       rsvd : 16;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDGUID1) == 4);

struct RegHierarchyIDGUID2
{
    uint32_t sys_guid_2;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDGUID2) == 4);

struct RegHierarchyIDGUID3
{
    uint32_t sys_guid_3;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDGUID3) == 4);

struct RegHierarchyIDGUID4
{
    uint32_t sys_guid_4;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDGUID4) == 4);

struct RegHierarchyIDGUID5
{
    uint32_t sys_guid_5;
} __attribute__((packed));
static_assert(sizeof(RegHierarchyIDGUID5) == 4);

struct HierarchyIDCap
{
    ExtCapHdr             hdr;    // 0x0
    RegHierarchyIDStatus  status; // 0x4
    RegHierarchyIDData    data;   // 0x8
    RegHierarchyIDGUID1   guid1;  // 0xc
    RegHierarchyIDGUID2   guid2;  // 0x10
    RegHierarchyIDGUID3   guid3;  // 0x14
    RegHierarchyIDGUID4   guid4;  // 0x18
    RegHierarchyIDGUID5   guid5;  // 0x1c
} __attribute__((packed));
static_assert(sizeof(HierarchyIDCap) == 0x20);

struct RegNPEMCap
{
    uint32_t                  npem_capable : 1;
    uint32_t            npem_reset_capable : 1;
    uint32_t               npem_ok_capable : 1;
    uint32_t           npem_locate_capable : 1;
    uint32_t             npem_fail_capable : 1;
    uint32_t          npem_rebuild_capable : 1;
    uint32_t              npem_pfa_capable : 1;
    uint32_t        npem_hot_spare_capable : 1;
    uint32_t  npem_in_critical_arr_capable : 1;
    uint32_t    npem_in_failed_arr_capable : 1;
    uint32_t npem_invalid_dev_type_capable : 1;
    uint32_t         npem_disabled_capable : 1;
    uint32_t                          rsvd : 12;
    uint32_t        enclosure_specific_cap : 8;
} __attribute__((packed));
static_assert(sizeof(RegNPEMCap) == 4);

struct RegNPEMCtl
{
    uint32_t                  npem_ena : 1;
    uint32_t       npem_initiate_reset : 1;
    uint32_t               npem_ok_ctl : 1;
    uint32_t           npem_locate_ctl : 1;
    uint32_t             npem_fail_ctl : 1;
    uint32_t          npem_rebuild_ctl : 1;
    uint32_t              npem_pfa_ctl : 1;
    uint32_t        npem_hot_spare_ctl : 1;
    uint32_t  npem_in_critical_arr_ctl : 1;
    uint32_t    npem_in_failed_arr_ctl : 1;
    uint32_t npem_invalid_dev_type_ctl : 1;
    uint32_t         npem_disabled_ctl : 1;
    uint32_t                      rsvd : 12;
    uint32_t    enclosure_specific_ctl : 8;
} __attribute__((packed));
static_assert(sizeof(RegNPEMCtl) == 4);

struct RegNPEMStatus
{
    uint32_t        npem_cmd_completed : 1;
    uint32_t                      rsvd : 23;
    uint32_t enclosure_specific_status : 8;
} __attribute__((packed));
static_assert(sizeof(RegNPEMStatus) == 4);

struct NPEMCap
{
    ExtCapHdr       hdr;     // 0x0
    RegNPEMCap      npem_cap;    // 0x4
    RegNPEMCtl      npem_ctl;    // 0x8
    RegNPEMStatus   npem_status; // 0xc
} __attribute__((packed));
static_assert(sizeof(NPEMCap) == 0x10);

struct RegRCRBVenDevID
{
    uint32_t vendor_id : 16;
    uint32_t device_id : 16;
} __attribute__((packed));
static_assert(sizeof(RegRCRBVenDevID) == 4);

struct RegRCRBCap
{
    uint32_t crs_sw_vis : 1;
    uint32_t       rsvd : 31;
} __attribute__((packed));
static_assert(sizeof(RegRCRBCap) == 4);

struct RegRCRBCtl
{
    uint32_t crs_sw_vis_ena : 1;
    uint32_t           rsvd : 31;
} __attribute__((packed));
static_assert(sizeof(RegRCRBCtl) == 4);

struct RegRCRBHdrRsvd
{
    uint32_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegRCRBHdrRsvd) == 4);

struct RCRBHeaderCap
{
    ExtCapHdr        hdr;         // 0x0
    RegRCRBVenDevID  ven_dev_id;  // 0x4
    RegRCRBCap       rcrb_cap;    // 0x8
    RegRCRBCtl       rcrb_ctl;    // 0xc
    RegRCRBHdrRsvd   rsvd;        // 0x10
} __attribute__((packed));
static_assert(sizeof(RCRBHeaderCap) == 0x14);

struct RegVSECHeader
{
    uint32_t     vsec_id : 16;
    uint32_t    vsec_rev : 4;
    uint32_t vsec_length : 12;
} __attribute__((packed));
static_assert(sizeof(RegVSECHeader) == 4);

// Fixed head only; vendor-specific area (size = vsec_length-8) not represented.
struct VendorSpecExtCap
{
    ExtCapHdr      hdr;      // 0x0
    RegVSECHeader  vsec_hdr; // 0x4
} __attribute__((packed));
static_assert(sizeof(VendorSpecExtCap) == 0x8);

struct RegACSCap
{
    uint16_t             src_val : 1;
    uint16_t         trans_block : 1;
    uint16_t    p2p_req_redirect : 1;
    uint16_t  p2p_compl_redirect : 1;
    uint16_t        upstream_fwd : 1;
    uint16_t      p2p_egress_ctl : 1;
    uint16_t    direct_trans_p2p : 1;
    uint16_t        enhanced_cap : 1;
    uint16_t egress_ctl_vec_size : 8;
} __attribute__((packed));
static_assert(sizeof(RegACSCap) == 2);

struct RegACSCtl
{
    uint16_t                src_val_ena : 1;
    uint16_t            trans_block_ena : 1;
    uint16_t       p2p_req_redirect_ena : 1;
    uint16_t     p2p_compl_redirect_ena : 1;
    uint16_t           upstream_fwd_ena : 1;
    uint16_t         p2p_egress_ctl_ena : 1;
    uint16_t       direct_trans_p2p_ena : 1;
    uint16_t           io_req_block_ena : 1;
    uint16_t     dsp_mem_tgt_access_ctl : 2;
    uint16_t     usp_mem_tgt_access_ctl : 2;
    uint16_t unclaimed_req_redirect_ctl : 1;
    uint16_t                       rsvd : 3;
} __attribute__((packed));
static_assert(sizeof(RegACSCtl) == 2);

// Fixed head only; Egress Control Vector (size = egress_ctl_vec_size) not represented.
struct ACSCap
{
    ExtCapHdr  hdr;      // 0x0
    RegACSCap  acs_cap;  // 0x4
    RegACSCtl  acs_ctl;  // 0x6
} __attribute__((packed));
static_assert(sizeof(ACSCap) == 0x8);

struct RegL1PMSubstatesCap
{
    uint32_t         pci_pm_l1_2_supported : 1;
    uint32_t         pci_pm_l1_1_supported : 1;
    uint32_t           aspm_l1_2_supported : 1;
    uint32_t           aspm_l1_1_supported : 1;
    uint32_t     l1_pm_substates_supported : 1;
    uint32_t     link_activation_supported : 1;
    uint32_t                         rsvd0 : 2;
    uint32_t port_common_mode_restore_time : 8;
    uint32_t         port_t_power_on_scale : 2;
    uint32_t                         rsvd1 : 1;
    uint32_t         port_t_power_on_value : 5;
    uint32_t                         rsvd2 : 8;
} __attribute__((packed));
static_assert(sizeof(RegL1PMSubstatesCap) == 4);

struct RegL1PMSubstatesCtl1
{
    uint32_t          pci_pm_l1_2_ena : 1;
    uint32_t          pci_pm_l1_1_ena : 1;
    uint32_t            aspm_l1_2_ena : 1;
    uint32_t            aspm_l1_1_ena : 1;
    uint32_t  link_activation_itr_ena : 1;
    uint32_t      link_activation_ctl : 1;
    uint32_t                    rsvd0 : 2;
    uint32_t common_mode_restore_time : 8;
    uint32_t ltr_l1_2_threshold_value : 10;
    uint32_t                    rsvd1 : 3;
    uint32_t ltr_l1_2_threshold_scale : 3;
} __attribute__((packed));
static_assert(sizeof(RegL1PMSubstatesCtl1) == 4);

struct RegL1PMSubstatesCtl2
{
    uint32_t t_power_on_scale : 2;
    uint32_t            rsvd0 : 1;
    uint32_t t_power_on_value : 5;
    uint32_t            rsvd1 : 24;
} __attribute__((packed));
static_assert(sizeof(RegL1PMSubstatesCtl2) == 4);

// Present only when cap_ver >= 2h; RsvdZ otherwise.
struct RegL1PMSubstatesStatus
{
    uint32_t link_activation_status : 1;
    uint32_t                   rsvd : 31;
} __attribute__((packed));
static_assert(sizeof(RegL1PMSubstatesStatus) == 4);

struct L1PMSubstatesCap
{
    ExtCapHdr                hdr;      // 0x0
    RegL1PMSubstatesCap      cap;       // 0x4
    RegL1PMSubstatesCtl1     ctl1;      // 0x8
    RegL1PMSubstatesCtl2     ctl2;      // 0xc
    RegL1PMSubstatesStatus   status;    // 0x10
} __attribute__((packed));
static_assert(sizeof(L1PMSubstatesCap) == 0x14);

struct RegDPCCap
{
    uint16_t                   dpc_itr_msg_num : 5;
    uint16_t                    rp_ext_for_dpc : 1;
    uint16_t poisoned_tlp_egress_blk_supported : 1;
    uint16_t       dpc_sw_triggering_supported : 1;
    uint16_t                   rp_pio_log_size : 4;
    uint16_t   dl_active_err_cor_sig_supported : 1;
    uint16_t                              rsvd : 3;
} __attribute__((packed));
static_assert(sizeof(RegDPCCap) == 2);

struct RegDPCCtl
{
    uint16_t             dpc_trigger_ena : 2;
    uint16_t          dpc_completion_ctl : 1;
    uint16_t                 dpc_itr_ena : 1;
    uint16_t             dpc_err_cor_ena : 1;
    uint16_t poisoned_tlp_egress_blk_ena : 1;
    uint16_t              dpc_sw_trigger : 1;
    uint16_t       dl_active_err_cor_ena : 1;
    uint16_t             dpc_sig_sfw_ena : 1;
    uint16_t                        rsvd : 7;
} __attribute__((packed));
static_assert(sizeof(RegDPCCtl) == 2);

struct RegDPCStatus
{
    uint16_t     dpc_trigger_status : 1;
    uint16_t     dpc_trigger_reason : 2;
    uint16_t         dpc_itr_status : 1;
    uint16_t            dpc_rp_busy : 1;
    uint16_t dpc_trigger_reason_ext : 2;
    uint16_t                  rsvd0 : 1;
    uint16_t   rp_pio_first_err_ptr : 5;
    uint16_t     dpc_sig_sfw_status : 1;
    uint16_t                  rsvd1 : 2;
} __attribute__((packed));
static_assert(sizeof(RegDPCStatus) == 2);

struct RegDPCErrSourceID
{
    uint16_t req_id;
} __attribute__((packed));
static_assert(sizeof(RegDPCErrSourceID) == 2);

// Shared layout for RP PIO Status/Mask/Severity/SysError/Exception (RP Extensions for DPC only).
struct RegDPCRPPIOStatus
{
    uint32_t       cfg_ur_cpl : 1;
    uint32_t       cfg_ca_cpl : 1;
    uint32_t          cfg_cto : 1;
    uint32_t            rsvd0 : 5;
    uint32_t        io_ur_cpl : 1;
    uint32_t        io_ca_cpl : 1;
    uint32_t           io_cto : 1;
    uint32_t            rsvd1 : 5;
    uint32_t       mem_ur_cpl : 1;
    uint32_t       mem_ca_cpl : 1;
    uint32_t          mem_cto : 1;
    uint32_t            rsvd2 : 12;
    uint32_t permanently_rsvd : 1;
} __attribute__((packed));
static_assert(sizeof(RegDPCRPPIOStatus) == 4);

struct RegDPCRPPIOMask
{
    uint32_t cfg_ur_cpl : 1;
    uint32_t cfg_ca_cpl : 1;
    uint32_t    cfg_cto : 1;
    uint32_t      rsvd0 : 5;
    uint32_t  io_ur_cpl : 1;
    uint32_t  io_ca_cpl : 1;
    uint32_t     io_cto : 1;
    uint32_t      rsvd1 : 5;
    uint32_t mem_ur_cpl : 1;
    uint32_t mem_ca_cpl : 1;
    uint32_t    mem_cto : 1;
    uint32_t      rsvd2 : 13;
} __attribute__((packed));
static_assert(sizeof(RegDPCRPPIOMask) == 4);

struct RegDPCRPPIOSeverity
{
    uint32_t cfg_ur_cpl : 1;
    uint32_t cfg_ca_cpl : 1;
    uint32_t    cfg_cto : 1;
    uint32_t      rsvd0 : 5;
    uint32_t  io_ur_cpl : 1;
    uint32_t  io_ca_cpl : 1;
    uint32_t     io_cto : 1;
    uint32_t      rsvd1 : 5;
    uint32_t mem_ur_cpl : 1;
    uint32_t mem_ca_cpl : 1;
    uint32_t    mem_cto : 1;
    uint32_t      rsvd2 : 13;
} __attribute__((packed));
static_assert(sizeof(RegDPCRPPIOSeverity) == 4);

struct RegDPCRPPIOSysError
{
    uint32_t cfg_ur_cpl : 1;
    uint32_t cfg_ca_cpl : 1;
    uint32_t    cfg_cto : 1;
    uint32_t      rsvd0 : 5;
    uint32_t  io_ur_cpl : 1;
    uint32_t  io_ca_cpl : 1;
    uint32_t     io_cto : 1;
    uint32_t      rsvd1 : 5;
    uint32_t mem_ur_cpl : 1;
    uint32_t mem_ca_cpl : 1;
    uint32_t    mem_cto : 1;
    uint32_t      rsvd2 : 13;
} __attribute__((packed));
static_assert(sizeof(RegDPCRPPIOSysError) == 4);

struct RegDPCRPPIOException
{
    uint32_t cfg_ur_cpl : 1;
    uint32_t cfg_ca_cpl : 1;
    uint32_t    cfg_cto : 1;
    uint32_t      rsvd0 : 5;
    uint32_t  io_ur_cpl : 1;
    uint32_t  io_ca_cpl : 1;
    uint32_t     io_cto : 1;
    uint32_t      rsvd1 : 5;
    uint32_t mem_ur_cpl : 1;
    uint32_t mem_ca_cpl : 1;
    uint32_t    mem_cto : 1;
    uint32_t      rsvd2 : 13;
} __attribute__((packed));
static_assert(sizeof(RegDPCRPPIOException) == 4);

// Implementation-defined; allocated only when RP PIO Log Size >= 5.
struct RegDPCRPPIOImpSpecLog
{
    uint32_t log;
} __attribute__((packed));
static_assert(sizeof(RegDPCRPPIOImpSpecLog) == 4);

// RP PIO registers present only with RP Extensions for DPC; Header/TLP
// Prefix Log format matches AER but real size is variable (rp_pio_log_size).
struct DPCCap
{
    ExtCapHdr             hdr;              // 0x0
    RegDPCCap             dpc_cap;          // 0x4
    RegDPCCtl             dpc_ctl;          // 0x6
    RegDPCStatus          dpc_status;       // 0x8
    RegDPCErrSourceID     dpc_err_src_id;   // 0xa
    RegDPCRPPIOStatus     rp_pio_status;    // 0xc
    RegDPCRPPIOMask       rp_pio_mask;      // 0x10
    RegDPCRPPIOSeverity   rp_pio_severity;  // 0x14
    RegDPCRPPIOSysError   rp_pio_syserror;  // 0x18
    RegDPCRPPIOException  rp_pio_exception; // 0x1c
    RegAERHdrLog          rp_pio_hdr_log;   // 0x20
    RegDPCRPPIOImpSpecLog rp_pio_impspec_log; // 0x30
    RegAERTlpPrefixLog    rp_pio_tlp_pref_log; // 0x34
} __attribute__((packed));
static_assert(sizeof(DPCCap) == 0x44);

struct RegAltProtoCap
{
    uint32_t             alt_proto_count : 8;
    uint32_t alt_proto_sel_ena_supported : 1;
    uint32_t                        rsvd : 23;
} __attribute__((packed));
static_assert(sizeof(RegAltProtoCap) == 4);

struct RegAltProtoCtl
{
    uint32_t                alt_proto_idx_sel : 8;
    uint32_t alt_proto_negotiation_global_ena : 1;
    uint32_t                             rsvd : 23;
} __attribute__((packed));
static_assert(sizeof(RegAltProtoCtl) == 4);

struct RegAltProtoData1
{
    uint32_t usage_info : 3;
    uint32_t       rsvd : 2;
    uint32_t    details : 11;
    uint32_t  vendor_id : 16;
} __attribute__((packed));
static_assert(sizeof(RegAltProtoData1) == 4);

struct RegAltProtoData2
{
    uint32_t modified_ts_info2 : 24;
    uint32_t              rsvd : 8;
} __attribute__((packed));
static_assert(sizeof(RegAltProtoData2) == 4);

// Present only when alt_proto_sel_ena_supported is Set; may span more than
// one DWORD for alt_proto_count > 32.
struct RegAltProtoSelEnableMask
{
    uint32_t pcie_sel_ena : 1;
    uint32_t       others : 31;
} __attribute__((packed));
static_assert(sizeof(RegAltProtoSelEnableMask) == 4);

struct AlternateProtocolCap
{
    ExtCapHdr                  hdr;       // 0x0
    RegAltProtoCap             alt_cap;   // 0x4
    RegAltProtoCtl             alt_ctl;   // 0x8
    RegAltProtoData1           data1;     // 0xc
    RegAltProtoData2           data2;     // 0x10
    RegAltProtoSelEnableMask   sel_ena_mask; // 0x14
} __attribute__((packed));
static_assert(sizeof(AlternateProtocolCap) == 0x18);

struct RegSFICap
{
    uint16_t oob_pd_supported : 1;
    uint16_t             rsvd : 15;
} __attribute__((packed));
static_assert(sizeof(RegSFICap) == 2);

struct RegSFICtl
{
    uint16_t         pd_state_mask : 1;
    uint16_t        dll_state_mask : 1;
    uint16_t    oob_pd_changed_ena : 1;
    uint16_t dll_state_changed_ena : 1;
    uint16_t               dpf_ctl : 2;
    uint16_t          hps_suppress : 1;
    uint16_t              drs_mask : 1;
    uint16_t           drs_sig_ena : 1;
    uint16_t           drs_trigger : 1;
    uint16_t                  rsvd : 6;
} __attribute__((packed));
static_assert(sizeof(RegSFICtl) == 2);

struct RegSFIStatus
{
    uint16_t          pd_state : 1;
    uint16_t      oob_pd_state : 1;
    uint16_t    oob_pd_changed : 1;
    uint16_t         dll_state : 1;
    uint16_t dll_state_changed : 1;
    uint16_t      drs_received : 1;
    uint16_t              rsvd : 10;
} __attribute__((packed));
static_assert(sizeof(RegSFIStatus) == 2);

struct RegSFICAMAddr
{
    uint32_t cam_addr : 28;
    uint32_t     rsvd : 4;
} __attribute__((packed));
static_assert(sizeof(RegSFICAMAddr) == 4);

struct RegSFIHdrRsvd
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegSFIHdrRsvd) == 2);

struct RegSFICAMData
{
    uint32_t cam_data;
} __attribute__((packed));
static_assert(sizeof(RegSFICAMData) == 4);

struct SFICap
{
    ExtCapHdr       hdr;         // 0x0
    RegSFICap       sfi_cap;     // 0x4
    RegSFICtl       sfi_ctl;     // 0x6
    RegSFIStatus    sfi_status;  // 0x8
    RegSFIHdrRsvd   rsvd;        // 0xa
    RegSFICAMAddr   cam_addr;    // 0xc
    RegSFICAMData   cam_data;    // 0x10
} __attribute__((packed));
static_assert(sizeof(SFICap) == 0x14);

struct RegMulticastCap
{
    uint16_t             mc_max_group : 6;
    uint16_t                    rsvd0 : 2;
    uint16_t mc_window_size_requested : 6;
    uint16_t                    rsvd1 : 1;
    uint16_t  mc_ecrc_regen_supported : 1;
} __attribute__((packed));
static_assert(sizeof(RegMulticastCap) == 2);

struct RegMulticastCtl
{
    uint16_t mc_num_group : 6;
    uint16_t         rsvd : 9;
    uint16_t    mc_enable : 1;
} __attribute__((packed));
static_assert(sizeof(RegMulticastCtl) == 2);

struct RegMCBaseAddr
{
    uint64_t mc_index_position : 6;
    uint64_t              rsvd : 6;
    uint64_t   mc_base_address : 52;
} __attribute__((packed));
static_assert(sizeof(RegMCBaseAddr) == 8);

// Present only in Switch and Root Complex Ports; not implemented in Endpoints.
struct RegMCOverlayBAR
{
    uint64_t mc_overlay_size : 6;
    uint64_t  mc_overlay_bar : 58;
} __attribute__((packed));
static_assert(sizeof(RegMCOverlayBAR) == 8);

struct RegMCReceive
{
    uint64_t mc_receive;
} __attribute__((packed));
static_assert(sizeof(RegMCReceive) == 8);

struct RegMCBlockAll
{
    uint64_t mc_block_all;
} __attribute__((packed));
static_assert(sizeof(RegMCBlockAll) == 8);

struct RegMCBlockUntranslated
{
    uint64_t mc_block_untranslated;
} __attribute__((packed));
static_assert(sizeof(RegMCBlockUntranslated) == 8);

struct MulticastCap
{
    ExtCapHdr               hdr;                    // 0x0
    RegMulticastCap         mc_cap;                 // 0x4
    RegMulticastCtl         mc_ctl;                 // 0x6
    RegMCBaseAddr           mc_base_addr;           // 0x8
    RegMCReceive            mc_receive;             // 0x10
    RegMCBlockAll           mc_block_all;           // 0x18
    RegMCBlockUntranslated  mc_block_untranslated;  // 0x20
    RegMCOverlayBAR         mc_overlay_bar;         // 0x28
} __attribute__((packed));
static_assert(sizeof(MulticastCap) == 0x30);

struct RegPRICtl
{
    uint16_t enable : 1;
    uint16_t  reset : 1;
    uint16_t   rsvd : 14;
} __attribute__((packed));
static_assert(sizeof(RegPRICtl) == 2);

struct RegPRIStatus
{
    uint16_t        response_failure : 1;
    uint16_t      unexpected_prg_idx : 1;
    uint16_t                   rsvd0 : 6;
    uint16_t                 stopped : 1;
    uint16_t                   rsvd1 : 6;
    uint16_t prg_resp_pasid_required : 1;
} __attribute__((packed));
static_assert(sizeof(RegPRIStatus) == 2);

struct RegPRIOutstandingReqCapacity
{
    uint32_t outstanding_req_capacity;
} __attribute__((packed));
static_assert(sizeof(RegPRIOutstandingReqCapacity) == 4);

struct RegPRIOutstandingReqAllocation
{
    uint32_t outstanding_req_allocation;
} __attribute__((packed));
static_assert(sizeof(RegPRIOutstandingReqAllocation) == 4);

struct PageReqIfaceCap
{
    ExtCapHdr                        hdr;                        // 0x0
    RegPRICtl                        pri_ctl;                    // 0x4
    RegPRIStatus                     pri_status;                 // 0x6
    RegPRIOutstandingReqCapacity     outstanding_req_capacity;   // 0x8
    RegPRIOutstandingReqAllocation   outstanding_req_allocation; // 0xc
} __attribute__((packed));
static_assert(sizeof(PageReqIfaceCap) == 0x10);

struct RegFPBCap
{
    uint32_t      fpb_rid_decode_mech_supported : 1;
    uint32_t  fpb_mem_low_decode_mech_supported : 1;
    uint32_t fpb_mem_high_decode_mech_supported : 1;
    uint32_t                    fpb_num_sec_dev : 5;
    uint32_t         fpb_rid_vec_size_supported : 3;
    uint32_t                              rsvd0 : 5;
    uint32_t     fpb_mem_low_vec_size_supported : 3;
    uint32_t                              rsvd1 : 5;
    uint32_t    fpb_mem_high_vec_size_supported : 3;
    uint32_t                              rsvd2 : 5;
} __attribute__((packed));
static_assert(sizeof(RegFPBCap) == 4);

struct RegFPBRIDVecCtl1
{
    uint32_t fpb_rid_decode_mech_ena : 1;
    uint32_t                   rsvd0 : 3;
    uint32_t fpb_rid_vec_granularity : 4;
    uint32_t                   rsvd1 : 11;
    uint32_t       fpb_rid_vec_start : 13;
} __attribute__((packed));
static_assert(sizeof(RegFPBRIDVecCtl1) == 4);

struct RegFPBRIDVecCtl2
{
    uint32_t               rsvd0 : 3;
    uint32_t rid_secondary_start : 13;
    uint32_t               rsvd1 : 16;
} __attribute__((packed));
static_assert(sizeof(RegFPBRIDVecCtl2) == 4);

struct RegFPBMemLowVecCtl
{
    uint32_t fpb_mem_low_decode_mech_ena : 1;
    uint32_t                       rsvd0 : 3;
    uint32_t fpb_mem_low_vec_granularity : 4;
    uint32_t                       rsvd1 : 12;
    uint32_t       fpb_mem_low_vec_start : 12;
} __attribute__((packed));
static_assert(sizeof(RegFPBMemLowVecCtl) == 4);

struct RegFPBMemHighVecCtl1
{
    uint32_t fpb_mem_high_decode_mech_ena : 1;
    uint32_t                        rsvd0 : 3;
    uint32_t fpb_mem_high_vec_granularity : 4;
    uint32_t                        rsvd1 : 20;
    uint32_t fpb_mem_high_vec_start_lower : 4;
} __attribute__((packed));
static_assert(sizeof(RegFPBMemHighVecCtl1) == 4);

struct RegFPBVectorAccessCtl
{
    uint32_t fpb_vec_access_offset : 8;
    uint32_t                 rsvd0 : 6;
    uint32_t        fpb_vec_select : 2;
    uint32_t                 rsvd1 : 16;
} __attribute__((packed));
static_assert(sizeof(RegFPBVectorAccessCtl) == 4);

struct RegFPBHdrRsvd
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegFPBHdrRsvd) == 2);

struct RegFPBMemHighVecCtl2
{
    uint32_t mem_high_vec_start_upper;
} __attribute__((packed));
static_assert(sizeof(RegFPBMemHighVecCtl2) == 4);

struct RegFPBVectorAccessData
{
    uint32_t vec_access_data;
} __attribute__((packed));
static_assert(sizeof(RegFPBVectorAccessData) == 4);

struct FlatteningPortalBridgeCap
{
    CompatCapHdr            hdr;                // 0x0
    RegFPBHdrRsvd           rsvd;                // 0x2
    RegFPBCap               fpb_cap;             // 0x4
    RegFPBRIDVecCtl1        rid_vec_ctl1;        // 0x8
    RegFPBRIDVecCtl2        rid_vec_ctl2;        // 0xc
    RegFPBMemLowVecCtl      mem_low_vec_ctl;     // 0x10
    RegFPBMemHighVecCtl1    mem_high_vec_ctl1;   // 0x14
    RegFPBMemHighVecCtl2    mem_high_vec_ctl2;   // 0x18
    RegFPBVectorAccessCtl   vec_access_ctl;      // 0x1c
    RegFPBVectorAccessData  vec_access_data;     // 0x20
} __attribute__((packed));
static_assert(sizeof(FlatteningPortalBridgeCap) == 0x24);

struct RegSRIOVCap
{
    uint32_t            vf_migration_capable : 1;
    uint32_t ari_capable_hierarchy_preserved : 1;
    uint32_t      vf_10bit_tag_req_supported : 1;
    uint32_t                            rsvd : 18;
    uint32_t        vf_migration_itr_msg_num : 11;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVCap) == 4);

struct RegSRIOVCtl
{
    uint16_t                vf_ena : 1;
    uint16_t      vf_migration_ena : 1;
    uint16_t  vf_migration_itr_ena : 1;
    uint16_t                vf_mse : 1;
    uint16_t ari_capable_hierarchy : 1;
    uint16_t  vf_10bit_tag_req_ena : 1;
    uint16_t                  rsvd : 10;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVCtl) == 2);

struct RegSRIOVStatus
{
    uint16_t vf_migration_status : 1;
    uint16_t                rsvd : 15;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVStatus) == 2);

struct RegSRIOVNumVFsFcnDepLink
{
    uint32_t      num_vfs : 16;
    uint32_t fcn_dep_link : 8;
    uint32_t         rsvd : 8;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVNumVFsFcnDepLink) == 4);

struct RegSRIOVFirstVFOffsetStride
{
    uint32_t first_vf_offset : 16;
    uint32_t       vf_stride : 16;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVFirstVFOffsetStride) == 4);

struct RegSRIOVVFDeviceID
{
    uint32_t         rsvd : 16;
    uint32_t vf_device_id : 16;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVVFDeviceID) == 4);

struct RegSRIOVVFMigStateArrayOffset
{
    uint32_t    vf_migration_state_bir : 3;
    uint32_t vf_migration_state_offset : 29;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVVFMigStateArrayOffset) == 4);

struct RegSRIOVInitialVFs
{
    uint16_t initial_vfs;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVInitialVFs) == 2);

struct RegSRIOVTotalVFs
{
    uint16_t total_vfs;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVTotalVFs) == 2);

struct RegSRIOVSupportedPageSizes
{
    uint32_t supported_page_sizes;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVSupportedPageSizes) == 4);

struct RegSRIOVSystemPageSize
{
    uint32_t system_page_size;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVSystemPageSize) == 4);

// Normal PCI memory BAR (VFs don't support I/O Space); reused for VF BAR0-5.
struct RegSRIOVVFBAR
{
    uint32_t bar;
} __attribute__((packed));
static_assert(sizeof(RegSRIOVVFBAR) == 4);

struct SRIOVCap
{
    ExtCapHdr                      hdr;                   // 0x0
    RegSRIOVCap                    sriov_cap;              // 0x4
    RegSRIOVCtl                    sriov_ctl;              // 0x8
    RegSRIOVStatus                 sriov_status;           // 0xa
    RegSRIOVInitialVFs             initial_vfs;            // 0xc
    RegSRIOVTotalVFs               total_vfs;              // 0xe
    RegSRIOVNumVFsFcnDepLink       num_vfs_fcn_dep_link;   // 0x10
    RegSRIOVFirstVFOffsetStride    first_vf_off_stride;    // 0x14
    RegSRIOVVFDeviceID             vf_dev_id;              // 0x18
    RegSRIOVSupportedPageSizes     supported_page_sizes;   // 0x1c
    RegSRIOVSystemPageSize         system_page_size;       // 0x20
    RegSRIOVVFBAR                  vf_bar0;                // 0x24
    RegSRIOVVFBAR                  vf_bar1;                // 0x28
    RegSRIOVVFBAR                  vf_bar2;                // 0x2c
    RegSRIOVVFBAR                  vf_bar3;                // 0x30
    RegSRIOVVFBAR                  vf_bar4;                // 0x34
    RegSRIOVVFBAR                  vf_bar5;                // 0x38
    RegSRIOVVFMigStateArrayOffset  vf_mig_state_arr_off;   // 0x3c
} __attribute__((packed));
static_assert(sizeof(SRIOVCap) == 0x40);

struct RegATSCap
{
    uint16_t      invalidate_queue_depth : 5;
    uint16_t            page_aligned_req : 1;
    uint16_t global_invalidate_supported : 1;
    uint16_t  relaxed_ordering_supported : 1;
    uint16_t                        rsvd : 8;
} __attribute__((packed));
static_assert(sizeof(RegATSCap) == 2);

struct RegATSCtl
{
    uint16_t  stu : 5;
    uint16_t rsvd : 10;
    uint16_t  ena : 1;
} __attribute__((packed));
static_assert(sizeof(RegATSCtl) == 2);

struct ATSCap
{
    ExtCapHdr  hdr;      // 0x0
    RegATSCap  ats_cap;  // 0x4
    RegATSCtl  ats_ctl;  // 0x6
} __attribute__((packed));
static_assert(sizeof(ATSCap) == 0x8);

struct RegDVSECHeader1
{
    uint32_t dvsec_vendor_id : 16;
    uint32_t       dvsec_rev : 4;
    uint32_t    dvsec_length : 12;
} __attribute__((packed));
static_assert(sizeof(RegDVSECHeader1) == 4);

struct RegDVSECHeader2
{
    uint16_t dvsec_id;
} __attribute__((packed));
static_assert(sizeof(RegDVSECHeader2) == 2);

// Fixed head only; vendor-specific area (size = dvsec_length-0xa) not represented.
struct DVSECCap
{
    ExtCapHdr        hdr;         // 0x0
    RegDVSECHeader1  dvsec_hdr1;  // 0x4
    RegDVSECHeader2  dvsec_hdr2;  // 0x8
} __attribute__((packed));
static_assert(sizeof(DVSECCap) == 0xa);

struct RegDevSerialNumber
{
    uint64_t serial_number;
} __attribute__((packed));
static_assert(sizeof(RegDevSerialNumber) == 8);

struct DeviceSerialNumberCap
{
    ExtCapHdr           hdr;            // 0x0
    RegDevSerialNumber  serial_number;  // 0x4
} __attribute__((packed));
static_assert(sizeof(DeviceSerialNumberCap) == 0xc);

struct RegTPHReqCap
{
    uint32_t            no_st_mode_supported : 1;
    uint32_t interrupt_vector_mode_supported : 1;
    uint32_t  device_specific_mode_supported : 1;
    uint32_t                           rsvd0 : 5;
    uint32_t      extended_tph_req_supported : 1;
    uint32_t               st_table_location : 2;
    uint32_t                           rsvd1 : 5;
    uint32_t                   st_table_size : 11;
    uint32_t                           rsvd2 : 5;
} __attribute__((packed));
static_assert(sizeof(RegTPHReqCap) == 4);

struct RegTPHReqCtl
{
    uint32_t st_mode_select : 3;
    uint32_t          rsvd0 : 5;
    uint32_t    tph_req_ena : 2;
    uint32_t          rsvd1 : 22;
} __attribute__((packed));
static_assert(sizeof(RegTPHReqCtl) == 4);

struct RegTPHSTEntry
{
    uint16_t st_lower : 8;
    uint16_t st_upper : 8;
} __attribute__((packed));
static_assert(sizeof(RegTPHSTEntry) == 2);

// Fixed head only; followed by (st_table_size+1) RegTPHSTEntry entries
// starting at offset 0xc, present only when st_table_location == 01b.
struct TPHRequesterCap
{
    ExtCapHdr     hdr;      // 0x0
    RegTPHReqCap  tph_cap;  // 0x4
    RegTPHReqCtl  tph_ctl;  // 0x8
} __attribute__((packed));
static_assert(sizeof(TPHRequesterCap) == 0xc);

struct RegDPACap
{
    uint32_t              substate_max : 5;
    uint32_t                     rsvd0 : 3;
    uint32_t   transition_latency_unit : 2;
    uint32_t                     rsvd1 : 2;
    uint32_t    power_allocation_scale : 2;
    uint32_t                     rsvd2 : 2;
    uint32_t transition_latency_value0 : 8;
    uint32_t transition_latency_value1 : 8;
} __attribute__((packed));
static_assert(sizeof(RegDPACap) == 4);

struct RegDPAStatus
{
    uint16_t      substate_status : 5;
    uint16_t                rsvd0 : 3;
    uint16_t substate_ctl_enabled : 1;
    uint16_t                rsvd1 : 7;
} __attribute__((packed));
static_assert(sizeof(RegDPAStatus) == 2);

struct RegDPACtl
{
    uint16_t substate_ctl : 5;
    uint16_t         rsvd : 11;
} __attribute__((packed));
static_assert(sizeof(RegDPACtl) == 2);

// Fixed head only; Power Allocation Array (substate_max+1 bytes) not represented.
struct RegDPALatencyIndicator
{
    uint32_t transition_latency_indicator_bits;
} __attribute__((packed));
static_assert(sizeof(RegDPALatencyIndicator) == 4);

struct DPACap
{
    ExtCapHdr               hdr;               // 0x0
    RegDPACap               dpa_cap;           // 0x4
    RegDPALatencyIndicator  latency_indicator;  // 0x8
    RegDPAStatus            dpa_status;        // 0xc
    RegDPACtl               dpa_ctl;           // 0xe
} __attribute__((packed));
static_assert(sizeof(DPACap) == 0x10);

// Bit layout identical between PF and VF variants.
struct RegResizableBARCap
{
    uint32_t                rsvd : 4;
    uint32_t supported_bar_sizes : 28;
} __attribute__((packed));
static_assert(sizeof(RegResizableBARCap) == 4);

struct RegResizableBARCtl
{
    uint32_t               bar_index : 3;
    uint32_t                   rsvd0 : 2;
    uint32_t      num_resizable_bars : 3;
    uint32_t                bar_size : 6;
    uint32_t                   rsvd1 : 2;
    uint32_t supported_bar_sizes_ext : 16;
} __attribute__((packed));
static_assert(sizeof(RegResizableBARCtl) == 4);

struct RegResizableBAREntry
{
    RegResizableBARCap  cap;
    RegResizableBARCtl  ctl;
} __attribute__((packed));
static_assert(sizeof(RegResizableBAREntry) == 8);

// Fixed head only; 1-6 RegResizableBAREntry entries follow (count = num_resizable_bars).
struct ResizableBARCap
{
    ExtCapHdr  hdr;  // 0x0
} __attribute__((packed));
static_assert(sizeof(ResizableBARCap) == 0x4);

// Fixed head only; 1-6 RegResizableBAREntry entries follow (count = num_resizable_bars).
struct VFResizableBARCap
{
    ExtCapHdr  hdr;  // 0x0
} __attribute__((packed));
static_assert(sizeof(VFResizableBARCap) == 0x4);

struct RegRCLinkDeclElemSelfDesc
{
    uint32_t     element_type : 4;
    uint32_t            rsvd0 : 4;
    uint32_t num_link_entries : 8;
    uint32_t     component_id : 8;
    uint32_t      port_number : 8;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkDeclElemSelfDesc) == 4);

struct RegRCLinkDeclLinkDesc
{
    uint32_t          link_valid : 1;
    uint32_t           link_type : 1;
    uint32_t   assoc_rcrb_header : 1;
    uint32_t                rsvd : 13;
    uint32_t target_component_id : 8;
    uint32_t  target_port_number : 8;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkDeclLinkDesc) == 4);

struct RegRCLinkDeclLinkEntryRsvd
{
    uint32_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkDeclLinkEntryRsvd) == 4);

// Meaning depends on link_desc.link_type; Link Type 1's variable-width BDF
// encoding (Section 7.9.8.3.2.2) isn't modeled here.
struct RegRCLinkDeclLinkAddress
{
    uint64_t link_address;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkDeclLinkAddress) == 8);

struct RCLinkDeclLinkEntry
{
    RegRCLinkDeclLinkDesc      link_desc;     // 0x0
    RegRCLinkDeclLinkEntryRsvd rsvd;          // 0x4
    RegRCLinkDeclLinkAddress   link_address;  // 0x8
} __attribute__((packed));
static_assert(sizeof(RCLinkDeclLinkEntry) == 0x10);

struct RegRCLinkDeclCapRsvd
{
    uint64_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegRCLinkDeclCapRsvd) == 8);

// Fixed head only; RCLinkDeclLinkEntry entries follow (count = num_link_entries).
struct RCLinkDeclarationCap
{
    ExtCapHdr                   hdr;             // 0x0
    RegRCLinkDeclElemSelfDesc   elem_self_desc;   // 0x4
    RegRCLinkDeclCapRsvd        rsvd;             // 0x8
} __attribute__((packed));
static_assert(sizeof(RCLinkDeclarationCap) == 0x10);

// Capabilities and Control registers are fully RsvdP in this version of the spec.
struct RegPhys16GTStatus
{
    uint32_t       eq_16gts_complete : 1;
    uint32_t eq_16gts_phase1_success : 1;
    uint32_t eq_16gts_phase2_success : 1;
    uint32_t eq_16gts_phase3_success : 1;
    uint32_t       link_eq_req_16gts : 1;
    uint32_t                    rsvd : 27;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTStatus) == 4);

struct RegPhys16GTLaneEqCtl
{
    uint8_t ds_port_16gts_tx_preset : 4;
    uint8_t us_port_16gts_tx_preset : 4;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTLaneEqCtl) == 1);

// Capabilities and Control registers are fully RsvdP in this version of the spec.
struct RegPhys16GTCap
{
    uint32_t cap;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTCap) == 4);

struct RegPhys16GTCtl
{
    uint32_t ctl;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTCtl) == 4);

struct RegPhys16GTLocalDPMStatus
{
    uint32_t local_dpm_status;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTLocalDPMStatus) == 4);

struct RegPhys16GTFirstRetimerDPMStatus
{
    uint32_t first_retimer_dpm_status;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTFirstRetimerDPMStatus) == 4);

struct RegPhys16GTSecondRetimerDPMStatus
{
    uint32_t second_retimer_dpm_status;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTSecondRetimerDPMStatus) == 4);

struct RegPhys16GTRsvd
{
    uint32_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegPhys16GTRsvd) == 4);

// Fixed head only; up to 32 RegPhys16GTLaneEqCtl entries follow (one per Lane).
struct Phys16GTCap
{
    ExtCapHdr                          hdr;                       // 0x0
    RegPhys16GTCap                     cap;                       // 0x4
    RegPhys16GTCtl                     ctl;                       // 0x8
    RegPhys16GTStatus                  status;                    // 0xc
    RegPhys16GTLocalDPMStatus          local_dpm_status;          // 0x10
    RegPhys16GTFirstRetimerDPMStatus   first_retimer_dpm_status;  // 0x14
    RegPhys16GTSecondRetimerDPMStatus  second_retimer_dpm_status; // 0x18
    RegPhys16GTRsvd                    rsvd;                      // 0x1c
} __attribute__((packed));
static_assert(sizeof(Phys16GTCap) == 0x20);

struct RegPhys32GTCap
{
    uint32_t eq_bypass_highest_rate_supported : 1;
    uint32_t           no_eq_needed_supported : 1;
    uint32_t                            rsvd0 : 6;
    uint32_t     mod_ts_usage_mode0_supported : 1;
    uint32_t     mod_ts_usage_mode1_supported : 1;
    uint32_t     mod_ts_usage_mode2_supported : 1;
    uint32_t      mod_ts_reserved_usage_modes : 5;
    uint32_t                            rsvd1 : 16;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTCap) == 4);

struct RegPhys32GTCtl
{
    uint32_t eq_bypass_highest_rate_disable : 1;
    uint32_t           no_eq_needed_disable : 1;
    uint32_t                          rsvd0 : 6;
    uint32_t     mod_ts_usage_mode_selected : 3;
    uint32_t                          rsvd1 : 21;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTCtl) == 4);

struct RegPhys32GTStatus
{
    uint32_t               eq_32gts_complete : 1;
    uint32_t         eq_32gts_phase1_success : 1;
    uint32_t         eq_32gts_phase2_success : 1;
    uint32_t         eq_32gts_phase3_success : 1;
    uint32_t               link_eq_req_32gts : 1;
    uint32_t            modified_ts_received : 1;
    uint32_t recv_enhanced_link_behavior_ctl : 2;
    uint32_t                 tx_precoding_on : 1;
    uint32_t              tx_precode_request : 1;
    uint32_t           no_eq_needed_received : 1;
    uint32_t                            rsvd : 21;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTStatus) == 4);

struct RegPhys32GTRecvModTSData1
{
    uint32_t recv_mod_ts_usage_mode : 3;
    uint32_t      recv_mod_ts_info1 : 13;
    uint32_t  recv_mod_ts_vendor_id : 16;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTRecvModTSData1) == 4);

struct RegPhys32GTRecvModTSData2
{
    uint32_t            recv_mod_ts_info2 : 24;
    uint32_t alt_proto_negotiation_status : 2;
    uint32_t                         rsvd : 6;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTRecvModTSData2) == 4);

struct RegPhys32GTXmitModTSData1
{
    uint32_t xmit_mod_ts_usage_mode : 3;
    uint32_t      xmit_mod_ts_info1 : 13;
    uint32_t  xmit_mod_ts_vendor_id : 16;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTXmitModTSData1) == 4);

struct RegPhys32GTXmitModTSData2
{
    uint32_t            xmit_mod_ts_info2 : 24;
    uint32_t alt_proto_negotiation_status : 2;
    uint32_t                         rsvd : 6;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTXmitModTSData2) == 4);

struct RegPhys32GTLaneEqCtl
{
    uint8_t ds_port_32gts_tx_preset : 4;
    uint8_t us_port_32gts_tx_preset : 4;
} __attribute__((packed));
static_assert(sizeof(RegPhys32GTLaneEqCtl) == 1);

// Fixed head only; up to 32 RegPhys32GTLaneEqCtl entries follow (one per Lane).
struct Phys32GTCap
{
    ExtCapHdr                   hdr;                 // 0x0
    RegPhys32GTCap              cap;                 // 0x4
    RegPhys32GTCtl              ctl;                 // 0x8
    RegPhys32GTStatus           status;              // 0xc
    RegPhys32GTRecvModTSData1   recv_mod_ts_data1;   // 0x10
    RegPhys32GTRecvModTSData2   recv_mod_ts_data2;   // 0x14
    RegPhys32GTXmitModTSData1   xmit_mod_ts_data1;   // 0x18
    RegPhys32GTXmitModTSData2   xmit_mod_ts_data2;   // 0x1c
} __attribute__((packed));
static_assert(sizeof(Phys32GTCap) == 0x20);

struct RegMarginPortCap
{
    uint16_t margining_uses_driver_sw : 1;
    uint16_t                     rsvd : 15;
} __attribute__((packed));
static_assert(sizeof(RegMarginPortCap) == 2);

struct RegMarginPortStatus
{
    uint16_t    margining_ready : 1;
    uint16_t margining_sw_ready : 1;
    uint16_t               rsvd : 14;
} __attribute__((packed));
static_assert(sizeof(RegMarginPortStatus) == 2);

struct RegMarginLaneCtl
{
    uint16_t receiver_number : 3;
    uint16_t     margin_type : 3;
    uint16_t     usage_model : 1;
    uint16_t            rsvd : 1;
    uint16_t  margin_payload : 8;
} __attribute__((packed));
static_assert(sizeof(RegMarginLaneCtl) == 2);

struct RegMarginLaneStatus
{
    uint16_t receiver_number_status : 3;
    uint16_t     margin_type_status : 3;
    uint16_t     usage_model_status : 1;
    uint16_t                   rsvd : 1;
    uint16_t  margin_payload_status : 8;
} __attribute__((packed));
static_assert(sizeof(RegMarginLaneStatus) == 2);

struct RegMarginLaneEntry
{
    RegMarginLaneCtl     ctl;     // 0x0
    RegMarginLaneStatus  status;  // 0x2
} __attribute__((packed));
static_assert(sizeof(RegMarginLaneEntry) == 4);

// Fixed head only; up to 32 RegMarginLaneEntry entries follow (one per Lane).
struct LaneMarginingCap
{
    ExtCapHdr             hdr;          // 0x0
    RegMarginPortCap      port_cap;     // 0x4
    RegMarginPortStatus   port_status;  // 0x6
} __attribute__((packed));
static_assert(sizeof(LaneMarginingCap) == 0x8);

// ext cap id 0x2 (no MFVC) or 0x9 (MFVC present); same layout either way.
struct RegPortVCCap1
{
    uint32_t              ext_vc_count : 3;
    uint32_t                     rsvd0 : 1;
    uint32_t     low_prio_ext_vc_count : 3;
    uint32_t                     rsvd1 : 1;
    uint32_t           reference_clock : 2;
    uint32_t port_arb_table_entry_size : 2;
    uint32_t                     rsvd2 : 20;
} __attribute__((packed));
static_assert(sizeof(RegPortVCCap1) == 4);

struct RegPortVCCap2
{
    uint32_t   vc_arb_capability : 8;
    uint32_t                rsvd : 16;
    uint32_t vc_arb_table_offset : 8;
} __attribute__((packed));
static_assert(sizeof(RegPortVCCap2) == 4);

struct RegPortVCCtl
{
    uint16_t load_vc_arb_table : 1;
    uint16_t     vc_arb_select : 3;
    uint16_t              rsvd : 12;
} __attribute__((packed));
static_assert(sizeof(RegPortVCCtl) == 2);

struct RegPortVCStatus
{
    uint16_t vc_arb_table_status : 1;
    uint16_t                rsvd : 15;
} __attribute__((packed));
static_assert(sizeof(RegPortVCStatus) == 2);

struct RegVCResourceCap
{
    uint32_t   port_arb_capability : 8;
    uint32_t                 rsvd0 : 6;
    uint32_t         undefined_bit : 1;
    uint32_t    reject_snoop_trans : 1;
    uint32_t        max_time_slots : 7;
    uint32_t                 rsvd1 : 1;
    uint32_t port_arb_table_offset : 8;
} __attribute__((packed));
static_assert(sizeof(RegVCResourceCap) == 4);

struct RegVCResourceCtl
{
    uint32_t           tc_vc_map : 8;
    uint32_t               rsvd0 : 8;
    uint32_t load_port_arb_table : 1;
    uint32_t     port_arb_select : 3;
    uint32_t               rsvd1 : 4;
    uint32_t               vc_id : 3;
    uint32_t               rsvd2 : 4;
    uint32_t           vc_enable : 1;
} __attribute__((packed));
static_assert(sizeof(RegVCResourceCtl) == 4);

struct RegVCResourceStatus
{
    uint16_t  port_arb_table_status : 1;
    uint16_t vc_negotiation_pending : 1;
    uint16_t                   rsvd : 14;
} __attribute__((packed));
static_assert(sizeof(RegVCResourceStatus) == 2);

struct RegVCResourceBlockRsvd
{
    uint16_t rsvd;
} __attribute__((packed));
static_assert(sizeof(RegVCResourceBlockRsvd) == 2);

struct VCResourceBlock
{
    RegVCResourceCap      cap;     // 0x0
    RegVCResourceCtl      ctl;     // 0x4
    RegVCResourceStatus   status;  // 0x8
    RegVCResourceBlockRsvd rsvd;   // 0xa
} __attribute__((packed));
static_assert(sizeof(VCResourceBlock) == 0xc);

// Packs two 4-bit VC Arbitration Table phases per byte (low nibble = even phase).
struct RegVCArbTablePhasePair
{
    uint8_t  phase_n_vc_id : 3;
    uint8_t   phase_n_rsvd : 1;
    uint8_t phase_n1_vc_id : 3;
    uint8_t  phase_n1_rsvd : 1;
} __attribute__((packed));
static_assert(sizeof(RegVCArbTablePhasePair) == 1);

// Fixed head only; ext_vc_count+1 VCResourceBlock entries follow. Arbitration
// tables are located separately via their *_table_offset fields.
struct VirtualChannelCap
{
    ExtCapHdr        hdr;             // 0x0
    RegPortVCCap1    port_vc_cap1;    // 0x4
    RegPortVCCap2    port_vc_cap2;    // 0x8
    RegPortVCCtl     port_vc_ctl;     // 0xc
    RegPortVCStatus  port_vc_status;  // 0xe
} __attribute__((packed));
static_assert(sizeof(VirtualChannelCap) == 0x10);

// ext cap id 0x8; layout identical to VirtualChannelCap ("Port Arbitration"
// relabeled "Function Arbitration"), same fixed-head/VCResourceBlock pattern.
struct MFVCCap
{
    ExtCapHdr        hdr;             // 0x0
    RegPortVCCap1    port_vc_cap1;    // 0x4
    RegPortVCCap2    port_vc_cap2;    // 0x8
    RegPortVCCtl     port_vc_ctl;     // 0xc
    RegPortVCStatus  port_vc_status;  // 0xe
} __attribute__((packed));
static_assert(sizeof(MFVCCap) == 0x10);







