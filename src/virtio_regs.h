// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <cstdint>

namespace virtio {

// VirtIO capabilities IDs
// See VirtIO spec paragraph 4.1.4 for details
enum class VirtIOCapID
{
    common_cfg    = 0x1,
    notifications = 0x2,
    isr_status    = 0x3,
    dev_spec_cfg  = 0x4,
    pci_cfg_acc   = 0x5,
    shm_cfg       = 0x8,

    cap_id_max    = shm_cfg
};

constexpr auto VirtIOCapIDName(const VirtIOCapID cap_id)
{
    switch (cap_id) {
    case VirtIOCapID::common_cfg:
        return "Common Configuration";
    case VirtIOCapID::notifications:
        return "Notifications";
    case VirtIOCapID::isr_status:
        return "ISR status";
    case VirtIOCapID::dev_spec_cfg:
        return "Device-specific configuration";
    case VirtIOCapID::pci_cfg_acc:
        return "PCI configuration access";
    case VirtIOCapID::shm_cfg:
        return "Shared memory";
    default:
        return "";
    }
}

// VirtIO structure capability
struct VirtIOPCICap
{
     uint8_t cap_id;
     uint8_t next_cap;
     uint8_t cap_len;
     uint8_t cfg_type;   // structure type (one of VirtIOCapID)
     uint8_t bar_idx;    // index of BAR
     uint8_t id;         // multiple capabilities of the same type
    uint16_t rsvd;
    uint32_t bar_off;    // offset within bar
    uint32_t length;     // length of the structure, in bytes
} __attribute__((packed));
static_assert(sizeof(VirtIOPCICap) == 0x10);

constexpr uint16_t virtio_rh_qumranet_vid     = 0x1af4;
constexpr uint16_t virtio_dev_id_lower        = 0x1000;
constexpr uint16_t virtio_dev_id_upper        = 0x107f;
constexpr uint16_t virtio_dev_id_modern_lower = 0x1040;

inline bool is_virtio_dev(const uint16_t vid, const uint16_t dev_id)
{
    if (vid != virtio_rh_qumranet_vid)
        return false;

    return dev_id >= virtio_dev_id_lower && dev_id <= virtio_dev_id_upper;
}

inline bool is_virtio_modern(const uint16_t dev_id)
{
    return dev_id >= 0x1040;
}

} // namespace virtio

