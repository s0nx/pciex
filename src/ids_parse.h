// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <unordered_map>
#include <span>
#include <cstdint>
#include <string_view>
#include <memory>

#include "util.h"
#include "log.h"

extern Logger logger;

namespace pci {

struct IdParseEx : public CommonEx
{
    using CommonEx::CommonEx;
};

/* Cached PCI device db entry of particular vendor */
struct CachedDbDevEntry
{
    std::string_view device_name_;
    off_t            device_db_off_;
};

/* Cached PCI vendor db entry */
struct CachedDbVendorEntry
{
    std::string_view vendor_name_;
    off_t            vendor_db_off_;
    std::unordered_map<uint16_t, CachedDbDevEntry> devs_;

    CachedDbVendorEntry(std::string_view &vendor_name, off_t db_off) :
        vendor_name_(vendor_name), vendor_db_off_(db_off)
    {
        devs_.clear();
    }
};

constexpr char pci_ids_db_path[] { "/usr/share/hwdata/pci.ids" };

//                 class name,       subclass name,    programming interface
typedef std::tuple<std::string_view, std::string_view, std::string_view> ClassCodeInfo;

struct PciIdParser
{
    size_t                  db_size_ {0};
    std::string_view	    db_str_;
    // position of class/subclass/programming interface block within @db_str_
    off_t                   class_code_db_off_ {0};
    std::unique_ptr<char[]> buf_;

    std::unordered_map<uint16_t, CachedDbVendorEntry> ids_cache_;

    PciIdParser();
    std::string_view vendor_name_lookup(const uint16_t vid);
    std::string_view device_name_lookup(const uint16_t vid, const uint16_t dev_id);
    std::string_view subsys_name_lookup(const uint16_t vid, const uint16_t dev_id,
                                        const uint16_t subsys_vid, const uint16_t subsys_id);
    ClassCodeInfo class_info_lookup(const uint32_t ccode);
};

} /* namespace pci */
