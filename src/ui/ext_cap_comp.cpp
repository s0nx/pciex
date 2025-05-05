// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024-2025 Petr Vyazovik <xen@f-m.fm>

#include "ext_cap_comp.h"
#include "log.h"
#include "util.h"

#include <cassert>
#include <bitset>
#include <format>

extern Logger logger;

using namespace ftxui;

namespace ui {

static capability_comp_ctx
ExtSecondaryPCIECap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                    std::vector<uint8_t> &vis)
{
    Components upper, lower;

    // In order to determine total amount of registers we need to consult
    // Link Capabilities register in PCIE capability and get max link width
    uint16_t pcie_cap_off = dev->GetCapOffByID(pci::CapType::compat,
                                               e_to_type(CompatCapID::pci_express));
    if (pcie_cap_off == 0) {
      logger.log(Verbosity::WARN,
                 "Secondary PCIe cap: failed to get primary PCIe cap offset");
      return NotImplCap();
    }

    auto pcie_cap = reinterpret_cast<const PciECap *>(dev->cfg_space_.get() + pcie_cap_off);
    auto max_link_width = pcie_cap->link_cap.max_link_width;
    auto reg_per_cap = 2 + max_link_width;

    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto sec_pcie_cap = reinterpret_cast<const SecPciECap *>(dev->cfg_space_.get() + off);
    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Link Control 3 +0x4", &vis[i++]),
                        CapHdrComp(sec_pcie_cap->hdr)
                    }));

    auto link_ctl3_content = vbox({
        RegFieldCompElem(0, 0, " Perform EQ", sec_pcie_cap->link_ctl3.perform_eq),
        RegFieldCompElem(1, 1, " Link EQ req intr enable", sec_pcie_cap->link_ctl3.link_eq_req_itr_ena),
        RegFieldCompElem(2, 8),
        RegFieldVerbElem(9, 15, std::format(" Enable lower SKP OS gen vector: {}",
                                            EnableLowerSKPOSGenVecDesc(sec_pcie_cap->link_ctl3.lower_skp_os_gen_vec_ena)),
                         sec_pcie_cap->link_ctl3.lower_skp_os_gen_vec_ena),
        RegFieldCompElem(16, 31)
    });
    lower.push_back(CreateCapRegInfo(std::format("[extended][{:#02x}] Secondary PCIe", off),
                                     "Link Control 3 +0x4",
                                     std::move(link_ctl3_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Lane Error Status +0x8", &vis[i++]),
                    }));

    auto lane_err_status_content = vbox({
        RegFieldVerbElem(0, 31, std::format(" Lane(s) with error detected: {:#04x}",
                                            sec_pcie_cap->lane_err_stat.lane_err_status),
                         sec_pcie_cap->lane_err_stat.lane_err_status),
    });
    lower.push_back(CreateCapRegInfo(std::format("[extended][{:#02x}] Secondary PCIe", off),
                                     "Lane Error Status +0x8",
                                     std::move(lane_err_status_content), &vis[i - 1]));

    // Check of port supports 8.0GT/s link speed or higher
    if (pcie_cap->link_cap2.supported_speed_vec & 0x4) {
        upper.push_back(
                Container::Horizontal({
                    RegButtonComp(std::format("Lane Equalization Control [{} lane(s)] +0xc",
                                  max_link_width), &vis[i++]),
        }));

        for (uint32_t cur_link = 0; cur_link < max_link_width; cur_link++) {
            auto lane_eq_ctl_reg = reinterpret_cast<const RegLaneEqCtl *>
                (dev->cfg_space_.get() + off + cur_link * sizeof(RegLaneEqCtl));
            auto lane_eq_ctl_content = vbox({
                RegFieldVerbElem(0, 3,
                    std::format(" Downstream port 8GT/s transmitter preset: {}",
                                TransPresHint8gtsDesc(lane_eq_ctl_reg->ds_port_8gts_trans_pres)),
                                lane_eq_ctl_reg->ds_port_8gts_trans_pres
                ),
                RegFieldVerbElem(4, 6,
                    std::format(" Downstream port 8GT/s receiver preset: {}",
                                RecvPresHint8gtsDesc(lane_eq_ctl_reg->ds_port_8gts_recv_pres_h)),
                                lane_eq_ctl_reg->ds_port_8gts_recv_pres_h
                ),
                RegFieldCompElem(7, 7),
                RegFieldVerbElem(8, 11,
                    std::format(" Upstream port 8GT/s transmitter preset: {}",
                                TransPresHint8gtsDesc(lane_eq_ctl_reg->us_port_8gts_trans_pres)),
                                lane_eq_ctl_reg->us_port_8gts_trans_pres
                ),
                RegFieldVerbElem(4, 6,
                    std::format(" Upstream port 8GT/s receiver preset: {}",
                                RecvPresHint8gtsDesc(lane_eq_ctl_reg->us_port_8gts_recv_pres_h)),
                                lane_eq_ctl_reg->us_port_8gts_recv_pres_h
                ),
                RegFieldCompElem(15, 15),
            });
            lower.push_back(
                    CreateCapRegInfo(std::format("[extended][{:#02x}] Secondary PCIe", off),
                                     std::format("Lane #{} Equalization Control +{:#01x}",
                                                 cur_link, (0xc + cur_link * 0x2)),
                                     std::move(lane_eq_ctl_content), &vis[i - 1]));
        }
    }

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
ExtDataLinkFeatureCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                      std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto dlink_feature_cap = reinterpret_cast<const DataLinkFeatureCap *>
                                             (dev->cfg_space_.get() + off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Data Link Feature Capabilities +0x4", &vis[i++]),
                        CapHdrComp(dlink_feature_cap->hdr)
                    }));

    std::bitset<23> local_dlink_feat_map {dlink_feature_cap->dlink_feat_cap.local_data_link_feat_supp};

    auto dlink_feature_caps_content = vbox({
        RegFieldVerbElem(0, 22,
                         std::format(" Local data link feature(s): Local Scaled Flow Ctl[{}]",
                                local_dlink_feat_map[0] ? '+' : '-'),
                         dlink_feature_cap->dlink_feat_cap.local_data_link_feat_supp),
        RegFieldCompElem(23, 30),
        RegFieldCompElem(31, 31, "Data link feature exchange enable",
                         dlink_feature_cap->dlink_feat_cap.data_link_feat_xchg_ena == 1)
    });
    lower.push_back(
            CreateCapRegInfo(std::format(
                                "[extended][{:#02x}] Data Link Feature", off),
                             "Data Link Feature Capabilities +0x4",
                             std::move(dlink_feature_caps_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Data Link Feature Status +0x8", &vis[i++]),
                    }));

    std::bitset<23> rem_dlink_feat_map {dlink_feature_cap->dlink_feat_stat.rem_data_link_feat_supp};

    auto dlink_feature_stat_content = vbox({
        RegFieldVerbElem(0, 22,
                         std::format(" Remote data link feature(s): Remote Scaled Flow Ctl[{}]",
                                rem_dlink_feat_map[0] ? '+' : '-'),
                         dlink_feature_cap->dlink_feat_stat.rem_data_link_feat_supp),
        RegFieldCompElem(23, 30),
        RegFieldCompElem(31, 31, "Remote Data link feature supported valid",
                         dlink_feature_cap->dlink_feat_stat.rem_data_link_feat_supp_valid == 1)
    });
    lower.push_back(
            CreateCapRegInfo(std::format(
                                "[extended][{:#02x}] Data Link Feature", off),
                             "Data Link Feature Status +0x8",
                             std::move(dlink_feature_stat_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
ExtARICap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                      std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto ari_cap = reinterpret_cast<const ARICap *>(dev->cfg_space_.get() + off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("ARI Capabilities +0x4", &vis[i++]),
                        CapHdrComp(ari_cap->hdr)
                    }));

    auto ari_cap_content = vbox({
        RegFieldCompElem(0, 0, " MFVC function groups capability", ari_cap->ari_cap.mfvc_func_grp_cap == 1),
        RegFieldCompElem(1, 1, " ACS function groups capability", ari_cap->ari_cap.acs_func_grp_cap == 1),
        RegFieldCompElem(2, 7),
        RegFieldCompElem(8, 15, std::format(" Next function: {:#0x}",
                                            ari_cap->ari_cap.next_func_num))
    });
    lower.push_back(
            CreateCapRegInfo(std::format("[extended][{:#02x}] ARI", off),
                             "ARI Capabilities +0x4",
                             std::move(ari_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("ARI Control +0x6", &vis[i++]),
                    }));
    auto ari_ctl_content = vbox({
        RegFieldCompElem(0, 0, " MFVC function groups enable", ari_cap->ari_ctl.mfvc_func_grps_ena == 1),
        RegFieldCompElem(1, 1, " ACS function groups enable", ari_cap->ari_ctl.acs_func_grps_ena == 1),
        RegFieldCompElem(2, 3),
        RegFieldCompElem(4, 6, std::format(" Function group: {:#01x}",
                                            ari_cap->ari_ctl.func_grp)),
        RegFieldCompElem(7, 15),
    });
    lower.push_back(
            CreateCapRegInfo(std::format("[extended][{:#02x}] ARI", off),
                             "ARI Control +0x6",
                             std::move(ari_ctl_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
ExtPASIDCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
            std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto pasid_cap = reinterpret_cast<const PASIDCap *>(dev->cfg_space_.get() + off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("PASID Capability +0x4", &vis[i++]),
                        CapHdrComp(pasid_cap->hdr)
                    }));

    auto pasid_cap_content = vbox({
        RegFieldCompElem(0, 0),
        RegFieldCompElem(1, 1, " Execute permission supported", pasid_cap->pasid_cap.exec_perm_supp == 1),
        RegFieldCompElem(2, 2, " Privileged mode supported", pasid_cap->pasid_cap.privileged_mode_supp == 1),
        RegFieldCompElem(3, 7),
        RegFieldCompElem(8, 12, std::format(" Max PASID width: {:#01x}",
                                            pasid_cap->pasid_cap.max_pasid_width)),
        RegFieldCompElem(13, 15)
    });
    lower.push_back(
            CreateCapRegInfo(std::format("[extended][{:#02x}] PASID", off),
                             "PASID Capability +0x4",
                             std::move(pasid_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("PASID Control +0x6", &vis[i++]),
                    }));
    auto pasid_ctl_content = vbox({
        RegFieldCompElem(0, 0, " PASID enable", pasid_cap->pasid_ctl.pasid_ena),
        RegFieldCompElem(1, 1, " Execute permission enable", pasid_cap->pasid_ctl.exec_perm_ena),
        RegFieldCompElem(2, 2, " Privileged mode enable", pasid_cap->pasid_ctl.privileged_mode_ena),
        RegFieldCompElem(3, 15)
    });
    lower.push_back(
            CreateCapRegInfo(std::format("[extended][{:#02x}] PASID", off),
                             "PASID Control +0x6",
                             std::move(pasid_ctl_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
ExtAERCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
          std::vector<uint8_t> &vis)
{
    Components upper, lower;

    uint16_t pcie_cap_off = dev->GetCapOffByID(pci::CapType::compat,
                                               e_to_type(CompatCapID::pci_express));
    if (pcie_cap_off == 0) {
      logger.log(Verbosity::WARN,
                 "AER cap: failed to get primary PCIe cap offset");
      return NotImplCap();
    }

    constexpr auto reg_per_cap = 12;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto aer_cap = reinterpret_cast<const AERCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] AER", off);
    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Uncorrectable Error Status +0x4", &vis[i++]),
                        CapHdrComp(aer_cap->hdr)
                    }));

    auto uncorr_err_status_content = vbox({
        RegFieldCompElem(0, 3),
        RegFieldCompElem(4, 4, " Data Link Protocol Error Status", aer_cap->uncorr_err_status.dlink_prot_err_status),
        RegFieldCompElem(5, 5, " Surprise Down Error Status", aer_cap->uncorr_err_status.supr_down_err_status),
        RegFieldCompElem(6, 11),
        RegFieldCompElem(12, 12, " Poisoned TLP Received Status", aer_cap->uncorr_err_status.poisoned_tlp_recv_status),
        RegFieldCompElem(13, 13, " Flow Control Protocol Error Status", aer_cap->uncorr_err_status.flow_ctl_proto_err_status),
        RegFieldCompElem(14, 14, " Completion Timeout Status", aer_cap->uncorr_err_status.comp_tmo_status),
        RegFieldCompElem(15, 15, " Completer Abort Status", aer_cap->uncorr_err_status.compl_abort_status),
        RegFieldCompElem(16, 16, " Enexpected Completion Status", aer_cap->uncorr_err_status.unexp_comp_status),
        RegFieldCompElem(17, 17, " Receiver Overflow Status", aer_cap->uncorr_err_status.recv_overflow_status),
        RegFieldCompElem(18, 18, " Malformed TLP Status", aer_cap->uncorr_err_status.malformed_tlp_status),
        RegFieldCompElem(19, 19, " ECRC Error Status", aer_cap->uncorr_err_status.ecrc_err_status),
        RegFieldCompElem(20, 20, " Unsupported Request Error Status", aer_cap->uncorr_err_status.unsupp_req_err_status),
        RegFieldCompElem(21, 21, " ACS Violation Status", aer_cap->uncorr_err_status.acs_violation_status),
        RegFieldCompElem(22, 22, " Uncorrectable Internal Error Status", aer_cap->uncorr_err_status.uncorr_internal_err_status),
        RegFieldCompElem(23, 23, " MC Blocked TLP Status", aer_cap->uncorr_err_status.mc_blocked_tlp_status),
        RegFieldCompElem(24, 24, " AtomicOP Egress Blocked Status", aer_cap->uncorr_err_status.atomic_op_egress_blocked_status),
        RegFieldCompElem(25, 25, " TLP Prefix Blocked Error Status", aer_cap->uncorr_err_status.tlp_pref_blocked_err_status),
        RegFieldCompElem(26, 26, " Poisoned TLP Egress Blocked Status", aer_cap->uncorr_err_status.poisoned_tlp_egress_blocked_status),
        RegFieldCompElem(27, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Uncorrectable Error Status +0x4",
                                     std::move(uncorr_err_status_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Uncorrectable Error Mask +0x8", &vis[i++]),
                    }));

    auto uncorr_err_mask_content = vbox({
        RegFieldCompElem(0, 3),
        RegFieldCompElem(4, 4, " Data Link Protocol Error Mask", aer_cap->uncorr_err_mask.dlink_prot_err_mask),
        RegFieldCompElem(5, 5, " Surprise Down Error Mask", aer_cap->uncorr_err_mask.supr_down_err_mask),
        RegFieldCompElem(6, 11),
        RegFieldCompElem(12, 12, " Poisoned TLP Received Mask", aer_cap->uncorr_err_mask.poisoned_tlp_recv_mask),
        RegFieldCompElem(13, 13, " Flow Control Protocol Error Mask", aer_cap->uncorr_err_mask.flow_ctl_proto_err_mask),
        RegFieldCompElem(14, 14, " Completion Timeout Mask", aer_cap->uncorr_err_mask.comp_tmo_mask),
        RegFieldCompElem(15, 15, " Completer Abort Mask", aer_cap->uncorr_err_mask.compl_abort_mask),
        RegFieldCompElem(16, 16, " Enexpected Completion Mask", aer_cap->uncorr_err_mask.unexp_comp_mask),
        RegFieldCompElem(17, 17, " Receiver Overflow Mask", aer_cap->uncorr_err_mask.recv_overflow_mask),
        RegFieldCompElem(18, 18, " Malformed TLP Mask", aer_cap->uncorr_err_mask.malformed_tlp_mask),
        RegFieldCompElem(19, 19, " ECRC Error Mask", aer_cap->uncorr_err_mask.ecrc_err_mask),
        RegFieldCompElem(20, 20, " Unsupported Request Error Mask", aer_cap->uncorr_err_mask.unsupp_req_err_mask),
        RegFieldCompElem(21, 21, " ACS Violation Mask", aer_cap->uncorr_err_mask.acs_violation_mask),
        RegFieldCompElem(22, 22, " Uncorrectable Internal Error Mask", aer_cap->uncorr_err_mask.uncorr_internal_err_mask),
        RegFieldCompElem(23, 23, " MC Blocked TLP Mask", aer_cap->uncorr_err_mask.mc_blocked_tlp_mask),
        RegFieldCompElem(24, 24, " AtomicOP Egress Blocked Mask", aer_cap->uncorr_err_mask.atomic_op_egress_blocked_mask),
        RegFieldCompElem(25, 25, " TLP Prefix Blocked Error Mask", aer_cap->uncorr_err_mask.tlp_pref_blocked_err_mask),
        RegFieldCompElem(26, 26, " Poisoned TLP Egress Blocked Mask", aer_cap->uncorr_err_mask.poisoned_tlp_egress_blocked_mask),
        RegFieldCompElem(27, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Uncorrectable Error Mask +0x8",
                                     std::move(uncorr_err_mask_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Uncorrectable Error Severity +0xc", &vis[i++]),
                    }));

    auto uncorr_err_sev_content = vbox({
        RegFieldCompElem(0, 3),
        RegFieldCompElem(4, 4, " Data Link Protocol Error Severity", aer_cap->uncorr_err_sev.dlink_prot_err_sev),
        RegFieldCompElem(5, 5, " Surprise Down Error Severity", aer_cap->uncorr_err_sev.supr_down_err_sev),
        RegFieldCompElem(6, 11),
        RegFieldCompElem(12, 12, " Poisoned TLP Received Severity", aer_cap->uncorr_err_sev.poisoned_tlp_recv_sev),
        RegFieldCompElem(13, 13, " Flow Control Protocol Error Severity", aer_cap->uncorr_err_sev.flow_ctl_proto_err_sev),
        RegFieldCompElem(14, 14, " Completion Timeout Severity", aer_cap->uncorr_err_sev.comp_tmo_sev),
        RegFieldCompElem(15, 15, " Completer Abort Severity", aer_cap->uncorr_err_sev.compl_abort_sev),
        RegFieldCompElem(16, 16, " Enexpected Completion Severity", aer_cap->uncorr_err_sev.unexp_comp_sev),
        RegFieldCompElem(17, 17, " Receiver Overflow Severity", aer_cap->uncorr_err_sev.recv_overflow_sev),
        RegFieldCompElem(18, 18, " Malformed TLP Severity", aer_cap->uncorr_err_sev.malformed_tlp_sev),
        RegFieldCompElem(19, 19, " ECRC Error Severity", aer_cap->uncorr_err_sev.ecrc_err_sev),
        RegFieldCompElem(20, 20, " Unsupported Request Error Severity", aer_cap->uncorr_err_sev.unsupp_req_err_sev),
        RegFieldCompElem(21, 21, " ACS Violation Severity", aer_cap->uncorr_err_sev.acs_violation_sev),
        RegFieldCompElem(22, 22, " Uncorrectable Internal Error Severity", aer_cap->uncorr_err_sev.uncorr_internal_err_sev),
        RegFieldCompElem(23, 23, " MC Blocked TLP Severity", aer_cap->uncorr_err_sev.mc_blocked_tlp_sev),
        RegFieldCompElem(24, 24, " AtomicOP Egress Blocked Severity", aer_cap->uncorr_err_sev.atomic_op_egress_blocked_sev),
        RegFieldCompElem(25, 25, " TLP Prefix Blocked Error Severity", aer_cap->uncorr_err_sev.tlp_pref_blocked_err_sev),
        RegFieldCompElem(26, 26, " Poisoned TLP Egress Blocked Severity", aer_cap->uncorr_err_sev.poisoned_tlp_egress_blocked_sev),
        RegFieldCompElem(27, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Uncorrectable Error Severity +0xc",
                                     std::move(uncorr_err_sev_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Correctable Error Status +0x10", &vis[i++]),
                    }));

    auto corr_err_status_content = vbox({
        RegFieldCompElem(0, 0, " Receiver Error Status", aer_cap->corr_err_status.recv_err_status),
        RegFieldCompElem(1, 5),
        RegFieldCompElem(6, 6, " Bad TLP Status", aer_cap->corr_err_status.bad_tlp_status),
        RegFieldCompElem(7, 7, " Bad DLLP Status", aer_cap->corr_err_status.bad_dllp_status),
        RegFieldCompElem(8, 8, " REPLAY_NUM Rollover Status", aer_cap->corr_err_status.repl_num_rollover_status),
        RegFieldCompElem(9, 11),
        RegFieldCompElem(12, 12, " Replay Timer Timeout Status", aer_cap->corr_err_status.repl_tmr_tmo_status),
        RegFieldCompElem(13, 13, " Advisory Non-Fatal Error Status", aer_cap->corr_err_status.adv_non_fatal_err_status),
        RegFieldCompElem(14, 14, " Corrected Internal Error Status", aer_cap->corr_err_status.corr_int_err_status),
        RegFieldCompElem(15, 15, " Header Log Overflow Status", aer_cap->corr_err_status.hdr_log_overflow_status),
        RegFieldCompElem(16, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Correctable Error Status +0x10",
                                     std::move(corr_err_status_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Correctable Error Mask +0x14", &vis[i++]),
                    }));

    auto corr_err_mask_content = vbox({
        RegFieldCompElem(0, 0, " Receiver Error Mask", aer_cap->corr_err_mask.recv_err_mask),
        RegFieldCompElem(1, 5),
        RegFieldCompElem(6, 6, " Bad TLP Mask", aer_cap->corr_err_mask.bad_tlp_mask),
        RegFieldCompElem(7, 7, " Bad DLLP Mask", aer_cap->corr_err_mask.bad_dllp_mask),
        RegFieldCompElem(8, 8, " REPLAY_NUM Rollover Mask", aer_cap->corr_err_mask.repl_num_rollover_mask),
        RegFieldCompElem(9, 11),
        RegFieldCompElem(12, 12, " Replay Timer Timeout Mask", aer_cap->corr_err_mask.repl_tmr_tmo_mask),
        RegFieldCompElem(13, 13, " Advisory Non-Fatal Error Mask", aer_cap->corr_err_mask.adv_non_fatal_err_mask),
        RegFieldCompElem(14, 14, " Corrected Internal Error Mask", aer_cap->corr_err_mask.corr_int_err_mask),
        RegFieldCompElem(15, 15, " Header Log Overflow Mask", aer_cap->corr_err_mask.hdr_log_overflow_mask),
        RegFieldCompElem(16, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Correctable Error Mask +0x14",
                                     std::move(corr_err_mask_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Advanced Error Capabilities and Control +0x18", &vis[i++]),
                    }));

    auto adv_err_cap_ctl_content = vbox({
        RegFieldCompElem(0, 4, std::format(" First Error Pointer: {:x}",
                                           aer_cap->adv_err_cap_ctl.first_err_ptr)),
        RegFieldCompElem(5, 5, " ECRC Generation Capable",  aer_cap->adv_err_cap_ctl.ecrc_gen_cap),
        RegFieldCompElem(6, 6, " ECRC Generation Enable", aer_cap->adv_err_cap_ctl.ecrc_gen_ena),
        RegFieldCompElem(7, 7, " ECRC Check Capable", aer_cap->adv_err_cap_ctl.ecrc_check_cap),
        RegFieldCompElem(8, 8, " ECRC Check Enable", aer_cap->adv_err_cap_ctl.ecrc_check_ena),
        RegFieldCompElem(9, 9, " Multiple Header Recording Capable", aer_cap->adv_err_cap_ctl.multi_hdr_rec_cap),
        RegFieldCompElem(10, 10, " Multiple Header Recording Enable", aer_cap->adv_err_cap_ctl.multi_hdr_rec_ena),
        RegFieldCompElem(11, 11, " TLP Prefix Log Present", aer_cap->adv_err_cap_ctl.tlp_pref_log_present),
        RegFieldCompElem(12, 12, " Completion Timeout Prefix/Header Log Capable",
                                 aer_cap->adv_err_cap_ctl.comp_tmo_pref_hdr_log_cap),
        RegFieldCompElem(13, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Advanced Error Capabilities and Control +0x18",
                                     std::move(adv_err_cap_ctl_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Header Log +0x1c", &vis[i++]),
                    }));

    // 7.8.4.8 header log register, need to do byteswap before building the component
    std::array<uint8_t, sizeof(RegAERHdrLog)> hdr_log;
    uint32_t *hdr_dw = reinterpret_cast<uint32_t *>(hdr_log.data());
    for (int i = 0; i < 4; i++)
        *hdr_dw++ = std::byteswap(aer_cap->hdr_log.dw[i]);

    auto hdr_log_content = GetHexDumpElem("TLP hdr >>>", hdr_log.data(), hdr_log.size(), 4);
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                     "Header Log +0x1c",
                                     std::move(hdr_log_content), &vis[i - 1]));

    auto pcie_cap = reinterpret_cast<const PciECap *>(dev->cfg_space_.get() + pcie_cap_off);
    // The following 4 registers are only available for root ports and root complex event collectors
    bool dev_is_rp_or_rcec = dev->type_ == pci::pci_dev_type::TYPE0 ?
                             pcie_cap->pcie_cap_reg.dev_port_type == 0b1010 :
                             pcie_cap->pcie_cap_reg.dev_port_type == 0b0100;

    if (dev_is_rp_or_rcec) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("Root Error Command +0x2c", &vis[i++]),
                        }));

        auto root_err_comm_content = vbox({
            RegFieldCompElem(0, 0, " Correctable Error Reporting Enable", aer_cap->root_err_cmd.corr_err_rep_ena),
            RegFieldCompElem(1, 1, " Non-fatal Error Reporting Enable", aer_cap->root_err_cmd.non_fatal_err_rep_ena),
            RegFieldCompElem(2, 2, " Fatal Error Reporting Enable", aer_cap->root_err_cmd.fatal_err_rep_ena),
            RegFieldCompElem(3, 31)
        });

        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         "Root Error Command +0x2c",
                                         std::move(root_err_comm_content), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("Root Error Status +0x30", &vis[i++]),
                        }));

        auto root_err_status_content = vbox({
            RegFieldCompElem(0, 0, " ERR_COR received", aer_cap->root_err_status.err_cor_recv),
            RegFieldCompElem(1, 1, " Multiple ERR_COR received", aer_cap->root_err_status.multi_err_cor_recv),
            RegFieldCompElem(2, 2, " ERR_FATAL/NONFATAL received", aer_cap->root_err_status.err_fatal_nonfatal_recv),
            RegFieldCompElem(3, 3, " Multiple ERR_FATAL/NONFATAL received", aer_cap->root_err_status.multi_err_fatal_nonfatal_recv),
            RegFieldCompElem(4, 4, " First Uncorrectable Fatal", aer_cap->root_err_status.first_uncorr_fatal),
            RegFieldCompElem(5, 5, " Non-fatal error messages received", aer_cap->root_err_status.nonfatal_err_msg_recv),
            RegFieldCompElem(6, 6, " Fatal error messages received", aer_cap->root_err_status.fatal_err_msg_recv),
            RegFieldCompElem(7, 26),
            RegFieldCompElem(27, 31, std::format(" Advanced Error Itr Msg Number: {:#x}",
                                                 aer_cap->root_err_status.adv_err_int_msg_num))
        });

        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         "Root Error Status +0x30",
                                         std::move(root_err_status_content), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("Error Source ID +0x34", &vis[i++]),
                        }));

        auto err_srcid_content = vbox({
            RegFieldCompElem(0, 15, std::format("            ERR_COR Requester ID: {:#x}",
                                                 aer_cap->corr_err_src_id.req_id)),
            RegFieldCompElem(16, 31, std::format(" ERR_FATAL/NONFATAL Requester ID: {:#x}",
                                                 aer_cap->err_src_id.req_id))
        });

        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         "Error Source ID +0x34",
                                         std::move(err_srcid_content), &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Root Error Command +0x2c")
                        }));
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Root Error Status +0x30")
                        }));
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Error Source ID +0x36"),
                            EmptyCapRegComp("Correctable Error Source ID +0x34")
                        }));
    }

    if (aer_cap->adv_err_cap_ctl.tlp_pref_log_present) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("TLP Prefix Log +0x38", &vis[i++]),
                        }));

        // 7.8.4.12 tlp prefix log register, need to do byteswap before building the component
        std::array<uint8_t, sizeof(RegAERTlpPrefixLog)> tlp_pref_log;
        uint32_t *hdr_dw = reinterpret_cast<uint32_t *>(tlp_pref_log.data());
        for (int i = 0; i < 4; i++)
            *hdr_dw++ = std::byteswap(aer_cap->tlp_pref_log.dw[i]);

        auto tlp_pref_log_content = GetHexDumpElem("e2e TLP prefix >>>",
                                                   tlp_pref_log.data(), tlp_pref_log.size(), 4);
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         "TLP Prefix Log +0x38",
                                         std::move(tlp_pref_log_content), &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("TLP Prefix Log +0x38")
                        }));
    }

    return {std::move(upper), std::move(lower)};
}

capability_comp_ctx
GetExtendedCapComponents(const pci::PciDevBase *dev, const ExtCapID cap_id,
                         const pci::CapDesc &cap, std::vector<uint8_t> &vis)
{
    switch(cap_id) {
    case ExtCapID::null_cap:
        return NotImplCap();
    case ExtCapID::aer:
        return ExtAERCap(dev, cap, vis);
    case ExtCapID::vc_no_mfvc:
        return NotImplCap();
    case ExtCapID::dev_serial:
        return NotImplCap();
    case ExtCapID::power_budget:
        return NotImplCap();
    case ExtCapID::rc_link_decl:
        return NotImplCap();
    case ExtCapID::rc_internal_link_ctl:
        return NotImplCap();
    case ExtCapID::rc_ev_collector_ep_assoc:
        return NotImplCap();
    case ExtCapID::mfvc:
        return NotImplCap();
    case ExtCapID::vc_mfvc_pres:
        return NotImplCap();
    case ExtCapID::rcrb:
        return NotImplCap();
    case ExtCapID::vendor_spec_ext_cap:
        return NotImplCap();
    case ExtCapID::cac:
        return NotImplCap();
    case ExtCapID::acs:
        return NotImplCap();
    case ExtCapID::ari:
        return ExtARICap(dev, cap, vis);
    case ExtCapID::ats:
        return NotImplCap();
    case ExtCapID::sriov:
        return NotImplCap();
    case ExtCapID::mriov:
        return NotImplCap();
    case ExtCapID::mcast:
        return NotImplCap();
    case ExtCapID::page_req_iface:
        return NotImplCap();
    case ExtCapID::amd_rsvd:
        return NotImplCap();
    case ExtCapID::res_bar:
        return NotImplCap();
    case ExtCapID::dpa:
        return NotImplCap();
    case ExtCapID::tph_req:
        return NotImplCap();
    case ExtCapID::ltr:
        return NotImplCap();
    case ExtCapID::sec_pcie:
        return ExtSecondaryPCIECap(dev, cap, vis);
    case ExtCapID::pmux:
        return NotImplCap();
    case ExtCapID::pasid:
        return ExtPASIDCap(dev, cap, vis);
    case ExtCapID::lnr:
        return NotImplCap();
    case ExtCapID::dpc:
        return NotImplCap();
    case ExtCapID::l1_pm_substates:
        return NotImplCap();
    case ExtCapID::ptm:
        return NotImplCap();
    case ExtCapID::pcie_over_mphy:
        return NotImplCap();
    case ExtCapID::frs_q:
        return NotImplCap();
    case ExtCapID::readiness_tr:
        return NotImplCap();
    case ExtCapID::dvsec:
        return NotImplCap();
    case ExtCapID::vf_res_bar:
        return NotImplCap();
    case ExtCapID::data_link_feat:
        return ExtDataLinkFeatureCap(dev, cap, vis);
    case ExtCapID::phys_16gt:
        return NotImplCap();
    case ExtCapID::lane_marg_rx:
        return NotImplCap();
    case ExtCapID::hierarchy_id:
        return NotImplCap();
    case ExtCapID::npem:
        return NotImplCap();
    case ExtCapID::phys_32gt:
        return NotImplCap();
    case ExtCapID::alter_proto:
        return NotImplCap();
    case ExtCapID::sfi:
        return NotImplCap();
    default:
        assert(false);
    }
}

} // namespace ui
