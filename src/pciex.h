#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <utility>
#include <regex>

#include <fmt/printf.h>

#include "log.h"
#include "util.h"
#include "pci_regs.h"

extern Logger logger;

namespace fs = std::filesystem;

namespace vm {

constexpr int pg_size = 4096;

/* This object repesents a struct vm_struct of the Linux kernel */
struct VmallocEntry
{
    uint64_t start;
    uint64_t end;
    uint64_t len;
    uint64_t pa;
};

class VmallocStats
{
private:
    std::string_view vmallocinfo {"/proc/vmallocinfo"};
    std::vector<VmallocEntry> vm_info;

public:
    void add_entry(const VmallocEntry &entry);
    void dump_stats();
    void parse();
    void get_mapping_in_range(uint64_t start, uint64_t end);
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

constexpr char kptr_path[] {"/proc/sys/kernel/kptr_restrict"};
bool is_kptr_set();

} /* namespace sys */

namespace pci {

struct CfgEx : public CommonEx
{
    using CommonEx::CommonEx;
};

enum cfg_space_type
{
    LEGACY,
    ECS
};

constexpr uint32_t pci_cfg_legacy_len = 256;
constexpr uint32_t pci_cfg_ecs_len    = 4096;

constexpr char pci_dev_path[] { "/sys/bus/pci/devices" };

// Type 0 / Type 1 device
struct PciDevice
{
    uint16_t dom_;
    uint8_t  bus_;
    uint8_t  dev_;
    uint8_t  func_;
    // Extended Configuration Space
    cfg_space_type  cfg_type_;

    std::unique_ptr<uint8_t[]> cfg_space_;

    PciDevice() = delete;
    PciDevice(uint16_t dom, uint8_t bus, uint8_t dev, uint8_t func,
               cfg_space_type type, std::unique_ptr<uint8_t[]> cfg);

    uint16_t get_vendor_id() const noexcept;
    uint16_t get_dev_id() const noexcept;
    uint32_t get_class_code() const noexcept;
    void print_data() const noexcept;
};

struct pci_bus
{
    uint32_t bus_nr;
    std::vector<std::unique_ptr<PciDevice>> devices;
};

struct pci_domain
{
    uint16_t num;
    std::vector<pci_bus> buses;
};

struct PCITopologyCtx
{
    std::vector<PciDevice> devs_;

    void populate();
    void dump_data() const noexcept;
};

} // namespace pci
