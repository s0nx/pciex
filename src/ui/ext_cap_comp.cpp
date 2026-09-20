// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024-2026 Petr Vyazovik <xen@f-m.fm>

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

static std::string LTRLatencyDesc(uint16_t value, uint8_t scale)
{
    static constexpr uint32_t mult[] = {1, 32, 1024, 32768, 1048576, 33554432};
    if (scale > 5)
        return "< reserved scale >";
    return std::format("{} ns", static_cast<uint64_t>(value) * mult[scale]);
}

static capability_comp_ctx
ExtLTRCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
          std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto ltr_cap = reinterpret_cast<const LTRCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] LTR", off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Max Snoop Latency +0x4", &vis[i++]),
                        CapHdrComp(ltr_cap->hdr)
                    }));

    auto max_snoop_content = vbox({
        RegFieldVerbElem(0, 9, std::format(" Max snoop latency: {}",
                                           LTRLatencyDesc(ltr_cap->max_snoop.value,
                                                          ltr_cap->max_snoop.scale)),
                         ltr_cap->max_snoop.value),
        RegFieldVerbElem(10, 12, " Max snoop latency scale", ltr_cap->max_snoop.scale),
        RegFieldCompElem(13, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Max Snoop Latency +0x4",
                                     std::move(max_snoop_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Max No-Snoop Latency +0x6", &vis[i++]),
                    }));

    auto max_no_snoop_content = vbox({
        RegFieldVerbElem(0, 9, std::format(" Max no-snoop latency: {}",
                                           LTRLatencyDesc(ltr_cap->max_no_snoop.value,
                                                          ltr_cap->max_no_snoop.scale)),
                         ltr_cap->max_no_snoop.value),
        RegFieldVerbElem(10, 12, " Max no-snoop latency scale", ltr_cap->max_no_snoop.scale),
        RegFieldCompElem(13, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Max No-Snoop Latency +0x6",
                                     std::move(max_no_snoop_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static std::string L1PMPowerOnScaleDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "2 us";
    case 0b01:
        return "10 us";
    case 0b10:
        return "100 us";
    default:
        return "< reserved >";
    }
}

static capability_comp_ctx
ExtL1PMSubstatesCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                    std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 4;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto l1pm_cap = reinterpret_cast<const L1PMSubstatesCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] L1 PM Substates", off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("L1 PM Substates Capabilities +0x4", &vis[i++]),
                        CapHdrComp(l1pm_cap->hdr)
                    }));

    auto &pmcap = l1pm_cap->cap;
    auto l1pm_cap_content = vbox({
        RegFieldCompElem(0, 0, " PCI-PM L1.2 supported", pmcap.pci_pm_l1_2_supported == 1),
        RegFieldCompElem(1, 1, " PCI-PM L1.1 supported", pmcap.pci_pm_l1_1_supported == 1),
        RegFieldCompElem(2, 2, " ASPM L1.2 supported", pmcap.aspm_l1_2_supported == 1),
        RegFieldCompElem(3, 3, " ASPM L1.1 supported", pmcap.aspm_l1_1_supported == 1),
        RegFieldCompElem(4, 4, " L1 PM Substates supported", pmcap.l1_pm_substates_supported == 1),
        RegFieldCompElem(5, 5, " Link Activation supported", pmcap.link_activation_supported == 1),
        RegFieldCompElem(6, 7),
        RegFieldVerbElem(8, 15, " Port Common_Mode_Restore_Time (us)",
                         pmcap.port_common_mode_restore_time),
        RegFieldVerbElem(16, 17, std::format(" Port T_POWER_ON scale: {}",
                                             L1PMPowerOnScaleDesc(pmcap.port_t_power_on_scale)),
                         pmcap.port_t_power_on_scale),
        RegFieldCompElem(18, 18),
        RegFieldVerbElem(19, 23, " Port T_POWER_ON value", pmcap.port_t_power_on_value),
        RegFieldCompElem(24, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "L1 PM Substates Capabilities +0x4",
                                     std::move(l1pm_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("L1 PM Substates Control 1 +0x8", &vis[i++]),
                    }));
    auto &ctl1 = l1pm_cap->ctl1;
    auto l1pm_ctl1_content = vbox({
        RegFieldCompElem(0, 0, " PCI-PM L1.2 enable", ctl1.pci_pm_l1_2_ena == 1),
        RegFieldCompElem(1, 1, " PCI-PM L1.1 enable", ctl1.pci_pm_l1_1_ena == 1),
        RegFieldCompElem(2, 2, " ASPM L1.2 enable", ctl1.aspm_l1_2_ena == 1),
        RegFieldCompElem(3, 3, " ASPM L1.1 enable", ctl1.aspm_l1_1_ena == 1),
        RegFieldCompElem(4, 4, " Link Activation interrupt enable", ctl1.link_activation_itr_ena == 1),
        RegFieldCompElem(5, 5, " Link Activation control", ctl1.link_activation_ctl == 1),
        RegFieldCompElem(6, 7),
        RegFieldVerbElem(8, 15, " Common_Mode_Restore_Time (us)", ctl1.common_mode_restore_time),
        RegFieldVerbElem(16, 25, " LTR_L1.2_THRESHOLD value", ctl1.ltr_l1_2_threshold_value),
        RegFieldCompElem(26, 28),
        RegFieldVerbElem(29, 31, " LTR_L1.2_THRESHOLD scale", ctl1.ltr_l1_2_threshold_scale)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "L1 PM Substates Control 1 +0x8",
                                     std::move(l1pm_ctl1_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("L1 PM Substates Control 2 +0xc", &vis[i++]),
                    }));
    auto &ctl2 = l1pm_cap->ctl2;
    auto l1pm_ctl2_content = vbox({
        RegFieldVerbElem(0, 1, std::format(" T_POWER_ON scale: {}",
                                           L1PMPowerOnScaleDesc(ctl2.t_power_on_scale)),
                         ctl2.t_power_on_scale),
        RegFieldCompElem(2, 2),
        RegFieldVerbElem(3, 7, " T_POWER_ON value", ctl2.t_power_on_value),
        RegFieldCompElem(8, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "L1 PM Substates Control 2 +0xc",
                                     std::move(l1pm_ctl2_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("L1 PM Substates Status +0x10", &vis[i++]),
                    }));
    auto l1pm_status_content = vbox({
        RegFieldCompElem(0, 0, " Link Activation status", l1pm_cap->status.link_activation_status == 1),
        RegFieldCompElem(1, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "L1 PM Substates Status +0x10",
                                     std::move(l1pm_status_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static std::string ResizableBARSizeDesc(uint8_t val)
{
    if (val > 43)
        return "< reserved >";
    static constexpr const char *units[] = {"MB", "GB", "TB", "PB", "EB"};
    uint32_t unit_idx = val / 10;
    uint32_t unit_val = 1u << (val % 10);
    return std::format("{} {}", unit_val, units[unit_idx]);
}

// Same [+]/[-] flag convention as RegFlagListElem, but wrapped into a grid of
// @num_cols columns instead of one flag per line, for bitmaps with many
// short, uniform names (kept local since only these two capabilities need it).
static Element FlagGridElem(const std::vector<std::pair<std::string, bool>> &flags,
                            uint32_t num_cols)
{
    std::vector<Elements> rows;
    Elements row;

    for (auto &[name, set] : flags) {
        auto mark = text(set ? "[+] " : "[-] ") | bold;
        auto name_text = text(name);

        if (set) {
            mark |= color(Color::Green);
        } else {
            mark |= dim;
            name_text |= dim;
        }

        row.push_back(hbox({std::move(mark), std::move(name_text), separatorEmpty()}));
        if (row.size() == num_cols) {
            rows.push_back(std::move(row));
            row.clear();
        }
    }
    if (!row.empty())
        rows.push_back(std::move(row));

    return hbox({
        text("         "),
        separator(),
        gridbox(std::move(rows))
    });
}

// Lists every size bit in @bitmap as a [+]/[-] grid; @base_idx is the BAR
// Size encoding of bit 0 (0 for the Capability register's bits 4:31, 28 for
// the Control register's extended bits 16:31 - see Table 7-116/7-117);
// @num_bits is the field width (28 or 16).
static Element ResizableBARSupportedSizesElem(uint32_t bitmap, uint8_t base_idx, uint8_t num_bits)
{
    std::vector<std::pair<std::string, bool>> flags;
    for (uint8_t bit = 0; bit < num_bits; bit++)
        flags.emplace_back(ResizableBARSizeDesc(base_idx + bit), bitmap & (1u << bit));
    return FlagGridElem(flags, 4);
}

static capability_comp_ctx
ExtResizableBARCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                   std::vector<uint8_t> &vis)
{
    Components upper, lower;

    auto off = std::get<3>(cap);
    auto rbar_cap = reinterpret_cast<const ResizableBARCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] Resizable BAR", off);

    constexpr auto entries_off = sizeof(ResizableBARCap);
    auto entry0 = reinterpret_cast<const RegResizableBAREntry *>
                 (dev->cfg_space_.get() + off + entries_off);
    uint32_t num_bars = entry0->ctl.num_resizable_bars;
    if (num_bars == 0 || num_bars > 6)
        num_bars = 1;

    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), num_bars * 2, 0);

    upper.push_back(CapDelimComp(cap));

    for (uint32_t n = 0; n < num_bars; n++) {
        auto entry_off = entries_off + n * sizeof(RegResizableBAREntry);
        auto entry = reinterpret_cast<const RegResizableBAREntry *>
                    (dev->cfg_space_.get() + off + entry_off);

        if (n == 0)
            upper.push_back(Container::Horizontal({
                                RegButtonComp(std::format("BAR{} Capability +{:#03x}", n, entry_off),
                                             &vis[i++]),
                                CapHdrComp(rbar_cap->hdr)
                            }));
        else
            upper.push_back(Container::Horizontal({
                                RegButtonComp(std::format("BAR{} Capability +{:#03x}", n, entry_off),
                                             &vis[i++])
                            }));

        auto cap_content = vbox({
            RegFieldCompElem(0, 3),
            RegFieldVerbElem(4, 31, " Supported BAR sizes (1 MB - 128 TB):",
                             entry->cap.supported_bar_sizes),
            ResizableBARSupportedSizesElem(entry->cap.supported_bar_sizes, 0, 28)
        });
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         std::format("BAR{} Capability +{:#03x}", n, entry_off),
                                         std::move(cap_content), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(std::format("BAR{} Control +{:#03x}", n, entry_off + 4),
                                         &vis[i++]),
                        }));
        auto ctl_content = vbox({
            RegFieldVerbElem(0, 2, std::format(" BAR index: {}", entry->ctl.bar_index),
                             entry->ctl.bar_index),
            RegFieldCompElem(3, 4),
            RegFieldVerbElem(5, 7, std::format(" Number of resizable BARs: {}",
                                               entry->ctl.num_resizable_bars),
                             entry->ctl.num_resizable_bars),
            RegFieldVerbElem(8, 13, std::format(" BAR size: {}",
                                                ResizableBARSizeDesc(entry->ctl.bar_size)),
                             entry->ctl.bar_size),
            RegFieldCompElem(14, 15),
            RegFieldVerbElem(16, 31, " Supported BAR sizes (256 TB - 8 EB):",
                             entry->ctl.supported_bar_sizes_ext),
            ResizableBARSupportedSizesElem(entry->ctl.supported_bar_sizes_ext, 28, 16)
        });
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         std::format("BAR{} Control +{:#03x}", n, entry_off + 4),
                                         std::move(ctl_content), &vis[i - 1]));
    }

    return {std::move(upper), std::move(lower)};
}

// bit n means page size 4KB * 2^n (2^(n+12) bytes) is supported/selected
static std::string SRIOVPageSizeDesc(uint8_t bit)
{
    static constexpr const char *units[] = {"KB", "MB", "GB", "TB"};
    uint32_t exp = bit + 2;
    uint32_t unit_idx = exp / 10;
    uint32_t unit_val = 1u << (exp % 10);
    return std::format("{} {}", unit_val, units[unit_idx]);
}

static Element SRIOVPageSizesElem(uint32_t bitmap)
{
    std::vector<std::pair<std::string, bool>> flags;
    for (uint8_t bit = 0; bit < 32; bit++)
        flags.emplace_back(SRIOVPageSizeDesc(bit), bitmap & (1u << bit));
    return FlagGridElem(flags, 4);
}

static capability_comp_ctx
ExtSRIOVCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
           std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 17;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto sriov_cap = reinterpret_cast<const SRIOVCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] SR-IOV", off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("SR-IOV Capabilities +0x4", &vis[i++]),
                        CapHdrComp(sriov_cap->hdr)
                    }));
    auto &scap = sriov_cap->sriov_cap;
    auto sriov_cap_content = vbox({
        RegFieldCompElem(0, 0, " VF Migration capable", scap.vf_migration_capable == 1),
        RegFieldCompElem(1, 1, " ARI Capable Hierarchy preserved", scap.ari_capable_hierarchy_preserved == 1),
        RegFieldCompElem(2, 2, " VF 10-Bit Tag Requester supported", scap.vf_10bit_tag_req_supported == 1),
        RegFieldCompElem(3, 20),
        RegFieldVerbElem(21, 31, " VF Migration interrupt message number", scap.vf_migration_itr_msg_num)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "SR-IOV Capabilities +0x4",
                                     std::move(sriov_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("SR-IOV Control +0x8", &vis[i++]),
                    }));
    auto &sctl = sriov_cap->sriov_ctl;
    auto sriov_ctl_content = vbox({
        RegFieldCompElem(0, 0, " VF Enable", sctl.vf_ena == 1),
        RegFieldCompElem(1, 1, " VF Migration Enable", sctl.vf_migration_ena == 1),
        RegFieldCompElem(2, 2, " VF Migration Interrupt Enable", sctl.vf_migration_itr_ena == 1),
        RegFieldCompElem(3, 3, " VF MSE", sctl.vf_mse == 1),
        RegFieldCompElem(4, 4, " ARI Capable Hierarchy", sctl.ari_capable_hierarchy == 1),
        RegFieldCompElem(5, 5, " VF 10-Bit Tag Requester Enable", sctl.vf_10bit_tag_req_ena == 1),
        RegFieldCompElem(6, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "SR-IOV Control +0x8",
                                     std::move(sriov_ctl_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("SR-IOV Status +0xa", &vis[i++]),
                    }));
    auto sriov_status_content = vbox({
        RegFieldCompElem(0, 0, " VF Migration Status", sriov_cap->sriov_status.vf_migration_status == 1),
        RegFieldCompElem(1, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "SR-IOV Status +0xa",
                                     std::move(sriov_status_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("InitialVFs +0xc", &vis[i++]),
                    }));
    auto initial_vfs_content = vbox({
        RegFieldVerbElem(0, 15, " InitialVFs", sriov_cap->initial_vfs.initial_vfs)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "InitialVFs +0xc",
                                     std::move(initial_vfs_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("TotalVFs +0xe", &vis[i++]),
                    }));
    auto total_vfs_content = vbox({
        RegFieldVerbElem(0, 15, " TotalVFs", sriov_cap->total_vfs.total_vfs)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "TotalVFs +0xe",
                                     std::move(total_vfs_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("NumVFs / Fcn Dep Link +0x10", &vis[i++]),
                    }));
    auto &numvfs = sriov_cap->num_vfs_fcn_dep_link;
    auto numvfs_content = vbox({
        RegFieldVerbElem(0, 15, " NumVFs", numvfs.num_vfs),
        RegFieldVerbElem(16, 23, " Function Dependency Link", numvfs.fcn_dep_link),
        RegFieldCompElem(24, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "NumVFs / Fcn Dep Link +0x10",
                                     std::move(numvfs_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("First VF Offset / VF Stride +0x14", &vis[i++]),
                    }));
    auto &vfoff = sriov_cap->first_vf_off_stride;
    auto vfoff_content = vbox({
        RegFieldVerbElem(0, 15, " First VF Offset", vfoff.first_vf_offset),
        RegFieldVerbElem(16, 31, " VF Stride", vfoff.vf_stride)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "First VF Offset / VF Stride +0x14",
                                     std::move(vfoff_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("VF Device ID +0x18", &vis[i++]),
                    }));
    auto vfdevid_content = vbox({
        RegFieldCompElem(0, 15),
        RegFieldVerbElem(16, 31, " VF Device ID", sriov_cap->vf_dev_id.vf_device_id)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "VF Device ID +0x18",
                                     std::move(vfdevid_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Supported Page Sizes +0x1c", &vis[i++]),
                    }));
    auto supp_pg_content = vbox({
        RegFieldVerbElem(0, 31, " Supported page sizes:",
                         sriov_cap->supported_page_sizes.supported_page_sizes),
        SRIOVPageSizesElem(sriov_cap->supported_page_sizes.supported_page_sizes)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Supported Page Sizes +0x1c",
                                     std::move(supp_pg_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("System Page Size +0x20", &vis[i++]),
                    }));
    auto sys_pg_content = vbox({
        RegFieldVerbElem(0, 31, " System page size:",
                         sriov_cap->system_page_size.system_page_size),
        SRIOVPageSizesElem(sriov_cap->system_page_size.system_page_size)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "System Page Size +0x20",
                                     std::move(sys_pg_content), &vis[i - 1]));

    const RegSRIOVVFBAR *vf_bars[6] = {
        &sriov_cap->vf_bar0, &sriov_cap->vf_bar1, &sriov_cap->vf_bar2,
        &sriov_cap->vf_bar3, &sriov_cap->vf_bar4, &sriov_cap->vf_bar5
    };
    for (uint32_t n = 0; n < 6; n++) {
        auto bar_off = 0x24 + n * 4;
        upper.push_back(Container::Horizontal({
                            RegButtonComp(std::format("VF BAR{} +{:#03x}", n, bar_off), &vis[i++]),
                        }));
        auto vf_bar_content = vbox({
            RegFieldVerbElem(0, 31, std::format(" VF BAR{}: {:#08x}", n, vf_bars[n]->bar),
                             vf_bars[n]->bar)
        });
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr,
                                         std::format("VF BAR{} +{:#03x}", n, bar_off),
                                         std::move(vf_bar_content), &vis[i - 1]));
    }

    upper.push_back(Container::Horizontal({
                        RegButtonComp("VF Migration State Array Offset +0x3c", &vis[i++]),
                    }));
    auto &migoff = sriov_cap->vf_mig_state_arr_off;
    auto migoff_content = vbox({
        RegFieldVerbElem(0, 2, " VF Migration State BIR", migoff.vf_migration_state_bir),
        RegFieldVerbElem(3, 31, " VF Migration State Offset", migoff.vf_migration_state_offset)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "VF Migration State Array Offset +0x3c",
                                     std::move(migoff_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static const char *ACSMemTgtAccessCtlDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "Direct Request access enabled";
    case 0b01:
        return "Request blocking enabled";
    case 0b10:
        return "Request redirect enabled";
    default:
        return "< reserved >";
    }
}

static capability_comp_ctx
ExtACSCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
         std::vector<uint8_t> &vis)
{
    Components upper, lower;

    auto off = std::get<3>(cap);
    auto acs_cap = reinterpret_cast<const ACSCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] ACS", off);

    bool has_egress_vec = acs_cap->acs_cap.p2p_egress_ctl == 1;
    uint32_t vec_size = !has_egress_vec ? 0 :
                        acs_cap->acs_cap.egress_ctl_vec_size == 0 ? 256 :
                        acs_cap->acs_cap.egress_ctl_vec_size;
    uint32_t vec_bytes = ((vec_size + 31) / 32) * 4;

    auto reg_per_cap = has_egress_vec ? 3 : 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("ACS Capability +0x4", &vis[i++]),
                        CapHdrComp(acs_cap->hdr)
                    }));

    auto &acap = acs_cap->acs_cap;
    auto acs_cap_content = vbox({
        RegFieldCompElem(0, 0, " ACS Source Validation", acap.src_val == 1),
        RegFieldCompElem(1, 1, " ACS Translation Blocking", acap.trans_block == 1),
        RegFieldCompElem(2, 2, " ACS P2P Request Redirect", acap.p2p_req_redirect == 1),
        RegFieldCompElem(3, 3, " ACS P2P Completion Redirect", acap.p2p_compl_redirect == 1),
        RegFieldCompElem(4, 4, " ACS Upstream Forwarding", acap.upstream_fwd == 1),
        RegFieldCompElem(5, 5, " ACS P2P Egress Control", acap.p2p_egress_ctl == 1),
        RegFieldCompElem(6, 6, " ACS Direct Translated P2P", acap.direct_trans_p2p == 1),
        RegFieldCompElem(7, 7, " ACS Enhanced Capability", acap.enhanced_cap == 1),
        RegFieldVerbElem(8, 15, std::format(" Egress Control Vector size: {} bits", vec_size),
                         acap.egress_ctl_vec_size)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "ACS Capability +0x4",
                                     std::move(acs_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("ACS Control +0x6", &vis[i++]),
                    }));
    auto &actl = acs_cap->acs_ctl;
    auto acs_ctl_content = vbox({
        RegFieldCompElem(0, 0, " ACS Source Validation Enable", actl.src_val_ena == 1),
        RegFieldCompElem(1, 1, " ACS Translation Blocking Enable", actl.trans_block_ena == 1),
        RegFieldCompElem(2, 2, " ACS P2P Request Redirect Enable", actl.p2p_req_redirect_ena == 1),
        RegFieldCompElem(3, 3, " ACS P2P Completion Redirect Enable", actl.p2p_compl_redirect_ena == 1),
        RegFieldCompElem(4, 4, " ACS Upstream Forwarding Enable", actl.upstream_fwd_ena == 1),
        RegFieldCompElem(5, 5, " ACS P2P Egress Control Enable", actl.p2p_egress_ctl_ena == 1),
        RegFieldCompElem(6, 6, " ACS Direct Translated P2P Enable", actl.direct_trans_p2p_ena == 1),
        RegFieldCompElem(7, 7, " ACS I/O Request Blocking Enable", actl.io_req_block_ena == 1),
        RegFieldVerbElem(8, 9, std::format(" ACS DSP Memory Target Access Control: {}",
                                          ACSMemTgtAccessCtlDesc(actl.dsp_mem_tgt_access_ctl)),
                         actl.dsp_mem_tgt_access_ctl),
        RegFieldVerbElem(10, 11, std::format(" ACS USP Memory Target Access Control: {}",
                                            ACSMemTgtAccessCtlDesc(actl.usp_mem_tgt_access_ctl)),
                         actl.usp_mem_tgt_access_ctl),
        RegFieldCompElem(12, 12, " ACS Unclaimed Request Redirect Control", actl.unclaimed_req_redirect_ctl == 1),
        RegFieldCompElem(13, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "ACS Control +0x6",
                                     std::move(acs_ctl_content), &vis[i - 1]));

    if (has_egress_vec) {
        auto reg_label = std::format("Egress Control Vector [{} bits] +0x8", vec_size);
        upper.push_back(Container::Horizontal({
                            RegButtonComp(reg_label, &vis[i++]),
                        }));
        auto vec_content = GetHexDumpElem("Egress Control Vector >>>",
                                          dev->cfg_space_.get() + off + 0x8, vec_bytes, 4);
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, reg_label,
                                         std::move(vec_content), &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("Egress Control Vector +0x8")
                        }));
    }

    return {std::move(upper), std::move(lower)};
}

static const char *DPCTriggerEnaDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "disabled";
    case 0b01:
        return "unmasked uncorr err / ERR_FATAL";
    case 0b10:
        return "unmasked uncorr err / ERR_NONFATAL / ERR_FATAL";
    default:
        return "< reserved >";
    }
}

static const char *DPCTriggerReasonDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "unmasked uncorrectable error";
    case 0b01:
        return "ERR_NONFATAL received";
    case 0b10:
        return "ERR_FATAL received";
    default:
        return "see Trigger Reason Extension";
    }
}

static const char *DPCTriggerReasonExtDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "RP PIO error";
    case 0b01:
        return "DPC Software Trigger";
    default:
        return "< reserved >";
    }
}

// RP PIO Status/Mask/Severity/SysError/Exception registers all share this
// bit layout (Cfg/IO/Mem x UR Cpl/CA Cpl/CTO); @kind labels which one
template <typename T>
static Element DPCRPPIOContent(const T &reg, const char *kind)
{
    return vbox({
        RegFieldCompElem(0, 0, std::format(" Cfg UR Cpl {}", kind), reg.cfg_ur_cpl == 1),
        RegFieldCompElem(1, 1, std::format(" Cfg CA Cpl {}", kind), reg.cfg_ca_cpl == 1),
        RegFieldCompElem(2, 2, std::format(" Cfg CTO {}", kind), reg.cfg_cto == 1),
        RegFieldCompElem(3, 7),
        RegFieldCompElem(8, 8, std::format(" I/O UR Cpl {}", kind), reg.io_ur_cpl == 1),
        RegFieldCompElem(9, 9, std::format(" I/O CA Cpl {}", kind), reg.io_ca_cpl == 1),
        RegFieldCompElem(10, 10, std::format(" I/O CTO {}", kind), reg.io_cto == 1),
        RegFieldCompElem(11, 15),
        RegFieldCompElem(16, 16, std::format(" Mem UR Cpl {}", kind), reg.mem_ur_cpl == 1),
        RegFieldCompElem(17, 17, std::format(" Mem CA Cpl {}", kind), reg.mem_ca_cpl == 1),
        RegFieldCompElem(18, 18, std::format(" Mem CTO {}", kind), reg.mem_cto == 1),
        RegFieldCompElem(19, 31)
    });
}

static capability_comp_ctx
ExtDPCCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
         std::vector<uint8_t> &vis)
{
    Components upper, lower;

    auto off = std::get<3>(cap);
    auto dpc_cap = reinterpret_cast<const DPCCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] DPC", off);

    bool rp_ext = dpc_cap->dpc_cap.rp_ext_for_dpc == 1;
    auto reg_per_cap = rp_ext ? 12 : 4;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("DPC Capability +0x4", &vis[i++]),
                        CapHdrComp(dpc_cap->hdr)
                    }));

    auto &dcap = dpc_cap->dpc_cap;
    auto dpc_cap_content = vbox({
        RegFieldVerbElem(0, 4, " DPC Interrupt Message Number", dcap.dpc_itr_msg_num),
        RegFieldCompElem(5, 5, " RP Extensions for DPC", dcap.rp_ext_for_dpc == 1),
        RegFieldCompElem(6, 6, " Poisoned TLP Egress Blocking Supported", dcap.poisoned_tlp_egress_blk_supported == 1),
        RegFieldCompElem(7, 7, " DPC Software Triggering Supported", dcap.dpc_sw_triggering_supported == 1),
        RegFieldVerbElem(8, 11, " RP PIO Log Size", dcap.rp_pio_log_size),
        RegFieldCompElem(12, 12, " DL_Active ERR_COR Signaling Supported", dcap.dl_active_err_cor_sig_supported == 1),
        RegFieldCompElem(13, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "DPC Capability +0x4",
                                     std::move(dpc_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("DPC Control +0x6", &vis[i++]),
                    }));
    auto &dctl = dpc_cap->dpc_ctl;
    auto dpc_ctl_content = vbox({
        RegFieldVerbElem(0, 1, std::format(" DPC Trigger Enable: {}", DPCTriggerEnaDesc(dctl.dpc_trigger_ena)),
                         dctl.dpc_trigger_ena),
        RegFieldCompElem(2, 2, std::format(" DPC Completion Control: {}",
                                           dctl.dpc_completion_ctl ?
                                           "Unsupported Request (UR)" : "Completer Abort (CA)")),
        RegFieldCompElem(3, 3, " DPC Interrupt Enable", dctl.dpc_itr_ena == 1),
        RegFieldCompElem(4, 4, " DPC ERR_COR Enable", dctl.dpc_err_cor_ena == 1),
        RegFieldCompElem(5, 5, " Poisoned TLP Egress Blocking Enable", dctl.poisoned_tlp_egress_blk_ena == 1),
        RegFieldCompElem(6, 6, " DPC Software Trigger", dctl.dpc_sw_trigger == 1),
        RegFieldCompElem(7, 7, " DL_Active ERR_COR Enable", dctl.dl_active_err_cor_ena == 1),
        RegFieldCompElem(8, 8, " DPC SIG_SFW Enable", dctl.dpc_sig_sfw_ena == 1),
        RegFieldCompElem(9, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "DPC Control +0x6",
                                     std::move(dpc_ctl_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("DPC Status +0x8", &vis[i++]),
                    }));
    auto &dstat = dpc_cap->dpc_status;
    auto dpc_status_content = vbox({
        RegFieldCompElem(0, 0, " DPC Trigger Status", dstat.dpc_trigger_status == 1),
        RegFieldVerbElem(1, 2, std::format(" DPC Trigger Reason: {}", DPCTriggerReasonDesc(dstat.dpc_trigger_reason)),
                         dstat.dpc_trigger_reason),
        RegFieldCompElem(3, 3, " DPC Interrupt Status", dstat.dpc_itr_status == 1),
        RegFieldCompElem(4, 4, " DPC RP Busy", dstat.dpc_rp_busy == 1),
        RegFieldVerbElem(5, 6, std::format(" DPC Trigger Reason Extension: {}",
                                          DPCTriggerReasonExtDesc(dstat.dpc_trigger_reason_ext)),
                         dstat.dpc_trigger_reason_ext),
        RegFieldCompElem(7, 7),
        RegFieldVerbElem(8, 12, " RP PIO First Error Pointer", dstat.rp_pio_first_err_ptr),
        RegFieldCompElem(13, 13, " DPC SIG_SFW Status", dstat.dpc_sig_sfw_status == 1),
        RegFieldCompElem(14, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "DPC Status +0x8",
                                     std::move(dpc_status_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("DPC Error Source ID +0xa", &vis[i++]),
                    }));
    auto dpc_err_src_content = vbox({
        RegFieldVerbElem(0, 15, std::format(" DPC Error Source ID (Requester ID): {:#06x}",
                                           dpc_cap->dpc_err_src_id.req_id),
                         dpc_cap->dpc_err_src_id.req_id)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "DPC Error Source ID +0xa",
                                     std::move(dpc_err_src_content), &vis[i - 1]));

    if (rp_ext) {
        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO Status +0xc", &vis[i++]),
                        }));
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO Status +0xc",
                                         DPCRPPIOContent(dpc_cap->rp_pio_status, "Status"), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO Mask +0x10", &vis[i++]),
                        }));
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO Mask +0x10",
                                         DPCRPPIOContent(dpc_cap->rp_pio_mask, "Mask"), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO Severity +0x14", &vis[i++]),
                        }));
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO Severity +0x14",
                                         DPCRPPIOContent(dpc_cap->rp_pio_severity, "Severity"), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO SysError +0x18", &vis[i++]),
                        }));
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO SysError +0x18",
                                         DPCRPPIOContent(dpc_cap->rp_pio_syserror, "SysError"), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO Exception +0x1c", &vis[i++]),
                        }));
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO Exception +0x1c",
                                         DPCRPPIOContent(dpc_cap->rp_pio_exception, "Exception"), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO Header Log +0x20", &vis[i++]),
                        }));
        std::array<uint8_t, sizeof(RegAERHdrLog)> hdr_log;
        uint32_t *hdr_dw = reinterpret_cast<uint32_t *>(hdr_log.data());
        for (int n = 0; n < 4; n++)
            *hdr_dw++ = std::byteswap(dpc_cap->rp_pio_hdr_log.dw[n]);
        auto hdr_log_content = GetHexDumpElem("RP PIO TLP hdr >>>", hdr_log.data(), hdr_log.size(), 4);
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO Header Log +0x20",
                                         std::move(hdr_log_content), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO ImpSpec Log +0x30", &vis[i++]),
                        }));
        auto impspec_content = vbox({
            RegFieldVerbElem(0, 31, std::format(" RP PIO ImpSpec Log: {:#010x}", dpc_cap->rp_pio_impspec_log.log),
                             dpc_cap->rp_pio_impspec_log.log)
        });
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO ImpSpec Log +0x30",
                                         std::move(impspec_content), &vis[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("RP PIO TLP Prefix Log +0x34", &vis[i++]),
                        }));
        std::array<uint8_t, sizeof(RegAERTlpPrefixLog)> tlp_pref_log;
        uint32_t *tlp_dw = reinterpret_cast<uint32_t *>(tlp_pref_log.data());
        for (int n = 0; n < 4; n++)
            *tlp_dw++ = std::byteswap(dpc_cap->rp_pio_tlp_pref_log.dw[n]);
        auto tlp_pref_log_content = GetHexDumpElem("RP PIO e2e TLP prefix >>>",
                                                   tlp_pref_log.data(), tlp_pref_log.size(), 4);
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "RP PIO TLP Prefix Log +0x34",
                                         std::move(tlp_pref_log_content), &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("RP PIO Status +0xc"),
                            EmptyCapRegComp("RP PIO Mask +0x10")
                        }));
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("RP PIO Severity +0x14"),
                            EmptyCapRegComp("RP PIO SysError +0x18")
                        }));
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("RP PIO Exception +0x1c"),
                            EmptyCapRegComp("RP PIO Header Log +0x20")
                        }));
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("RP PIO ImpSpec Log +0x30"),
                            EmptyCapRegComp("RP PIO TLP Prefix Log +0x34")
                        }));
    }

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
ExtATSCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
         std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 2;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto ats_cap = reinterpret_cast<const ATSCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] ATS", off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("ATS Capability +0x4", &vis[i++]),
                        CapHdrComp(ats_cap->hdr)
                    }));

    auto &acap = ats_cap->ats_cap;
    auto ats_cap_content = vbox({
        RegFieldVerbElem(0, 4, std::format(" Invalidate Queue Depth: {}",
                                          acap.invalidate_queue_depth == 0 ? 32 : acap.invalidate_queue_depth),
                         acap.invalidate_queue_depth),
        RegFieldCompElem(5, 5, " Page Aligned Request", acap.page_aligned_req == 1),
        RegFieldCompElem(6, 6, " Global Invalidate Supported", acap.global_invalidate_supported == 1),
        RegFieldCompElem(7, 7, " Relaxed Ordering Supported", acap.relaxed_ordering_supported == 1),
        RegFieldCompElem(8, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "ATS Capability +0x4",
                                     std::move(ats_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("ATS Control +0x6", &vis[i++]),
                    }));
    auto &actl = ats_cap->ats_ctl;
    auto ats_ctl_content = vbox({
        RegFieldVerbElem(0, 4, std::format(" Smallest Translation Unit: {} x 4096-byte block(s)",
                                          1u << actl.stu),
                         actl.stu),
        RegFieldCompElem(5, 14),
        RegFieldCompElem(15, 15, " Enable", actl.ena == 1)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "ATS Control +0x6",
                                     std::move(ats_ctl_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static capability_comp_ctx
ExtPageReqIfaceCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                   std::vector<uint8_t> &vis)
{
    Components upper, lower;
    constexpr auto reg_per_cap = 4;
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto off = std::get<3>(cap);
    auto pri_cap = reinterpret_cast<const PageReqIfaceCap *>(dev->cfg_space_.get() + off);
    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] Page Request Interface", off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Page Request Control +0x4", &vis[i++]),
                        CapHdrComp(pri_cap->hdr)
                    }));

    auto &pctl = pri_cap->pri_ctl;
    auto pri_ctl_content = vbox({
        RegFieldCompElem(0, 0, " Enable", pctl.enable == 1),
        RegFieldCompElem(1, 1, " Reset", pctl.reset == 1),
        RegFieldCompElem(2, 15)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Page Request Control +0x4",
                                     std::move(pri_ctl_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Page Request Status +0x6", &vis[i++]),
                    }));
    auto &pstat = pri_cap->pri_status;
    auto pri_status_content = vbox({
        RegFieldCompElem(0, 0, " Response Failure", pstat.response_failure == 1),
        RegFieldCompElem(1, 1, " Unexpected Page Request Group Index", pstat.unexpected_prg_idx == 1),
        RegFieldCompElem(2, 7),
        RegFieldCompElem(8, 8, " Stopped", pstat.stopped == 1),
        RegFieldCompElem(9, 14),
        RegFieldCompElem(15, 15, " PRG Response PASID Required", pstat.prg_resp_pasid_required == 1)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Page Request Status +0x6",
                                     std::move(pri_status_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Outstanding Page Request Capacity +0x8", &vis[i++]),
                    }));
    auto capacity_content = vbox({
        RegFieldVerbElem(0, 31, " Outstanding Page Request Capacity",
                         pri_cap->outstanding_req_capacity.outstanding_req_capacity)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Outstanding Page Request Capacity +0x8",
                                     std::move(capacity_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Outstanding Page Request Allocation +0xc", &vis[i++]),
                    }));
    auto allocation_content = vbox({
        RegFieldVerbElem(0, 31, " Outstanding Page Request Allocation",
                         pri_cap->outstanding_req_allocation.outstanding_req_allocation)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "Outstanding Page Request Allocation +0xc",
                                     std::move(allocation_content), &vis[i - 1]));

    return {std::move(upper), std::move(lower)};
}

static const char *TPHSTTableLocationDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "not present";
    case 0b01:
        return "in this capability structure";
    case 0b10:
        return "in the MSI-X Table";
    default:
        return "< reserved >";
    }
}

static const char *TPHReqEnaDesc(uint8_t val)
{
    switch (val) {
    case 0b00:
        return "not permitted to issue TPH or Extended TPH";
    case 0b01:
        return "TPH permitted, Extended TPH not permitted";
    case 0b11:
        return "TPH and Extended TPH permitted";
    default:
        return "< reserved >";
    }
}

static capability_comp_ctx
ExtTPHRequesterCap(const pci::PciDevBase *dev, const pci::CapDesc &cap,
                   std::vector<uint8_t> &vis)
{
    Components upper, lower;

    auto off = std::get<3>(cap);
    auto tph_cap = reinterpret_cast<const TPHRequesterCap *>(dev->cfg_space_.get() + off);

    bool st_table_here = tph_cap->tph_cap.st_table_location == 0b01;
    uint32_t st_entries = st_table_here ? tph_cap->tph_cap.st_table_size + 1u : 0;

    auto reg_per_cap = 2 + (st_table_here ? 1 : 0);
    size_t i = vis.size();
    std::ranges::fill_n(std::back_inserter(vis), reg_per_cap, 0);

    auto reg_info_cap_hdr = std::format("[extended][{:#02x}] TPH Requester", off);

    upper.push_back(CapDelimComp(cap));
    upper.push_back(Container::Horizontal({
                        RegButtonComp("TPH Requester Capability +0x4", &vis[i++]),
                        CapHdrComp(tph_cap->hdr)
                    }));

    auto &tcap = tph_cap->tph_cap;
    auto tph_cap_content = vbox({
        RegFieldCompElem(0, 0, " No ST Mode Supported", tcap.no_st_mode_supported == 1),
        RegFieldCompElem(1, 1, " Interrupt Vector Mode Supported", tcap.interrupt_vector_mode_supported == 1),
        RegFieldCompElem(2, 2, " Device Specific Mode Supported", tcap.device_specific_mode_supported == 1),
        RegFieldCompElem(3, 7),
        RegFieldCompElem(8, 8, " Extended TPH Requester Supported", tcap.extended_tph_req_supported == 1),
        RegFieldVerbElem(9, 10, std::format(" ST Table Location: {}",
                                           TPHSTTableLocationDesc(tcap.st_table_location)),
                         tcap.st_table_location),
        RegFieldCompElem(11, 15),
        RegFieldVerbElem(16, 26, std::format(" ST Table Size: {} entries", tcap.st_table_size + 1u),
                         tcap.st_table_size),
        RegFieldCompElem(27, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "TPH Requester Capability +0x4",
                                     std::move(tph_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("TPH Requester Control +0x8", &vis[i++]),
                    }));
    auto &tctl = tph_cap->tph_ctl;
    auto tph_ctl_content = vbox({
        RegFieldVerbElem(0, 2, std::format(" ST Mode Select: {}",
                                          tctl.st_mode_select == 0b000 ? "No ST Mode" :
                                          tctl.st_mode_select == 0b001 ? "Interrupt Vector Mode" :
                                          tctl.st_mode_select == 0b010 ? "Device Specific Mode" :
                                          "< reserved >"),
                         tctl.st_mode_select),
        RegFieldCompElem(3, 7),
        RegFieldVerbElem(8, 9, std::format(" TPH Requester Enable: {}", TPHReqEnaDesc(tctl.tph_req_ena)),
                         tctl.tph_req_ena),
        RegFieldCompElem(10, 31)
    });
    lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, "TPH Requester Control +0x8",
                                     std::move(tph_ctl_content), &vis[i - 1]));

    if (st_table_here) {
        auto reg_label = std::format("ST Table [{} entries] +0xc", st_entries);
        upper.push_back(Container::Horizontal({
                            RegButtonComp(reg_label, &vis[i++]),
                        }));

        Elements st_lines;
        for (uint32_t n = 0; n < st_entries; n++) {
            auto entry = reinterpret_cast<const RegTPHSTEntry *>
                        (dev->cfg_space_.get() + off + 0xc + n * sizeof(RegTPHSTEntry));
            st_lines.push_back(text(std::format("  [{:>3}] ST Lower: {:#04x}  ST Upper: {:#04x}",
                                                n, entry->st_lower, entry->st_upper)));
        }
        auto st_table_content = vbox(std::move(st_lines));
        lower.push_back(CreateCapRegInfo(reg_info_cap_hdr, reg_label,
                                         std::move(st_table_content), &vis[i - 1]));
    } else {
        upper.push_back(Container::Horizontal({
                            EmptyCapRegComp("ST Table +0xc")
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
        return ExtACSCap(dev, cap, vis);
    case ExtCapID::ari:
        return ExtARICap(dev, cap, vis);
    case ExtCapID::ats:
        return ExtATSCap(dev, cap, vis);
    case ExtCapID::sriov:
        return ExtSRIOVCap(dev, cap, vis);
    case ExtCapID::mriov:
        return NotImplCap();
    case ExtCapID::mcast:
        return NotImplCap();
    case ExtCapID::page_req_iface:
        return ExtPageReqIfaceCap(dev, cap, vis);
    case ExtCapID::amd_rsvd:
        return NotImplCap();
    case ExtCapID::res_bar:
        return ExtResizableBARCap(dev, cap, vis);
    case ExtCapID::dpa:
        return NotImplCap();
    case ExtCapID::tph_req:
        return ExtTPHRequesterCap(dev, cap, vis);
    case ExtCapID::ltr:
        return ExtLTRCap(dev, cap, vis);
    case ExtCapID::sec_pcie:
        return ExtSecondaryPCIECap(dev, cap, vis);
    case ExtCapID::pmux:
        return NotImplCap();
    case ExtCapID::pasid:
        return ExtPASIDCap(dev, cap, vis);
    case ExtCapID::lnr:
        return NotImplCap();
    case ExtCapID::dpc:
        return ExtDPCCap(dev, cap, vis);
    case ExtCapID::l1_pm_substates:
        return ExtL1PMSubstatesCap(dev, cap, vis);
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
