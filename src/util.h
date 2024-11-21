// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <algorithm>
#include <vector>
#include <cstdint>
#include <string_view>
#include <cassert>

// Explicit scoped enums to underlying type conversion
template <typename E>
constexpr auto e_to_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

// Constexpr map
template <typename Key, typename Val, std::size_t Size>
struct CTMap
{
    std::array<std::pair<Key, Val>, Size> arr_;

    [[nodiscard]] constexpr Val at(const Key &key) const
    {
        const auto it = std::find_if(std::begin(arr_), std::end(arr_),
                                     [&key](const auto &v) { return v.first == key; });
        // Since this map is supposed to be used with enums, value should definitely be found
        assert(it != std::end(arr_));
        return it->second;
    }
};

namespace vm {

constexpr int pg_size = 4096;
constexpr std::string_view VmallocInfoFile { "/proc/vmallocinfo" };

/* This object repesents a struct vm_struct of the Linux kernel */
struct VmallocEntry
{
    uint64_t start_;
    uint64_t end_;
    uint64_t len_;
    uint64_t pa_;
};

class VmallocStats
{
private:
    std::vector<VmallocEntry> vm_entries_;
    bool                      vm_info_available_ {false};

public:
    void AddEntry(const VmallocEntry &entry);
    void DumpStats();
    void Parse();
    bool InfoAvailable() { return vm_info_available_; }
    std::vector<VmallocEntry> GetMappingInRange(uint64_t start, uint64_t end);
};

} /* namespace vm */


namespace sys {

/*
 * Due to the fact that `%pK` format specifier is being used to print
 * the virtual address range, `kptr_restrict` sysctl parameter MUST be set to 1.
 * Otherwise we would get hashed addresses.
 * See Documentation/admin-guide/sysctl/kernel.rst doc in the Linux sources
 * This is needed for VmallocStats::parse().
 */
enum class kptr_mode
{
    HASHED = 0,
    REAL_ADDR,
    HIDDEN
};

constexpr std::string_view KptrSysPath {"/proc/sys/kernel/kptr_restrict"};
bool IsKptrSet();

} /* namespace sys */
