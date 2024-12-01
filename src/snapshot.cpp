// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#include "log.h"
#include "snapshot.h"
#include <numeric>
#include <fmt/chrono.h>

#include <fcntl.h>

extern Logger logger;

namespace snapshot {

size_t
SnapshotProvider::CurIovecDataLen() noexcept
{
    return std::accumulate(iovec_.begin(), iovec_.end(),
                           0, [](auto a, auto b) { return a + b.iov_len; });
}

bool
SnapshotProvider::StoreMainHeader(const uint64_t size, const uint32_t dev_cnt, const uint32_t bus_cnt)
{
    auto tm_s = std::time(nullptr);

    uint8_t buffer[sizeof(meta::SHeaderMd)];
    auto header = reinterpret_cast<meta::SHeaderMd *>(&buffer);

    std::memcpy(header->magic_, "xeicp", 5);
    header->ts_ = tm_s;
    header->fsize_ = size + sizeof(buffer);
    header->dev_cnt_ = dev_cnt;
    header->bus_cnt_ = bus_cnt;

    logger.log(Verbosity::INFO,
               "Snapshot header: ts -> {:%Y/%m/%d - %T %z} full size -> {} dev_cnt {} bus_cnt {}",
               fmt::localtime(tm_s), size + sizeof(buffer), dev_cnt, bus_cnt);

    auto res = pwrite(fd_, buffer, sizeof(buffer), 0);
    if (res < 0) {
        logger.log(Verbosity::FATAL, "snapshot: Failed to write snapshot header: path {} err {}",
                    full_snapshot_path_.c_str(), errno);
        return false;
    } else if (res != sizeof(buffer)) {
        logger.log(Verbosity::FATAL, "snapshot: Main header has not been fully written: path {} written [{}/{}]b",
                    full_snapshot_path_.c_str(), res, sizeof(buffer));
        return false;
    }

    return true;
}

bool
SnapshotProvider::SnapshotCapturePrepare()
{
    snapshot_dir_ = full_snapshot_path_.remove_filename();

    fd_ = open(snapshot_dir_.c_str(), O_TMPFILE | O_WRONLY, 0644);
    if (fd_ < 0) {
        logger.log(Verbosity::FATAL, "snapshot: Failed to open file for capture: path {} err {}",
                    full_snapshot_path_.c_str(), errno);
        return false;
    }

    // Set initial write offset to the size header metadata.
    // The header itself would written last.
    off_ += sizeof(meta::SHeaderMd);

    return true;
}

bool
SnapshotProvider::SnapshotFinalize()
{
    auto res = linkat(fd_, "", AT_FDCWD, snapshot_filename_.c_str(), AT_EMPTY_PATH);
    if (res < 0) {
        logger.log(Verbosity::FATAL, "snapshot: Failed to publish snapshot file: path {} err {}",
                   full_snapshot_path_.c_str(), errno);
        return false;
    }

    return true;
}

bool
SnapshotProvider::WriteDeviceMetadata(const DeviceDesc &dev_desc)
{
    auto dom  = dev_desc.dbdf_ >> 24 & 0xffff;
    auto bus  = dev_desc.dbdf_ >> 16 & 0xff;
    auto dev  = dev_desc.dbdf_ >> 8 & 0xff;
    auto func = dev_desc.dbdf_ & 0xff;

    logger.log(Verbosity::INFO, "snapshot: saving metadata for [{:04x}|{:02x}:{:02x}.{:x}] device [{} / {}]",
               dom, bus, dev, func, ++cur_dev_num_, total_dev_num_);

    meta::SDeviceMd static_dev_md;
    static_dev_md.d_bdf_ = dev_desc.dbdf_;
    static_dev_md.cfg_space_len_ = (dev_desc.cfg_space_len_ == 256) ? 0 : 1;
    static_dev_md.dev_res_len_ = dev_desc.resources_.size();
    static_dev_md.numa_node_ = dev_desc.numa_node_;
    static_dev_md.iommu_group_ = dev_desc.iommu_group_;
    static_dev_md.driver_name_len_ = dev_desc.driver_name_.empty() ?
                                     0 : dev_desc.driver_name_.length() + 1;
    static_dev_md.is_final_dev_entry_ = (cur_dev_num_ == total_dev_num_) ? 1 : 0;

    iovec_[0].iov_base = &static_dev_md;
    iovec_[0].iov_len = sizeof(static_dev_md);

    auto dyn_md_size = dev_desc.resources_.size() * dev_res_desc_size + // number of device resources triples
                       static_dev_md.driver_name_len_;                  // driver name + '\0'

    if (cur_dyn_md_buf_len_ < dyn_md_size) {
        auto old_dyn_md_buf_size = cur_dyn_md_buf_len_;
        dyn_md_buf_.reset(new (std::nothrow) uint8_t[dyn_md_size]);
        if (dyn_md_buf_ == nullptr) {
            logger.log(Verbosity::FATAL, "snapshot: Failed to enlarge dyn md buffer");
            return false;
        }
        cur_dyn_md_buf_len_ = dyn_md_size;
        logger.log(Verbosity::INFO, "snapshot: increasing current dyn md buffer len: {} -> {}",
                   old_dyn_md_buf_size, cur_dyn_md_buf_len_);
    }

    // unpack resources to dynamic md buffer
    for (int i = 0; const auto &res : dev_desc.resources_) {
        auto desc = reinterpret_cast<uint64_t *>(dyn_md_buf_.get());
        std::tie(desc[i], desc[i + 1], desc[i + 2]) = res;
        i += std::tuple_size<DevResourceDesc>{};
    }

    // save driver name
    if (static_dev_md.driver_name_len_ != 0) {
        auto dyn_md_off = dev_desc.resources_.size() * dev_res_desc_size;
        auto drv_name_ptr = reinterpret_cast<uint8_t *>(dyn_md_buf_.get() + dyn_md_off);
        std::memcpy(drv_name_ptr, dev_desc.driver_name_.c_str(),
                    dev_desc.driver_name_.length() + 1);
    }

    iovec_[1].iov_base = dyn_md_buf_.get();
    iovec_[1].iov_len = dyn_md_size;

    // config space buffer
    iovec_[2].iov_base = dev_desc.cfg_space_.get();
    iovec_[2].iov_len = dev_desc.cfg_space_len_;

    auto total_iovec_data_len = CurIovecDataLen();
    auto res = pwritev(fd_, iovec_.data(), iovec_.size(), off_);
    if (res < 0) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Failed to write metadata for [{:04x}|{:02x}:{:02x}.{:x}], err {}",
                    dom, bus, dev, func, errno);
        return false;
    } else if ((uint64_t)res != total_iovec_data_len) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Metadata for [{:04x}|{:02x}:{:02x}.{:x}] has not been fully written [{} / {}]",
                    dom, bus, dev, func, res, total_iovec_data_len);
        return false;
    }

    bytes_written_ += total_iovec_data_len;
    off_ += total_iovec_data_len;

    logger.log(Verbosity::INFO,
               "snapshot: successfully wrote metadata for [{:04x}|{:02x}:{:02x}.{:x}], b_wr {} off {} ",
                dom, bus, dev, func, bytes_written_, off_);

    return true;
}

bool
SnapshotProvider::WriteBusesMetadata(const std::vector<BusDesc> &buses)
{
    logger.log(Verbosity::INFO, "snapshot: saving buses metadata, buses cnt -> {} snapshot off {}",
               buses.size(), off_);

    auto buses_md_size = buses.size() * bus_desc_size;
    if (cur_dyn_md_buf_len_ < buses_md_size) {
        auto old_dyn_md_buf_size = cur_dyn_md_buf_len_;
        dyn_md_buf_.reset(new (std::nothrow) uint8_t[buses_md_size]);
        if (dyn_md_buf_ == nullptr) {
            logger.log(Verbosity::FATAL, "snapshot: Failed to enlarge dyn md buffer");
            return false;
        }
        cur_dyn_md_buf_len_ = buses_md_size;
        logger.log(Verbosity::INFO, "snapshot: Increasing current dyn md buffer len: {} -> {}",
                   old_dyn_md_buf_size, cur_dyn_md_buf_len_);
    }

    // unpack resources to dynamic md buffer
    for (int i = 0; const auto &bus_desc : buses) {
        auto desc = reinterpret_cast<uint16_t *>(dyn_md_buf_.get());
        std::tie(desc[i], desc[i + 1], desc[i + 2]) = bus_desc;
        i += std::tuple_size<BusDesc>{};
    }

    auto res = pwrite(fd_, dyn_md_buf_.get(), buses_md_size, off_);
    if (res < 0) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Failed to write buses metadata, err {}", errno);
        return false;
    } else if ((uint64_t)res != buses_md_size) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Buses metadata has not been fully written [{} / {}]",
                    res, buses_md_size);
        return false;
    }

    bytes_written_ += buses_md_size;
    off_ += buses_md_size;

    logger.log(Verbosity::INFO,
               "snapshot: successfully wrote buses metadata, b_wr {} off {} ",
                bytes_written_, off_);

    return true;
}

void
SnapshotProvider::SaveState(const std::vector<DeviceDesc> &devs,
                            const std::vector<BusDesc> &buses)
{
    auto save_error = []{ throw std::runtime_error("Failed to create snapshot"); };

    if (!SnapshotCapturePrepare())
        return save_error();

    total_dev_num_ = devs.size();
    for (const auto &dev_desc : devs)
        if (!WriteDeviceMetadata(dev_desc))
            return save_error();

    if (!WriteBusesMetadata(buses))
        return save_error();

    if (!StoreMainHeader(bytes_written_, devs.size(), buses.size()))
        return save_error();

    if (!SnapshotFinalize())
        return save_error();
}

bool
SnapshotProvider::SnapshotParsePrepare()
{
    // just in case
    if (!fs::exists(full_snapshot_path_)) {
        logger.log(Verbosity::FATAL,
                   "snapshot: '{}' doesn't exist", full_snapshot_path_.c_str());
        return false;
    }

    std::error_code ec;
    auto actual_snap_size = fs::file_size(full_snapshot_path_, ec);
    if (ec) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Failed to check snapshot size, path {} err {}",
                   full_snapshot_path_.c_str(), ec.message());
        return false;
    }

    fd_ = open(full_snapshot_path_.c_str(), O_RDONLY);
    if (fd_ < 0) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Failed to open snapshot, path {} err {}",
                   full_snapshot_path_.c_str(), errno);
        return false;
    }

    // XXX: cannot bind packed field to ...
    // works with explicit casting to const *
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36566
    uint8_t buffer[sizeof(meta::SHeaderMd)];
    auto snap_md = reinterpret_cast<const meta::SHeaderMd *>(&buffer);

    auto res = pread(fd_, buffer, sizeof(buffer), off_);
    if (res < 0) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Failed to read main metadata, err {}", errno);
        return false;
    } else if ((uint64_t)res != sizeof(buffer)) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Main metadata header has not been fully read [{} / {}]",
                    res, sizeof(buffer));
        return false;
    }

    if (std::memcmp(snap_md->magic_, "xeicp", 5)) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Magic value is incorrect, path {}",
                   full_snapshot_path_.c_str());
        return false;
    }

    if (snap_md->fsize_ != actual_snap_size) {
        logger.log(Verbosity::FATAL,
                   "snapshot: encoded/actual file size mismatch ({} != {}), path {}",
                   snap_md->fsize_, actual_snap_size, full_snapshot_path_.c_str());
        return false;
    }

    using namespace std::chrono;

    system_clock::time_point ts_tp{seconds{snap_md->ts_}};
    time_t in_time_t = system_clock::to_time_t(ts_tp);
    // XXX: using fmt::localtime() produces warnings like this one:
    // /usr/include/fmt/base.h:1213:31: warning: writing 8 bytes into a region of size 7
    // Seems to be a compiler false-positive warning
    logger.log(Verbosity::INFO,
               "snapshot: created {:%Y/%m/%d - %T %z} size {} dev_cnt {} bus_cnt {}",
               fmt::localtime(in_time_t), snap_md->fsize_, snap_md->dev_cnt_,
               snap_md->bus_cnt_);

    if (snap_md->dev_cnt_ == 0 || snap_md->bus_cnt_ == 0) {
        logger.log(Verbosity::FATAL,
                   "snapshot: parsed dev_cnt and/or bus_cnt is zero");
        return false;
    }

    off_ += sizeof(buffer);
    bytes_read_ += sizeof(buffer);

    total_dev_num_ = snap_md->dev_cnt_;
    total_bus_num_ = snap_md->bus_cnt_;
    cur_dev_num_ = 1;

    return true;
}

std::vector<BusDesc>
SnapshotProvider::GetBusDescriptors()
{
    logger.log(Verbosity::INFO,
               "snapshot: Reading metadata for {} buses, off {}", total_bus_num_, off_);

    auto parse_error = []() { throw std::runtime_error("Failed to parse snapshot"); };
    std::vector<BusDesc> buses;

    auto bus_meta_len = total_bus_num_ * bus_desc_size;
    dyn_md_buf_.reset(new (std::nothrow) uint8_t[bus_meta_len]);
    if (dyn_md_buf_ == nullptr) {
        logger.log(Verbosity::FATAL, "snapshot: Failed to allocate dyn md buffer");
        parse_error();
    }
    cur_dyn_md_buf_len_ = bus_meta_len;

    auto res = pread(fd_, dyn_md_buf_.get(), bus_meta_len, off_);
    if (res < 0) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Failed to read buses metadata, err {}", errno);
        parse_error();
    } else if ((uint64_t)res != bus_meta_len) {
        logger.log(Verbosity::FATAL,
                   "snapshot: Buses metadata has not been fully read [{} / {}]",
                    res, bus_meta_len);
        parse_error();
    }

    // unpack bus descriptors from dynamic md buffer
    auto bus_desc_entry = reinterpret_cast<uint16_t *>(dyn_md_buf_.get());

    for (size_t i = 0; i < total_bus_num_ * std::tuple_size<BusDesc>{};
            i += std::tuple_size<BusDesc>{}) {
        buses.emplace_back(bus_desc_entry[i],
                           bus_desc_entry[i + 1], bus_desc_entry[i + 2]);
    }

    off_ += bus_meta_len;
    bytes_read_ += bus_meta_len;

    return buses;
}

std::vector<DeviceDesc>
SnapshotProvider::GetPCIDevDescriptors()
{
    if (!SnapshotParsePrepare())
        throw std::runtime_error("Invalid snapshot metadata");

    auto parse_error = []() { throw std::runtime_error("Failed to parse snapshot"); };
    std::vector<DeviceDesc> devices;

    do {
        // read device static metadata
        logger.log(Verbosity::INFO,
                   "snapshot: Reading device [{} / {}] static metadata, off {}",
                   cur_dev_num_, total_dev_num_, off_);

        //meta::SDeviceMd dev_static_meta;
        uint8_t buffer[sizeof(meta::SDeviceMd)];
        auto dev_static_meta = reinterpret_cast<const meta::SDeviceMd *>(&buffer);

        auto res = pread(fd_, buffer, sizeof(buffer), off_);
        if (res < 0) {
            logger.log(Verbosity::FATAL,
                       "snapshot: Failed to read device [{} / {}] metadata, err {}",
                       cur_dev_num_, total_dev_num_, errno);
            parse_error();
        } else if ((uint64_t)res != sizeof(buffer)) {
            logger.log(Verbosity::FATAL,
                       "snapshot: Device [{} / {}] metadata header has not been fully read [{} / {}]",
                        cur_dev_num_, total_dev_num_, res, sizeof(buffer));
            parse_error();
        }

        off_ += sizeof(buffer);
        bytes_read_ += sizeof(buffer);

        auto dom  = dev_static_meta->d_bdf_ >> 24 & 0xffff;
        auto bus  = dev_static_meta->d_bdf_ >> 16 & 0xff;
        auto dev  = dev_static_meta->d_bdf_ >> 8 & 0xff;
        auto func = dev_static_meta->d_bdf_ & 0xff;

        auto cfg_len = dev_static_meta->cfg_space_len_ == 0 ? 256 : 4096;
        auto res_desc_cnt = dev_static_meta->dev_res_len_;
        auto is_last_device = dev_static_meta->is_final_dev_entry_ == 1;

        logger.log(Verbosity::INFO,
                   "snapshot: Parsed dev [{} / {}] -> [{:04x}|{:02x}:{:02x}.{:x}] cfg_len {} res_cnt {} last {}",
                   cur_dev_num_, total_dev_num_, dom, bus, dev, func,
                   cfg_len, res_desc_cnt, is_last_device);

        if ((cur_dev_num_ != total_dev_num_) && is_last_device) {
            logger.log(Verbosity::INFO,
                       "snapshot: Device [{} / {}] marked as last in metadata",
                       cur_dev_num_, total_dev_num_);
            parse_error();
        }

        // prepare dynamic md buffer
        auto dyn_md_size = res_desc_cnt * dev_res_desc_size +
                           dev_static_meta->driver_name_len_;
        dyn_md_buf_.reset(new (std::nothrow) uint8_t[dyn_md_size]);
        if (dyn_md_buf_ == nullptr) {
            logger.log(Verbosity::FATAL, "snapshot: Failed to allocate dyn md buffer");
            parse_error();
        }
        cur_dyn_md_buf_len_ = dyn_md_size;

        // prepare cfg space buffer
        OpaqueBuf dev_cfg_space_buf(new (std::nothrow) uint8_t[cfg_len]);
        if (dev_cfg_space_buf == nullptr) {
            logger.log(Verbosity::FATAL, "snapshot: Failed to allocate cfg space buffer");
            parse_error();
        }

        iovec_ = {};
        iovec_[0].iov_base = dyn_md_buf_.get();
        iovec_[0].iov_len = dyn_md_size;

        iovec_[1].iov_base = dev_cfg_space_buf.get();
        iovec_[1].iov_len = cfg_len;

        auto expected_read_bytes = CurIovecDataLen();

        // try read dyn md and cfg space
        res = preadv(fd_, iovec_.data(), 2, off_);
        if (res < 0) {
            logger.log(Verbosity::FATAL,
                       "snapshot: Failed to read device [{} / {}] dyn md + cfg buffer, err {}",
                       cur_dev_num_, total_dev_num_, errno);
            parse_error();
        } else if ((uint64_t)res != expected_read_bytes) {
            logger.log(Verbosity::FATAL,
                       "snapshot: Device [{} / {}] dyn md and cfg buffer have not been fully read [{} / {}]",
                        cur_dev_num_, total_dev_num_, res, expected_read_bytes);
            parse_error();
        }

        off_ += expected_read_bytes;
        bytes_read_ += expected_read_bytes;

        // parse dyn md

        // 1. unpack resources from dynamic md buffer
        std::vector<DevResourceDesc> dev_resources;
        auto res_desc = reinterpret_cast<uint64_t *>(dyn_md_buf_.get());

        for (size_t i = 0; i < res_desc_cnt * std::tuple_size<DevResourceDesc>{};
                i += std::tuple_size<DevResourceDesc>{}) {
            dev_resources.emplace_back(res_desc[i], res_desc[i + 1], res_desc[i + 2]);
        }

        // 2. get driver name
        std::string drv_name {};
        if (dev_static_meta->driver_name_len_ != 0) {
            auto drv_name_off = res_desc_cnt * std::tuple_size<DevResourceDesc>{} *
                                sizeof(uint64_t);
            auto drv_name_buf = reinterpret_cast<char *>(dyn_md_buf_.get() + drv_name_off);
            drv_name = std::string(drv_name_buf);
        }

        // construct @DeviceDesc
        devices.emplace_back(dev_static_meta->d_bdf_,
                             cfg_len,
                             std::move(dev_cfg_space_buf),
                             std::move(dev_resources),
                             std::move(drv_name),
                             dev_static_meta->numa_node_,
                             dev_static_meta->iommu_group_,
                             // FIXME: it's not clear what's the point of passing
                             // dyn md buffer to @PciDevBase
                             nullptr);

        cur_dev_num_ += 1;
    } while (cur_dev_num_ <= total_dev_num_);

    return devices;
}

} // namespace snapshot
