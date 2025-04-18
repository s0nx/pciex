// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024-2025 Petr Vyazovik <xen@f-m.fm>

#include <bitset>
#include <format>

#include "compat_cap_comp.h"
#include "log.h"
#include "util.h"
#include "virtio_regs.h"

extern Logger logger;

using namespace ftxui;

namespace ui {

static capability_comp_ctx
CompatVendorSpecCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                    std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 1;
    size_t i = vis.size();

    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto vspec = reinterpret_cast<const CompatCapVendorSpec *>(dev->cfg_space_.get() + off);
    auto vspec_buf = reinterpret_cast<const uint8_t *>(dev->cfg_space_.get() + off + sizeof(*vspec));

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Vendor-Specific", &vis[i]),
                        CapHdrComp(vspec->hdr)
                    }));

    auto buf_dump_elem = GetHexDumpElem(std::format("data [len {:#02x}] >>>", vspec->len),
                                        vspec_buf, vspec->len);
    Elements content_elems {
        buf_dump_elem
    };

    auto vid = dev->get_vendor_id();
    auto dev_id = dev->get_device_id();

    if (virtio::is_virtio_dev(vid, dev_id)) {
        // show additional info for modern virtio devices only
        if (virtio::is_virtio_modern(dev_id)) {
            auto virtio_struct = reinterpret_cast<const virtio::VirtIOPCICap *>
                                 (dev->cfg_space_.get() + off);
            if (virtio_struct->cfg_type > e_to_type(virtio::VirtIOCapID::cap_id_max)) {
                logger.log(Verbosity::WARN, "{}: unexpected virtio cfg type ({}) in vendor spec cap (off {:02x})",
                            dev->dev_id_str_, virtio_struct->cfg_type, off);
            } else {
                content_elems.push_back(separatorEmpty());
                auto vhdr = text("[VirtIO]") | bold | bgcolor(Color::Blue) | color(Color::Grey15);
                content_elems.push_back(hbox({std::move(vhdr), separatorEmpty()}));

                virtio::VirtIOCapID cap_id {virtio_struct->cfg_type};
                content_elems.push_back(text(std::format("struct type: [{:#01x}] {}",
                                                         virtio_struct->cfg_type,
                                                         virtio::VirtIOCapIDName(cap_id))));
                // "PCI conf access" layout (0x5) can't be mapped by BAR.
                // It's an alternative access method to conf regions
                if (cap_id != virtio::VirtIOCapID::pci_cfg_acc) {
                    content_elems.push_back(text(std::format("        BAR:  {:#01x}", virtio_struct->bar_idx)));
                    content_elems.push_back(text(std::format("         id:  {:#02x}", virtio_struct->id)));
                    content_elems.push_back(text(std::format(" BAR offset:  {:#x}", virtio_struct->bar_off)));
                    content_elems.push_back(text(std::format(" struct len:  {:#x}", virtio_struct->length)));
                }
            }
        }
    }

    auto content_elem = vbox(content_elems);

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] Vendor-Specific", off),
                                     "Info", std::move(content_elem), &vis[i]));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
CompatPMCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
            std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto pm_cap = reinterpret_cast<const PciPMCap *>(dev->cfg_space_.get() + off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("PM Capabilities +0x2", &vis[i++]),
                        CapHdrComp(pm_cap->hdr)
                    }));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("PM Ctrl/Status +0x4", &vis[i++])
                    }));

    std::array<uint16_t, 8> aux_max_current {0, 55, 100, 160, 220, 270, 320, 375};
    std::bitset<5> pme_state_map {pm_cap->pmcap.pme_support};

    auto pm_cap_content = vbox({
        RegFieldVerbElem(0,  2,  std::format(" version: {}", pm_cap->pmcap.version),
                                 pm_cap->pmcap.version),
        RegFieldCompElem(3,  3,  " PME clock", pm_cap->pmcap.pme_clk == 1),
        RegFieldCompElem(4,  4,  " imm ready on D0", pm_cap->pmcap.imm_readiness_on_ret_d0 == 1),
        RegFieldCompElem(5,  5,  " device specific init", pm_cap->pmcap.dsi == 1),
        RegFieldVerbElem(6,  8,  std::format(" aux current: {} mA",
                                 aux_max_current[pm_cap->pmcap.aux_cur]),
                                 pm_cap->pmcap.aux_cur),
        RegFieldCompElem(9,  9,  " D1 state support", pm_cap->pmcap.d1_support == 1),
        RegFieldCompElem(10, 10, " D2 state support", pm_cap->pmcap.d2_support == 1),
        RegFieldVerbElem(11, 15,
                         std::format(" PME support: D0[{}] D1[{}] D2[{}] D3hot[{}] D3cold[{}]",
                                     pme_state_map[0] ? '+' : '-',
                                     pme_state_map[1] ? '+' : '-',
                                     pme_state_map[2] ? '+' : '-',
                                     pme_state_map[3] ? '+' : '-',
                                     pme_state_map[4] ? '+' : '-'),
                         pm_cap->pmcap.pme_support)
    });

    auto pm_ctrl_stat_content = vbox({
        RegFieldVerbElem(0,  1,   std::format(" power state: D{}", pm_cap->pmcs.pwr_state),
                                  pm_cap->pmcs.pwr_state),
        RegFieldCompElem(2,  2),
        RegFieldCompElem(3,  3,   " no soft reset", pm_cap->pmcs.no_soft_reset == 1),
        RegFieldCompElem(4,  7),
        RegFieldCompElem(8,  8,   " PME generation enable", pm_cap->pmcs.pme_en == 1),
        RegFieldVerbElem(9,  12,  " data select", pm_cap->pmcs.data_select),
        RegFieldVerbElem(13, 14,  " data scale", pm_cap->pmcs.data_scale),
        RegFieldCompElem(15, 15,  " PME status", pm_cap->pmcs.pme_status == 1),
        RegFieldCompElem(16, 23),
        RegFieldVerbElem(24, 31,  " data", pm_cap->pmcs.data)
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] Power Management", off),
                                     "PM Capabilities +0x2", std::move(pm_cap_content), &vis[i - 2]));
    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] Power Management", off),
                                     "PM Ctrl/Status +0x4", std::move(pm_ctrl_stat_content),
                                     &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
CompatMSICap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
             std::vector<uint8_t> &vis)
{
    Components upper, lower;
    //constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), 1, 0);

    auto off = std::get<3>(cap);
    auto msi_cap_hdr = reinterpret_cast<const CompatCapHdr *>(dev->cfg_space_.get() + off);
    auto msi_msg_ctrl_reg = reinterpret_cast<const RegMSIMsgCtrl *>
                            (dev->cfg_space_.get() + off + 0x2);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Message Control +0x2", &vis[i]),
                        CapHdrComp(*msi_cap_hdr)
                    }));

    std::array<uint8_t, 8> multi_msg_map {1, 2, 4, 8, 16, 32, 0, 0};

    auto msi_mc_content = vbox({
        RegFieldCompElem(0,  0, " MSI enable", msi_msg_ctrl_reg->msi_ena == 1),
        RegFieldVerbElem(1,  3, std::format(" multiple msg capable: {}",
                                            multi_msg_map[msi_msg_ctrl_reg->multi_msg_capable]),
                                msi_msg_ctrl_reg->multi_msg_capable),
        RegFieldVerbElem(4,  6, std::format(" multiple msg enable: {}",
                                            multi_msg_map[msi_msg_ctrl_reg->multi_msg_ena]),
                                msi_msg_ctrl_reg->multi_msg_ena),
        RegFieldCompElem(7,  7, " 64-bit address", msi_msg_ctrl_reg->addr_64_bit_capable == 1),
        RegFieldCompElem(8,  8, " per-vector masking", msi_msg_ctrl_reg->per_vector_mask_capable == 1),
        RegFieldCompElem(9,  9, " extended msg capable", msi_msg_ctrl_reg->ext_msg_data_capable == 1),
        RegFieldCompElem(10, 10, " extended msg enable", msi_msg_ctrl_reg->ext_msg_data_ena == 1),
        RegFieldCompElem(11, 15)
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                     "Message Control +0x2", std::move(msi_mc_content), &vis[i++]));

    // Add other components depending on the type of MSI capability
    if (msi_msg_ctrl_reg->addr_64_bit_capable) {
        std::ranges::fill_n(std::back_inserter(vis), 2, 0);
        auto msg_addr_lower = *reinterpret_cast<const uint32_t *>(dev->cfg_space_.get() + off + 0x4);
        auto msg_addr_upper = *reinterpret_cast<const uint32_t *>(dev->cfg_space_.get() + off + 0x8);

        upper.push_back(Container::Horizontal({
                            RegButtonComp("Message Address lower 32 bits +0x4", &vis[i++])
                        }));
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Message Address upper 32 bits +0x8", &vis[i++])
                        }));

        auto laddr_content = vbox({
            hbox({
                hbox({
                    text(std::format("{:030b}", msg_addr_lower)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                    separator(),
                    text(std::format("{:02b}", msg_addr_lower & 0x3)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
                }) | border,
                filler()
            }),
            text(std::format(" Full address: {:#x}",
                 (uint64_t)msg_addr_upper << 32 | (uint64_t)msg_addr_lower))
        });

        auto uaddr_content = vbox({
            hbox({
                hbox({
                    text(std::format("{:032b}", msg_addr_upper)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta)
                }) | border,
                filler()
            }),
            text(std::format(" address: {:#x}", msg_addr_upper))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                         "Message Address +0x4", std::move(laddr_content),
                                         &vis[i - 2]));
        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                         "Message Address Upper +0x8", std::move(uaddr_content),
                                         &vis[i - 1]));
    } else {
        std::ranges::fill_n(std::back_inserter(vis), 1, 0);
        auto msg_addr_lower = *reinterpret_cast<const uint32_t *>(dev->cfg_space_.get() + off + 0x4);
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Message Address +0x4", &vis[i++])
                        }));

        auto laddr_content = vbox({
            hbox({
                hbox({
                    text(std::format("{:030b}", msg_addr_lower)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                    separator(),
                    text(std::format("{:02b}", msg_addr_lower & 0x3)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
                }) | border,
                filler()
            }),
            text(std::format(" Address: {:#x}", msg_addr_lower))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                         "Message Address +0x4", std::move(laddr_content),
                                         &vis[i - 1]));
    }

    // (extended) message data
    std::ranges::fill_n(std::back_inserter(vis), 2, 0);

    auto msg_data_off = msi_msg_ctrl_reg->addr_64_bit_capable ? 0xc : 0x8;
    upper.push_back(Container::Horizontal({
                        RegButtonComp(std::format("Extended Message Data +{:#x}", msg_data_off + 0x2),
                                      &vis[i++]),
                        RegButtonComp(std::format("Message Data +{:#x}", msg_data_off),
                                      &vis[i++])
                    }));

    auto msg_data = *reinterpret_cast<const uint16_t *>(dev->cfg_space_.get() + off + msg_data_off);
    auto ext_msg_data = *reinterpret_cast<const uint16_t *>
                        (dev->cfg_space_.get() + off + msg_data_off + 0x2);
    auto data_content = text(std::format("data: {:#x}", msg_data)) | bold;
    auto ext_data_content = text(std::format("extended data: {:#x}", ext_msg_data)) | bold;

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                     std::format("Extended Message Data +{:#x}", msg_data_off + 0x2),
                                     std::move(ext_data_content), &vis[i - 2]));

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                     std::format("Message Data +{:#x}", msg_data_off),
                                     std::move(data_content), &vis[i - 1]));

    // mask/pending bits info
    if (msi_msg_ctrl_reg->per_vector_mask_capable) {
        auto mask_bits_off = msi_msg_ctrl_reg->addr_64_bit_capable ? 0x10 : 0xc;
        auto pending_bits_off = mask_bits_off + 0x4;
        auto mask_bits = *reinterpret_cast<const uint32_t *>
                         (dev->cfg_space_.get() + off + mask_bits_off);
        auto pending_bits = *reinterpret_cast<const uint32_t *>
                         (dev->cfg_space_.get() + off + pending_bits_off);

        std::ranges::fill_n(std::back_inserter(vis), 2, 0);
        upper.push_back(Container::Horizontal({
                            RegButtonComp(std::format("Mask Bits +{:#x}", mask_bits_off),
                                          &vis[i++]),
                        }));
        upper.push_back(Container::Horizontal({
                            RegButtonComp(std::format("Pending Bits +{:#x}", pending_bits_off),
                                          &vis[i++]),
                        }));

        auto mask_bits_content = hbox({
            hbox({
                text(std::format("{:32b}", mask_bits)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta)
            }) | border,
            filler()
        });

        auto pending_bits_content = hbox({
            hbox({
                text(std::format("{:32b}", pending_bits)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta)
            }) | border,
            filler()
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                         std::format("Mask Bits +{:#x}", mask_bits_off),
                                         std::move(mask_bits_content), &vis[i - 2]));

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI", off),
                                         std::format("Pending Bits +{:#x}", pending_bits_off),
                                         std::move(pending_bits_content), &vis[i - 1]));
    }

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
CompatPCIECap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
              std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 22;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto pcie_cap = reinterpret_cast<const PciECap *>(dev->cfg_space_.get() + off);

    // pcie capabilities
    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("PCI Express Capabilities +0x2", &vis[i++]),
                        CapHdrComp(pcie_cap->hdr)
                    }));

    auto pcie_cap_reg_content = vbox({
        RegFieldCompElem(0, 3, std::format(" Version: {}",
                                           pcie_cap->pcie_cap_reg.cap_ver)),
        RegFieldVerbElem(4, 7, std::format(" Device/Port type: '{}'",
                                       dev->type_ == pci::pci_dev_type::TYPE0 ?
                                       PciEDevPortDescType0(pcie_cap->pcie_cap_reg.dev_port_type) :
                                       PciEDevPortDescType1(pcie_cap->pcie_cap_reg.dev_port_type)),
                                pcie_cap->pcie_cap_reg.dev_port_type),
        RegFieldCompElem(8, 8, " Slot implemented", pcie_cap->pcie_cap_reg.slot_impl == 1),
        RegFieldCompElem(9, 13, std::format(" ITR message number: {}",
                                pcie_cap->pcie_cap_reg.itr_msg_num)),
        RegFieldCompElem(14, 15)
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                     "PCIe Capabilities +0x2", std::move(pcie_cap_reg_content),
                                     &vis[i - 1]));

    // device capabilities
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Device Capabilities +0x4", &vis[i++]),
                    }));

    std::array<uint16_t, 8> pyld_sz_map { 128, 256, 512, 1024, 2048, 4096, 0, 0};

    auto dev_caps_content = vbox({
        RegFieldVerbElem(0, 2, std::format(" Max payload size: {}",
                                           pyld_sz_map[pcie_cap->dev_cap.max_pyld_size_supported]),
                                pcie_cap->dev_cap.max_pyld_size_supported),
        RegFieldCompElem(3, 4, std::format(" Phantom functions: MSB num {:02b} | {}",
                                           pcie_cap->dev_cap.phan_func_supported,
                                           pcie_cap->dev_cap.phan_func_supported)),
        RegFieldCompElem(5, 5, " Ext tag field supported",
                         pcie_cap->dev_cap.ext_tag_field_supported == 1),
        RegFieldCompElem(6, 8, std::format(" EP L0s acceptable latency: {}",
                                           EpL0sAcceptLatDesc(pcie_cap->dev_cap.ep_l0s_accept_lat))),
        RegFieldCompElem(9, 11, std::format(" EP L1 acceptable latency: {}",
                                            EpL1AcceptLatDesc(pcie_cap->dev_cap.ep_l1_accept_lat))),
        RegFieldCompElem(12, 14),
        RegFieldCompElem(15, 15, " Role-based error reporting",
                         pcie_cap->dev_cap.role_based_err_rep == 1),
        RegFieldCompElem(16, 17),
        RegFieldCompElem(18, 25, std::format(" Captured slot power limit: {:#x}",
                                             pcie_cap->dev_cap.cap_slot_pwr_lim_val)),
        RegFieldVerbElem(26, 27, std::format(" Captured slot power scale: {}",
                                        CapSlotPWRScale(pcie_cap->dev_cap.cap_slot_pwr_lim_scale)),
                        pcie_cap->dev_cap.cap_slot_pwr_lim_scale),
        RegFieldCompElem(28, 28, " FLR capable", pcie_cap->dev_cap.flr_cap == 1),
        RegFieldCompElem(29, 31)
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                     "Device Capabilities +0x4", std::move(dev_caps_content),
                                     &vis[i - 1]));

    // device control / status
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Device Status +0xa", &vis[i++]),
                        RegButtonComp("Device Control +0x8", &vis[i++]),
                    }));

    auto dev_ctrl_content = vbox({
        RegFieldCompElem(0, 0, " Correctable error reporting",
                         pcie_cap->dev_ctl.correct_err_rep_ena == 1),
        RegFieldCompElem(1, 1, " Non-fatal error reporting",
                         pcie_cap->dev_ctl.non_fatal_err_rep_ena == 1),
        RegFieldCompElem(2, 2, " Fatal error reporting",
                         pcie_cap->dev_ctl.fatal_err_rep_ena == 1),
        RegFieldCompElem(3, 3, " Unsupported request reporting",
                         pcie_cap->dev_ctl.unsupported_req_rep_ena == 1),
        RegFieldCompElem(4, 4, " Relaxed ordering",
                         pcie_cap->dev_ctl.relaxed_order_ena == 1),
        RegFieldCompElem(5, 7, std::format(" Max TLP payload size: {} bytes",
                                           pyld_sz_map[pcie_cap->dev_ctl.max_pyld_size])),
        RegFieldCompElem(8, 8, " Extended tag field",
                         pcie_cap->dev_ctl.ext_tag_field_ena == 1),
        RegFieldCompElem(9, 9, " Phantom functions",
                         pcie_cap->dev_ctl.phan_func_ena == 1),
        RegFieldCompElem(10, 10, " Aux power PM",
                         pcie_cap->dev_ctl.aux_power_pm_ena == 1),
        RegFieldCompElem(11, 11, " No snoop",
                         pcie_cap->dev_ctl.no_snoop_ena == 1),
        RegFieldCompElem(12, 14, std::format(" max READ request size: {} bytes",
                                           pyld_sz_map[pcie_cap->dev_ctl.max_read_req_size])),
        RegFieldCompElem(15, 15, std::format("{}",
                                 (dev->type_ == pci::pci_dev_type::TYPE1 &&
                                  pcie_cap->pcie_cap_reg.dev_port_type == 0b0111) ?
                                 " Bridge configuration retry" :
                                 (dev->type_ == pci::pci_dev_type::TYPE0 &&
                                  pcie_cap->pcie_cap_reg.dev_port_type != 0b1010) ?
                                 " Initiate FLR" :
                                 " - "),
                         pcie_cap->dev_ctl.brd_conf_retry_init_flr == 1)
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                     "Device Control +0x8", std::move(dev_ctrl_content),
                                     &vis[i - 1]));

    auto dev_status_content = vbox({
        RegFieldCompElem(0, 0, " Correctable error detected",
                         pcie_cap->dev_status.corr_err_detected == 1),
        RegFieldCompElem(1, 1, " Non-fatal error detected",
                         pcie_cap->dev_status.non_fatal_err_detected == 1),
        RegFieldCompElem(2, 2, " Fatal error detected",
                         pcie_cap->dev_status.fatal_err_detected == 1),
        RegFieldCompElem(3, 3, " Unsupported request detected",
                         pcie_cap->dev_status.unsupported_req_detected == 1),
        RegFieldCompElem(4, 4, " Aux power detected",
                         pcie_cap->dev_status.aux_pwr_detected == 1),
        RegFieldCompElem(5, 5, " Transaction pending",
                         pcie_cap->dev_status.trans_pending == 1),
        RegFieldCompElem(6, 6, " Emergency power reduction detected",
                         pcie_cap->dev_status.emerg_pwr_reduct_detected == 1),
        RegFieldCompElem(7, 15)
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                     "Device Status +0xa", std::move(dev_status_content),
                                     &vis[i - 2]));

    // link capabilities
    auto link_cap = reinterpret_cast<const uint32_t *>(&pcie_cap->link_cap);
    if (*link_cap != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Link Capabilities +0xc", &vis[i++]),
                        }));

        auto link_cap_content = vbox({
            RegFieldVerbElem(0, 3, LinkSpeedDesc(LinkSpeedRepType::max,
                                                 pcie_cap->link_cap.max_link_speed,
                                                 pcie_cap->link_cap2),
                             pcie_cap->link_cap.max_link_speed),
            RegFieldVerbElem(4, 9, std::format(" Max link width: {}",
                                               LinkWidthDesc(pcie_cap->link_cap.max_link_width)),
                             pcie_cap->link_cap.max_link_width),
            RegFieldCompElem(10, 11,
                             std::format(" ASPM support [{}]: L0s[{}] L1[{}]",
                                         pcie_cap->link_cap.aspm_support ? '+' : '-',
                                         (pcie_cap->link_cap.aspm_support & 0x1) ? '+' : '-',
                                         (pcie_cap->link_cap.aspm_support & 0x2) ? '+' : '-')),
            RegFieldVerbElem(12, 14, std::format(" L0s exit latency: {}",
                                                LinkCapL0sExitLat(pcie_cap->link_cap.l0s_exit_lat)),
                             pcie_cap->link_cap.l0s_exit_lat),
            RegFieldVerbElem(15, 17, std::format(" L1 exit latency: {}",
                                                LinkCapL1ExitLat(pcie_cap->link_cap.l1_exit_lat)),
                             pcie_cap->link_cap.l1_exit_lat),
            RegFieldCompElem(18, 18, " Clock PM", pcie_cap->link_cap.clk_pwr_mng == 1),
            RegFieldCompElem(19, 19, " Suprise down err reporting",
                             pcie_cap->link_cap.surpr_down_err_rep_cap == 1),
            RegFieldCompElem(20, 20, " Data link layer active reporting",
                             pcie_cap->link_cap.dlink_layer_link_act_rep_cap == 1),
            RegFieldCompElem(21, 21, " Link BW notification",
                             pcie_cap->link_cap.link_bw_notify_cap == 1),
            RegFieldCompElem(22, 22, " ASPM opt compliance",
                             pcie_cap->link_cap.aspm_opt_compl == 1),
            RegFieldCompElem(23, 31, std::format(" Port number: {}", pcie_cap->link_cap.port_num))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Capabilities +0xc", std::move(link_cap_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Link Capabilities +0xc")
                        }));
    }

    // link status / control
    auto lctrl_stat_dw = reinterpret_cast<const uint32_t *>(&pcie_cap->link_ctl);
    if (*lctrl_stat_dw != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Link Status +0x12", &vis[i++]),
                            RegButtonComp("Link Control +0x10", &vis[i++]),
                        }));

        auto link_ctrl_content = vbox({
            RegFieldCompElem(0, 1,
                             std::format(" ASPM ctrl [{}]: L0s[{}] L1[{}]",
                                         pcie_cap->link_ctl.aspm_ctl ? "+" : "disabled",
                                         (pcie_cap->link_ctl.aspm_ctl & 0x1) ? '+' : '-',
                                         (pcie_cap->link_ctl.aspm_ctl & 0x2) ? '+' : '-')),
            RegFieldCompElem(2, 2),
            RegFieldCompElem(3, 3, std::format(" RCB: {}",
                                   (dev->type_ == pci::pci_dev_type::TYPE1 &&
                                     (pcie_cap->pcie_cap_reg.dev_port_type == 0b0101 ||
                                      pcie_cap->pcie_cap_reg.dev_port_type == 0b0110)) ?
                                   "0 (not applicable)" :
                                   (pcie_cap->link_ctl.rcb == 0) ? "64 b" : "128 b")),
            RegFieldCompElem(4, 4, " Link disable", pcie_cap->link_ctl.link_disable == 1),
            RegFieldCompElem(5, 5, " Retrain link", pcie_cap->link_ctl.retrain_link == 1),
            RegFieldCompElem(6, 6, " Common clock configuration",
                             pcie_cap->link_ctl.common_clk_conf == 1),
            RegFieldCompElem(7, 7, " Extended synch",
                             pcie_cap->link_ctl.ext_synch == 1),
            RegFieldCompElem(8, 8, " Clock PM",
                             pcie_cap->link_ctl.clk_pm_ena == 1),
            RegFieldCompElem(9, 9, " HW autonomous width disable",
                             pcie_cap->link_ctl.hw_auto_width_disable == 1),
            RegFieldCompElem(10, 10, " Link BW mgmt itr",
                             pcie_cap->link_ctl.link_bw_mng_itr_ena == 1),
            RegFieldCompElem(11, 11, " Link autonomous BW itr",
                             pcie_cap->link_ctl.link_auto_bw_mng_itr_ena == 1),
            RegFieldCompElem(12, 13),
            RegFieldCompElem(14, 15, std::format(" DRS: {}",
                                     LinkCtlDrsSigCtlDesc(pcie_cap->link_ctl.drs_signl_ctl)))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Control +0x10", std::move(link_ctrl_content),
                                         &vis[i - 1]));

        auto link_status_content = vbox({
            RegFieldVerbElem(0, 3, LinkSpeedDesc(LinkSpeedRepType::current,
                                                 pcie_cap->link_status.curr_link_speed,
                                                 pcie_cap->link_cap2),
                             pcie_cap->link_status.curr_link_speed),
            RegFieldVerbElem(4, 9, std::format(" Negotiated link width: {}",
                                               LinkWidthDesc(pcie_cap->link_status.negotiated_link_width)),
                             pcie_cap->link_status.negotiated_link_width),
            RegFieldCompElem(10, 10),
            RegFieldCompElem(11, 11, " Link training",
                             pcie_cap->link_status.link_training == 1),
            RegFieldCompElem(12, 12, " Slot clock conf",
                             pcie_cap->link_status.slot_clk_conf == 1),
            RegFieldCompElem(13, 13, " Data link layer link active",
                             pcie_cap->link_status.slot_clk_conf == 1),
            RegFieldCompElem(14, 14, " Link BW mgmt status",
                             pcie_cap->link_status.link_bw_mng_status == 1),
            RegFieldCompElem(15, 15, " Link autonomous BW status",
                             pcie_cap->link_status.link_auto_bw_status == 1)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Status +0x12", std::move(link_status_content),
                                         &vis[i - 2]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Link Status +0x12"),
                            EmptyCapRegComp("Link Control +0x10")
                        }));
    }

    // slot capabilities
    auto slot_cap = reinterpret_cast<const uint32_t *>(&pcie_cap->slot_cap);
    if (*slot_cap != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Slot Capabilities +0x14", &vis[i++]),
                        }));
        auto slot_cap_content = vbox({
            RegFieldCompElem(0, 0, " Attention button present",
                             pcie_cap->slot_cap.attn_btn_pres == 1),
            RegFieldCompElem(1, 1, " Power controller present",
                             pcie_cap->slot_cap.pwr_ctl_pres == 1),
            RegFieldCompElem(2, 2, " MRL sensor present",
                             pcie_cap->slot_cap.mrl_sens_pres == 1),
            RegFieldCompElem(3, 3, " Attention indicator present",
                             pcie_cap->slot_cap.attn_ind_pres == 1),
            RegFieldCompElem(4, 4, " Power indicator present",
                             pcie_cap->slot_cap.pwr_ind_pres == 1),
            RegFieldCompElem(5, 5, " HP surprise",
                             pcie_cap->slot_cap.hot_plug_surpr == 1),
            RegFieldCompElem(6, 6, " HP capable",
                             pcie_cap->slot_cap.hot_plug_cap == 1),
            RegFieldVerbElem(7, 14, std::format(" Slot PL value: {}",
                                    SlotCapPWRLimitDesc(pcie_cap->slot_cap.slot_pwr_lim_val)),
                             pcie_cap->slot_cap.slot_pwr_lim_val),
            RegFieldCompElem(15, 16, std::format(" Slot PL scale: {}",
                                      CapSlotPWRScale(pcie_cap->slot_cap.slot_pwr_lim_val))),
            RegFieldCompElem(17, 17, " EM interlock present",
                             pcie_cap->slot_cap.em_interlock_pres == 1),
            RegFieldCompElem(18, 18, " No command completed",
                             pcie_cap->slot_cap.no_cmd_cmpl_support == 1),
            RegFieldCompElem(19, 31, std::format(" Physical slot number: {:#x}",
                                                 pcie_cap->slot_cap.phys_slot_num))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Slot Capabilities +0x14", std::move(slot_cap_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Slot Capabilities +0x14"),
                        }));
    }

    // slot status / control
    auto slot_stat_ctrl_dw = reinterpret_cast<const uint32_t *>(&pcie_cap->slot_ctl);
    if (*slot_stat_ctrl_dw != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Slot Status +0x1a", &vis[i++]),
                            RegButtonComp("Slot Control +0x18", &vis[i++]),
                        }));

        auto slot_stat_content = vbox({
            RegFieldCompElem(0, 0, " Attention button pressed",
                             pcie_cap->slot_status.attn_btn_pres == 1),
            RegFieldCompElem(1, 1, " Power fault detected",
                             pcie_cap->slot_status.pwr_fault_detected == 1),
            RegFieldCompElem(2, 2, " MRL sensor changed",
                             pcie_cap->slot_status.mrl_sens_changed == 1),
            RegFieldCompElem(3, 3, " Presence detect changed",
                             pcie_cap->slot_status.pres_detect_changed == 1),
            RegFieldCompElem(4, 4, " Cmd completed",
                             pcie_cap->slot_status.cmd_cmpl == 1),
            RegFieldCompElem(5, 5, std::format(" MRL sensor state: {}",
                             pcie_cap->slot_status.mrl_sens_state == 0x0 ? "closed" : "open")),
            RegFieldCompElem(6, 6, std::format(" Presence detect state: {}",
                             pcie_cap->slot_status.pres_detect_state == 0x0 ?
                             "slot empty" : "adapter present")),
            RegFieldCompElem(7, 7, std::format(" EM interlock status: {}",
                             pcie_cap->slot_status.em_interlock_status == 0x0 ?
                             "disengaged" : "engaged")),
            RegFieldCompElem(8, 8, " Data link layer state changed",
                             pcie_cap->slot_status.dlink_layer_state_changed == 1),
            RegFieldCompElem(9, 15)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Slot Status +0x1a", std::move(slot_stat_content),
                                         &vis[i - 2]));

        auto slot_ctrl_content = vbox({
            RegFieldCompElem(0, 0, " Attention button pressed enable",
                             pcie_cap->slot_ctl.attn_btn_pres_ena == 1),
            RegFieldCompElem(1, 1, " Power fault detected enable",
                             pcie_cap->slot_ctl.pwr_fault_detected_ena == 1),
            RegFieldCompElem(2, 2, " MRL sensor changed enable",
                             pcie_cap->slot_ctl.mrl_sens_changed_ena == 1),
            RegFieldCompElem(3, 3, " Presence detect changed enable",
                             pcie_cap->slot_ctl.pres_detect_changed_ena == 1),
            RegFieldCompElem(4, 4, " Cmd completed interrupt enable",
                             pcie_cap->slot_ctl.cmd_cmpl_itr_ena == 1),
            RegFieldCompElem(5, 5, " HP interrupt enable",
                             pcie_cap->slot_ctl.hot_plug_itr_ena == 1),
            RegFieldCompElem(6, 7, std::format(" Attention indicator ctrl: {}",
                             SlotCtlIndCtrlDesc(pcie_cap->slot_ctl.attn_ind_ctl))),
            RegFieldCompElem(8, 9, std::format(" Power indicator ctrl: {}",
                             SlotCtlIndCtrlDesc(pcie_cap->slot_ctl.pwr_ind_ctl))),
            RegFieldCompElem(10, 10, std::format(" Power controller ctrl: {}",
                             pcie_cap->slot_ctl.pwr_ctl_ctl == 0x0 ? "ON" : "OFF")),
            RegFieldCompElem(11, 11, " EM interlock ctrl"),
            RegFieldCompElem(12, 12, " Data link layer state changed enable",
                             pcie_cap->slot_ctl.dlink_layer_state_changed_ena == 1),
            RegFieldCompElem(13, 13, " Auto slot power limit disabled",
                             pcie_cap->slot_ctl.auto_slow_prw_lim_dis == 1),
            RegFieldCompElem(14, 15)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Slot Control +0x18", std::move(slot_ctrl_content),
                                         &vis[i - 1]));

    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Slot Status +0x1a"),
                            EmptyCapRegComp("Slot Control +0x18"),
                        }));
    }

    // root capabilities / control
    auto root_caps_ctrl_dw = reinterpret_cast<const uint32_t *>(&pcie_cap->root_ctl);
    if (*root_caps_ctrl_dw != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Root Capabilities +0x1e", &vis[i++]),
                            RegButtonComp("Root Control +0x1c", &vis[i++]),
                        }));

        auto root_cap_content = vbox({
            RegFieldCompElem(0, 0, " CRS sw visible", pcie_cap->root_cap.crs_sw_vis == 1),
            RegFieldCompElem(1, 15)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Root Capabilities +0x1e", std::move(root_cap_content),
                                         &vis[i - 2]));

        auto root_ctrl_content = vbox({
            RegFieldCompElem(0, 0, " Sys error on correctable err enable",
                             pcie_cap->root_ctl.sys_err_on_correct_err_ena == 1),
            RegFieldCompElem(1, 1, " Sys error on non-fatal err enable",
                             pcie_cap->root_ctl.sys_err_on_non_fat_err_ena == 1),
            RegFieldCompElem(2, 2, " Sys error on fatal err enable",
                             pcie_cap->root_ctl.sys_err_on_fat_err_ena == 1),
            RegFieldCompElem(3, 3, " PME itr enable",
                             pcie_cap->root_ctl.pme_itr_ena == 1),
            RegFieldCompElem(4, 4, "  CRS sw visibility enable",
                             pcie_cap->root_ctl.crs_sw_vis_ena == 1),
            RegFieldCompElem(5, 15)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Root Control +0x1c", std::move(root_ctrl_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Root Capabilities +0x1e"),
                            EmptyCapRegComp("Root Control +0x1c"),
                        }));
    }

    // root status
    auto root_status = reinterpret_cast<const uint32_t *>(&pcie_cap->root_status);
    if (*root_status != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Root Status +0x20", &vis[i++]),
                        }));

        auto root_status_content = vbox({
            RegFieldVerbElem(0, 15, std::format(" PME requester ID: {:#x}",
                                                pcie_cap->root_status.pme_req_id),
                             pcie_cap->root_status.pme_req_id),
            RegFieldCompElem(16, 16, " PME status", pcie_cap->root_status.pme_status),
            RegFieldCompElem(17, 17, " PME pending", pcie_cap->root_status.pme_pending),
            RegFieldCompElem(18, 31)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Root Status +0x20", std::move(root_status_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Root Status +0x20"),
                        }));
    }

    // device capabilities 2
    auto dev_cap_2 = reinterpret_cast<const uint32_t *>(&pcie_cap->dev_cap2);
    if (*dev_cap_2 != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Device Capabilities 2 +0x24", &vis[i++]),
                        }));

        auto dev_cap2_content = vbox({
            RegFieldCompElem(0, 3, std::format(" Cmpl timeout ranges: {}",
                             CmplTimeoutRangesDesc(pcie_cap->dev_cap2))),
            RegFieldCompElem(4, 4, " Cmpl timeout disable",
                             pcie_cap->dev_cap2.cmpl_timeout_dis_support == 1),
            RegFieldCompElem(5, 5, " ARI forwarding",
                             pcie_cap->dev_cap2.ari_fwd_support == 1),
            RegFieldCompElem(6, 6, " AtomicOP routing",
                             pcie_cap->dev_cap2.atomic_op_route_support == 1),
            RegFieldCompElem(7, 7, " 32-bit AtomicOP completer",
                             pcie_cap->dev_cap2.atomic_op_32_cmpl_support == 1),
            RegFieldCompElem(8, 8, " 64-bit AtomicOP completer",
                             pcie_cap->dev_cap2.atomic_op_64_cmpl_support == 1),
            RegFieldCompElem(9, 9, " 128-bit CAS completer",
                             pcie_cap->dev_cap2.cas_128_cmpl_support == 1),
            RegFieldCompElem(10, 10, " No RO-enabled PR-PR passing",
                             pcie_cap->dev_cap2.no_ro_ena_prpr_passing == 1),
            RegFieldCompElem(11, 11, " LTR",
                             pcie_cap->dev_cap2.ltr_support == 1),
            RegFieldCompElem(12, 13, std::format(" TPH completer: TPH[{}] eTPH[{}]",
                             (pcie_cap->dev_cap2.tph_cmpl_support & 0x1) ? '+' : '-',
                             (pcie_cap->dev_cap2.tph_cmpl_support & 0x2) ? '+' : '-')),
            RegFieldCompElem(14, 15, std::format(" LN system CLS: {}",
                             DevCap2LNSysCLSDesc(pcie_cap->dev_cap2.ln_sys_cls))),
            RegFieldCompElem(16, 16, " 10-bit tag completer",
                             pcie_cap->dev_cap2.tag_10bit_cmpl_support == 1),
            RegFieldCompElem(17, 17, " 10-bit tag requester",
                             pcie_cap->dev_cap2.tag_10bit_req_support == 1),
            RegFieldCompElem(18, 19, std::format(" OBFF[{}]: msg signal [{}] WAKE# signal [{}]",
                             (pcie_cap->dev_cap2.obff_supported == 0x0) ? '-' : '+',
                             (pcie_cap->dev_cap2.obff_supported & 0x1) ? '+' : '-',
                             (pcie_cap->dev_cap2.obff_supported & 0x2) ? '+' : '-')),
            RegFieldCompElem(20, 20, " Ext fmt field",
                             pcie_cap->dev_cap2.ext_fmt_field_support == 1),
            RegFieldCompElem(21, 21, " end-end TLP prefix",
                             pcie_cap->dev_cap2.end_end_tlp_pref_support == 1),
            RegFieldCompElem(22, 23, std::format(" max end-end TLP prefixes: {}",
                             (pcie_cap->dev_cap2.max_end_end_tlp_pref == 0) ?
                             0x4 : pcie_cap->dev_cap2.max_end_end_tlp_pref)),
            RegFieldCompElem(24, 25, std::format(" Emerg power reduction state: {:#x}",
                              pcie_cap->dev_cap2.emerg_pwr_reduct_support)),
            RegFieldCompElem(26, 26, " Emerg power reduction init required",
                             pcie_cap->dev_cap2.emerg_pwr_reduct_init_req == 1),
            RegFieldCompElem(27, 30),
            RegFieldCompElem(31, 31, " FRS supported",
                             pcie_cap->dev_cap2.frs_support == 1),
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Device Capabilities 2 +0x24", std::move(dev_cap2_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Device Capabilities 2 +0x24"),
                        }));
    }

    // device status 2 / control 2
    auto dev_ctrl2_stat2_dw = reinterpret_cast<const uint32_t *>(&pcie_cap->dev_ctl2);
    if (*dev_ctrl2_stat2_dw != 0) {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Device Status 2 +0x2a"),
                            RegButtonComp("Device Control 2 +0x28", &vis[i++]),
                        }));

        auto dev_ctrl2_content = vbox({
            RegFieldCompElem(0, 3, std::format(" Cmpl timeout value: {}",
                             CmplTimeoutValueDesc(pcie_cap->dev_ctl2.cmpl_timeout_val))),
            RegFieldCompElem(4, 4, " Cmpl timeout disable",
                             pcie_cap->dev_ctl2.cmpl_timeout_dis == 1),
            RegFieldCompElem(5, 5, " ARI forwarding enable",
                             pcie_cap->dev_ctl2.ari_fwd_ena == 1),
            RegFieldCompElem(6, 6, " AtomicOP requester enable",
                             pcie_cap->dev_ctl2.atomic_op_req_ena == 1),
            RegFieldCompElem(7, 7, " AtomicOP egress block",
                             pcie_cap->dev_ctl2.atomic_op_egr_block == 1),
            RegFieldCompElem(8, 8, " IDO request enable",
                             pcie_cap->dev_ctl2.ido_req_ena == 1),
            RegFieldCompElem(9, 9, " IDO cmpl enable",
                             pcie_cap->dev_ctl2.ido_cmpl_ena == 1),
            RegFieldCompElem(10, 10, " LTR enable",
                             pcie_cap->dev_ctl2.ltr_ena == 1),
            RegFieldCompElem(11, 11, " Emerg power reduction request",
                             pcie_cap->dev_ctl2.emerg_pwr_reduct_req == 1),
            RegFieldCompElem(12, 12, " 10-bit tag requester enable",
                             pcie_cap->dev_ctl2.tag_10bit_req_ena == 1),
            RegFieldCompElem(13, 14, std::format(" OBFF enable: {}",
                             DevCtl2ObffDesc(pcie_cap->dev_ctl2.obff_ena))),
            RegFieldCompElem(15, 15, std::format(" end-end TLP prefix blocking: {}",
                             (pcie_cap->dev_ctl2.end_end_tlp_pref_block == 1) ?
                             "fwd blocked" : "fwd enabled"))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Device Control 2 +0x28", std::move(dev_ctrl2_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Device Status 2 +0x2a"),
                            EmptyCapRegComp("Device Control 2 +0x28"),
                        }));
    }

    // link capabilities 2
    auto link_cap2_dw = reinterpret_cast<const uint32_t *>(&pcie_cap->link_cap2);
    if (*link_cap2_dw != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Link Capabilities 2 +0x2c", &vis[i++]),
                        }));

        auto link_cap2_content = vbox({
            RegFieldCompElem(0, 0),
            RegFieldVerbElem(1, 7, std::format(" Supported link speeds: {}",
                             SuppLinkSpeedDesc(pcie_cap->link_cap2.supported_speed_vec)),
                             pcie_cap->link_cap2.supported_speed_vec),
            RegFieldCompElem(8, 8, " Crosslink", pcie_cap->link_cap2.crosslink_support == 1),
            RegFieldVerbElem(9, 15, std::format(" Lower SKP OS gen speeds: {}",
                             SuppLinkSpeedDesc(pcie_cap->link_cap2.low_skp_os_gen_supp_speed_vec)),
                             pcie_cap->link_cap2.low_skp_os_gen_supp_speed_vec),
            RegFieldVerbElem(16, 22, std::format(" Lower SKP OS reception speeds: {}",
                             SuppLinkSpeedDesc(pcie_cap->link_cap2.low_skp_os_rec_supp_speed_vec)),
                             pcie_cap->link_cap2.low_skp_os_rec_supp_speed_vec),
            RegFieldCompElem(23, 23, " Retimer presence detect",
                             pcie_cap->link_cap2.retmr_pres_detect_support == 1),
            RegFieldCompElem(24, 24, " 2 Retimers presence detect",
                             pcie_cap->link_cap2.two_retmr_pres_detect_support == 1),
            RegFieldCompElem(25, 30),
            RegFieldCompElem(31, 31, " DRS",
                             pcie_cap->link_cap2.drs_support == 1)
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Capabilities 2 +0x2c", std::move(link_cap2_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Link Capabilities 2 +0x2c"),
                        }));
    }

    // link status 2 / control 2
    auto link_stat2_ctrl2_dw = reinterpret_cast<const uint32_t *>(&pcie_cap->link_ctl2);
    if (*link_stat2_ctrl2_dw != 0) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Link Status 2 +0x32", &vis[i++]),
                            RegButtonComp("Link Control 2 +0x30", &vis[i++]),
                        }));

        auto link_stat2_content = vbox({
            RegFieldCompElem(0, 0, std::format(" Current de-emphasis level: {}",
                             pcie_cap->link_status2.curr_de_emph_lvl == 0x0 ?
                             "-6 dB" : "-3.5 dB")),
            RegFieldCompElem(1, 1, " Equalization 8GT/s complete",
                             pcie_cap->link_status2.eq_8gts_compl == 1),
            RegFieldCompElem(2, 2, " Equalization 8GT/s phase 1 success",
                             pcie_cap->link_status2.eq_8gts_ph1_success == 1),
            RegFieldCompElem(3, 3, " Equalization 8GT/s phase 2 success",
                             pcie_cap->link_status2.eq_8gts_ph2_success == 1),
            RegFieldCompElem(4, 4, " Equalization 8GT/s phase 3 success",
                             pcie_cap->link_status2.eq_8gts_ph3_success == 1),
            RegFieldCompElem(5, 5, " Link equalization req 8GT/s",
                             pcie_cap->link_status2.link_eq_req_8gts == 1),
            RegFieldCompElem(6, 6, " Retimer presence detected",
                             pcie_cap->link_status2.retmr_pres_detect == 1),
            RegFieldCompElem(7, 7, " 2 Retimers presence detected",
                             pcie_cap->link_status2.two_retmr_pres_detect == 1),
            RegFieldCompElem(8, 9, std::format(" Crosslink resolution: {}",
                             CrosslinkResDesc(pcie_cap->link_status2.crosslink_resolution))),
            RegFieldCompElem(10, 11),
            RegFieldCompElem(12, 14, std::format(" Downstream comp presence: {}",
                              DownstreamCompPresDesc(pcie_cap->link_status2.downstream_comp_pres))),
            RegFieldCompElem(15, 15, " DRS msg received",
                             pcie_cap->link_status2.drs_msg_recv == 1),
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Status 2 +0x32", std::move(link_stat2_content),
                                         &vis[i - 2]));

        auto link_ctrl2_content = vbox({
            RegFieldCompElem(0, 3, LinkSpeedDesc(LinkSpeedRepType::target,
                                                 pcie_cap->link_ctl2.tgt_link_speed,
                                                 pcie_cap->link_cap2)),
            RegFieldCompElem(4, 4, " Enter Compliance", pcie_cap->link_ctl2.enter_compliance == 1),
            RegFieldCompElem(5, 5, " HW autonomous speed disable",
                             pcie_cap->link_ctl2.hw_auto_speed_dis == 1),
            RegFieldCompElem(6, 6, std::format(" Selectable de-emphasis level: {}",
                             pcie_cap->link_ctl2.select_de_emph == 0x0 ?
                             "-6 dB" : "-3.5 dB")),
            RegFieldCompElem(7, 9, std::format(" Transmit margin: {}",
                             pcie_cap->link_ctl2.trans_margin == 0x0 ?
                             "normal operation" : "other(tbd)")),
            RegFieldCompElem(10, 10, " Enter modified compliance",
                             pcie_cap->link_ctl2.enter_mod_compliance == 1),
            RegFieldCompElem(11, 11, " Compliance sos",
                             pcie_cap->link_ctl2.compliance_sos == 1),
            RegFieldCompElem(12, 15, std::format(" Compliance preset/de-emph: {:#x}",
                             pcie_cap->link_ctl2.compliance_preset_de_emph))
        });

        lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Control 2 +0x30", std::move(link_ctrl2_content),
                                         &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Link Status 2 +0x32"),
                            EmptyCapRegComp("Link Control 2 +0x30")
                        }));
    }

    // slot capabilities 2
    upper.push_back(Container::Horizontal({
                        EmptyCapRegComp("Slot Capabilities 2 +0x34")
                    }));

    // slot control 2 / status 2
    upper.push_back(Container::Horizontal({
                        EmptyCapRegComp("Slot Status 2 +0x3a"),
                        EmptyCapRegComp("Slot Control 2 +0x38")
                    }));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
CompatMSIxCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
              std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 3;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto msix_cap = reinterpret_cast<const PciMSIxCap *>(dev->cfg_space_.get() + off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Message Control +0x2", &vis[i++]),
                        CapHdrComp(msix_cap->hdr)
                    }));

    auto msix_mc_content = vbox({
        RegFieldVerbElem(0, 10, std::format(" Table size: {:#04x}", msix_cap->msg_ctrl.table_size + 1),
                         msix_cap->msg_ctrl.table_size),
        RegFieldCompElem(11, 13),
        RegFieldCompElem(14, 14, " Function mask", msix_cap->msg_ctrl.func_mask),
        RegFieldCompElem(15, 15, " MSI-X enable", msix_cap->msg_ctrl.msix_ena)
    });
    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI-X", off),
                                     "Message Control +0x2", std::move(msix_mc_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Table Off/BIR +0x4", &vis[i++]),
                    }));

    auto msix_tbl_off_bir_content = vbox({
        RegFieldCompElem(0, 2,  std::format("    BAR: {:#x}", msix_cap->tbl_off_id.tbl_bar_entry)),
        RegFieldCompElem(3, 31, std::format(" Offset: {:#08x}", msix_cap->tbl_off_id.tbl_off << 3)),
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI-X", off),
                                     "Message Table Off/BIR +0x4",
                                     std::move(msix_tbl_off_bir_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("PBA Off/BIR +0x8", &vis[i++]),
                    }));

    auto msix_pba_off_bir_content = vbox({
        RegFieldCompElem(0, 2,  std::format("    BAR: {:#x}", msix_cap->pba_off_id.pba_bar_entry)),
        RegFieldCompElem(3, 31, std::format(" Offset: {:#08x}", msix_cap->pba_off_id.pba_off << 3)),
    });

    lower.push_back(CreateCapRegInfo(std::format("[compat][{:#02x}] MSI-X", off),
                                     "PBA Off/BIR +0x8",
                                     std::move(msix_pba_off_bir_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

capability_comp_ctx
GetCompatCapComponents(const pci::PciDevBase *dev, const CompatCapID cap_id,
                       const pci::CapDesc &cap, std::vector<uint8_t> &vis)
{
    switch(cap_id) {
    case CompatCapID::null_cap:
        return NotImplCap();
    case CompatCapID::pci_pm_iface:
        return CompatPMCap(dev, cap, vis);
    case CompatCapID::agp:
        return NotImplCap();
    case CompatCapID::vpd:
        return NotImplCap();
    case CompatCapID::slot_ident:
        return NotImplCap();
    case CompatCapID::msi:
        return CompatMSICap(dev, cap, vis);
    case CompatCapID::compat_pci_hot_swap:
        return NotImplCap();
    case CompatCapID::pci_x:
        return NotImplCap();
    case CompatCapID::hyper_transport:
        return NotImplCap();
    case CompatCapID::vendor_spec:
        return CompatVendorSpecCap(dev, cap, vis);
    case CompatCapID::dbg_port:
        return NotImplCap();
    case CompatCapID::compat_pci_central_res_ctl:
        return NotImplCap();
    case CompatCapID::pci_hot_plug:
        return NotImplCap();
    case CompatCapID::pci_brd_sub_vid:
        return NotImplCap();
    case CompatCapID::agp_x8:
        return NotImplCap();
    case CompatCapID::secure_dev:
        return NotImplCap();
    case CompatCapID::pci_express:
        return CompatPCIECap(dev, cap, vis);
    case CompatCapID::msix:
        return CompatMSIxCap(dev, cap, vis);
    case CompatCapID::sata_data_idx_conf:
        return NotImplCap();
    case CompatCapID::af:
        return NotImplCap();
    case CompatCapID::enhanced_alloc:
        return NotImplCap();
    case CompatCapID::flat_portal_brd:
        return NotImplCap();
    default:
        assert(false);
    }
}

} // namespace ui
