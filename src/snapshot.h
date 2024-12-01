// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include "provider_iface.h"

#include <array>
#include <sys/uio.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace snapshot {
namespace meta {

// snapshot header metadata
struct SHeaderMd
{
     uint8_t magic_[5];
    uint64_t ts_;           // snapshot creation time is seconds since epoch
    uint64_t fsize_;        // full snapshot file size including this header
    uint32_t dev_cnt_;      // number of devices
    uint32_t bus_cnt_;      // number of buses
} __attribute__((packed));
static_assert(sizeof(SHeaderMd) == 0x1d);

// static part of device metadata
struct SDeviceMd
{
    uint64_t d_bdf_;
    uint8_t  cfg_space_len_;      // 0 - 256b, 1 - 4kb
    uint8_t  dev_res_len_;        // number of resource entries
    uint16_t numa_node_;
    uint16_t iommu_group_;
    uint8_t  driver_name_len_;    // including '\0'
    uint8_t  is_final_dev_entry_; // last device indicator
} __attribute__((packed));
static_assert(sizeof(SDeviceMd) == 0x10);

} // namespace meta

// Current shapshot format:
// ╔════════════════════════════════════════════════════════════╗
// ║  main shapshot header: off [+0x0]                          ║
// ║ ┌─────────────────────┐                                    ║
// ║ │ @SHeaderMd          │                                    ║
// ║ └─────────────────────┘                                    ║
// ║  devices metadata section start: off [+0x1d]               ║
// ║ ┌───────────────────────┐                                  ║
// ║ │ dev #N metadata block:│                                  ║
// ║ │┌────────────────────┐ │                                  ║
// ║ ││┌────────────┐      │ │  ─┐                              ║
// ║ │││ @SDeviceMd │      │ │   │ main device descriptor (16b) ║
// ║ ││└────────────┘      │ │  ─┘                              ║
// ║ ││┌──────────────────┐│ │  ─┐ variable-sized metadata      ║
// ║ │││ dynamic metadata ││ │   │ (resources, driver name, etc)║
// ║ ││└──────────────────┘│ │  ─┘                              ║
// ║ ││┌──────────────────┐│ │  ─┐                              ║
// ║ │││ cfg space buffer ││ │   │ cfg space (256b or 4096b)    ║
// ║ │││                  ││ │   │                              ║
// ║ ││└──────────────────┘│ │  ─┘                              ║
// ║ │└────────────────────┘ │                                  ║
// ║ │                       │                                  ║
// ║ │ . . .                 │                                  ║
// ║ └───────────────────────┘                                  ║
// ║  buses metadata section:                                   ║
// ║ ┌───────────────────────┐                                  ║
// ║ │┌─────────────────────┐│ ─┐                               ║
// ║ ││ bus #0 descriptor   ││  │ @BusDesc                      ║
// ║ │└─────────────────────┘│ ─┘                               ║
// ║ │ . . .                 │                                  ║
// ║ │┌─────────────────────┐│                                  ║
// ║ ││ bus #N descriptor   ││                                  ║
// ║ │└─────────────────────┘│                                  ║
// ║ └───────────────────────┘                                  ║
// ╚════════════════════════════════════════════════════════════╝

constexpr uint8_t iov_cnt = 3;

class SnapshotProvider : public Provider
{
public:
    SnapshotProvider() = delete;
    explicit SnapshotProvider(const fs::path spath) :
        Provider(),
        bytes_written_(0),
        bytes_read_(0),
        cur_dev_num_(0),
        total_dev_num_(0),
        total_bus_num_(0),
        off_(0),
        fd_(-1),
        dyn_md_buf_(nullptr),
        cur_dyn_md_buf_len_(0),
        full_snapshot_path_(spath),
        snapshot_filename_(spath.filename())
    {}

    ~SnapshotProvider()
    {
        close(fd_);
    }

    std::vector<BusDesc>         GetBusDescriptors() override;
    std::vector<DeviceDesc>      GetPCIDevDescriptors() override;
    std::string                  GetProviderName() const override { return "Snapshot"; }

    void SaveState(const std::vector<DeviceDesc> &devs,
                   const std::vector<BusDesc> &buses) override;

private:
    uint64_t                          bytes_written_;
    uint64_t                          bytes_read_;
    uint32_t                          cur_dev_num_;
    uint32_t                          total_dev_num_;
    uint32_t                          total_bus_num_;
    size_t                            off_;
    int                               fd_;
    OpaqueBuf                         dyn_md_buf_;
    size_t                            cur_dyn_md_buf_len_;
    fs::path                          full_snapshot_path_;
    fs::path                          snapshot_filename_;
    fs::path                          snapshot_dir_;
    std::array<struct iovec, iov_cnt> iovec_;

    size_t CurIovecDataLen() noexcept;
    bool StoreMainHeader(const uint64_t size, const uint32_t dev_cnt, uint32_t const bus_cnt);
    bool SnapshotCapturePrepare();
    bool SnapshotParsePrepare();
    bool SnapshotFinalize();
    bool WriteDeviceMetadata(const DeviceDesc &dev_desc);
    bool WriteBusesMetadata(const std::vector<BusDesc> &buses);
};


} // namespace snapshot
