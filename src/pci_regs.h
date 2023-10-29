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
    io_lim_upper     = 0x32, // I/O Limit (upper 16 bits) - 2 bytes

    cap_ptr          = 0x34, // Capabilities Pointer - 1 byte
    // 0x35 - 0x37 - reserved
    exp_rom_bar      = 0x38, // Expansion ROM Base Address - 4 bytes

    itr_line         = 0x3c, // Interrupt Line - 1 byte
    itr_pin          = 0x3d, // Interrupt Pin - 1 byte

    bridge_ctl       = 0x3e // Bridge Control - 2 bytes
};

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
    case Type1Cfg::io_lim_upper:
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
