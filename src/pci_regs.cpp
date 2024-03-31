// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include <string>
#include <bitset>
#include <fmt/core.h>

#include "pci_regs.h"

std::string LinkSpeedDesc(LinkSpeedRepType rep_type, const uint8_t link_speed,
                          const RegLinkCap2 &link_cap2)
{
    if (link_speed < 0x1 || link_speed > 0x7)
        return "< rsvd encoding >";

    const uint8_t tgt_vt_bit = link_speed - 1;

    std::bitset<4> supported_link_speed_vt {link_cap2.supported_speed_vec};
    if (tgt_vt_bit > (supported_link_speed_vt.size() - 1))
        return fmt::format(" < reserved Link Speeds Vector bit pos ({})>", tgt_vt_bit);

    if (!supported_link_speed_vt[tgt_vt_bit])
        return fmt::format(" [{}] {} link speed is not supported by port",
                           tgt_vt_bit, LinkSpeedBitDesc(tgt_vt_bit));
    else
        return fmt::format(" {} link speed: {}",
                           rep_type == LinkSpeedRepType::current ? "Current" :
                           rep_type == LinkSpeedRepType::max ? "Maximum" :
                           "Target",
                           LinkSpeedBitDesc(tgt_vt_bit));
}

std::string CmplTimeoutRangesDesc(const RegDevCap2 &dev_cap2)
{
    std::bitset<4> ranges_supported {dev_cap2.cmpl_timeout_rng_support};
    if (ranges_supported.none())
        return "[ cmpl timeout not supported ]";
    else
        return fmt::format("A(50us-10ms)[{}] B(10ms-250ms)[{}] C(250ms-4s)[{}] D(4s-64s)[{}]",
                           ranges_supported[0] ? '+' : '-',
                           ranges_supported[1] ? '+' : '-',
                           ranges_supported[2] ? '+' : '-',
                           ranges_supported[3] ? '+' : '-');
}

std::string SuppLinkSpeedDesc(const uint8_t link_speed_vector)
{
    std::bitset<4> link_speed_supported {link_speed_vector};
    return fmt::format("2.5GT/s[{}] 5GT/s[{}] 8GT/s[{}] 16GT/s[{}]",
                       link_speed_supported[0] ? '+' : '-',
                       link_speed_supported[1] ? '+' : '-',
                       link_speed_supported[2] ? '+' : '-',
                       link_speed_supported[3] ? '+' : '-');
}
