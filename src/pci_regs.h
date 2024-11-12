// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdint>

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
