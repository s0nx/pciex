// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include "pciex.h"

void vm::VmallocStats::add_entry(const VmallocEntry &entry)
{
    vm_info.push_back(entry);
}

void vm::VmallocStats::dump_stats()
{
    logger.info("vmalloc stats dump: >>>");
    for (std::size_t i = 0; const auto &elem : vm_info)
    {
        logger.raw("#{} ::> [ >{:#x} - {:#x}< len: {:#x} pa: {:#x} ]",
                   i++, elem.start, elem.end, elem.len, elem.pa);
    }
}

// Find VA space range(s) the physical address space [pa_start, pa_end]
// is mapped into.
// It is possible that only a fraction of the physical address space is mapped.
// TODO: use interval tree instead?
void vm::VmallocStats::get_mapping_in_range(uint64_t pa_start, uint64_t pa_end)
{
    std::vector<VmallocEntry> result;

    auto lb = std::ranges::lower_bound(vm_info, pa_start, std::less<>{}, &VmallocEntry::pa);
    auto ub = std::ranges::upper_bound(vm_info, pa_end, std::less<>{}, &VmallocEntry::pa);
    std::ranges::copy(lb, ub, std::back_inserter(result));

    if (!result.empty()) {
        logger.info("Found VA mapping for PA range [{:#x} - {:#x}]:", pa_start, pa_end);
        std::ranges::for_each(result, [](const auto &n) {
                logger.raw("VA [{:#x} - {:#x}] len {:#x}", n.start, n.end, n.len); });
    }
}

// Parse /proc/vmallocinfo in order to know how exactly a portion of
// physical address space assigned to the particular PCI device is
// remapped into the kernel virtual address space.
//
// NOTE: vmalloc allocations in the Linux kernel use guard pages by default
// to capture illegal out-of-bound accesses unless `VM_NO_GUARD` flag is set.
// This flag is not set for ioremap, so the reported VA range length
// should be interpreted as (VA end - VA start - PAGE_SIZE).
// See mm/vmalloc.c: __get_vm_area_node() for details.
void vm::VmallocStats::parse()
{
    std::ifstream proc_vminfo(vmallocinfo.data(), std::ios::in);
    if (!proc_vminfo.is_open()) {
        logger.err("Failed to open /proc/vmallocinfo");
        return;
    }

    std::string mapping_entry;
    while (std::getline(proc_vminfo, mapping_entry)) {

        // Not interested in non-ioremap allocations for now
        if (!mapping_entry.ends_with("ioremap"))
            continue;

        VmallocEntry entry;
        std::string::size_type s_pos = 0, e_pos;

        // VA start
        e_pos = mapping_entry.find("-");
        entry.start = std::stoull(mapping_entry.substr(s_pos, e_pos - s_pos),
                                  nullptr, 16);
        // VA end
        s_pos = e_pos + 1;
        e_pos = mapping_entry.find(" ");
        entry.end = std::stoull(mapping_entry.substr(s_pos, e_pos - s_pos),
                                nullptr, 16);
        entry.end -= vm::pg_size;
        entry.len = entry.end - entry.start;

        // phys addr
        s_pos = mapping_entry.find("=", e_pos) + 1;
        e_pos = mapping_entry.find(" ", s_pos);
        entry.pa = std::stoull(mapping_entry.substr(s_pos, e_pos - s_pos),
                               nullptr, 16);
        vm_info.push_back(std::move(entry));
    }

    std::ranges::sort(vm_info, [](const auto &a, const auto &b) {
                      return a.pa < b.pa; });
}

bool sys::is_kptr_set()
{
    int val;
    std::ifstream ist(kptr_path, std::ios::in);
    if (!ist.is_open()) {
        logger.err("Unable to check 'kptr_restrict' setting");
        return false;
    } else {
        ist >> val;
    }

    if (val == e_to_type(kptr_mode::REAL_ADDR))
        return true;

    logger.warn("kptr_restrict -> {}: VA mapping info is unavalible", val);
    return false;
}









