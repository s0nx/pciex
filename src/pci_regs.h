#pragma once

#include "util.h"
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
        return "Primary Latency Timer";
    case Type1Cfg::header_type:
        return "Header Type";
    case Type1Cfg::bist:
        return "BIST";
    case Type1Cfg::bar0:
        return "BAR 0";
    case Type1Cfg::bar1:
        return "BAR 1";
    case Type1Cfg::prim_bus_num:
        return "Primary Bus Number";
    case Type1Cfg::sec_bus_num:
        return "Secondary Bus Number";
    case Type1Cfg::sub_bus_num:
        return "Subordinate Bus Number";
    case Type1Cfg::sec_lat_timer:
        return "Secondary Latency Timer";
    case Type1Cfg::io_base:
        return "I/O Base";
    case Type1Cfg::io_limit:
        return "I/O Limit";
    case Type1Cfg::sec_status:
        return "Secondary Status";
    case Type1Cfg::mem_base:
        return "Memoru Base";
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

struct RegCapPtr
{
    uint8_t ptr;
} __attribute__((packed));
static_assert(sizeof(RegCapPtr) == 1);

enum class CapType
{
    compat,
    extended
};

constexpr uint32_t ext_cap_cfg_off = 0x100;

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

