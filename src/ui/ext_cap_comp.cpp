// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#include "ext_cap_comp.h"
#include "log.h"
#include "util.h"

#include <cassert>
#include <bitset>

#include <fmt/format.h>

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
        RegFieldVerbElem(9, 15, fmt::format(" Enable lower SKP OS gen vector: {}",
                                            EnableLowerSKPOSGenVecDesc(sec_pcie_cap->link_ctl3.lower_skp_os_gen_vec_ena)),
                         sec_pcie_cap->link_ctl3.lower_skp_os_gen_vec_ena),
        RegFieldCompElem(16, 31)
    });
    lower.push_back(CreateCapRegInfo(fmt::format("[extended][{:#02x}] Secondary PCIe", off),
                                     "Link Control 3 +0x4",
                                     std::move(link_ctl3_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Lane Error Status +0x8", &vis[i++]),
                    }));

    auto lane_err_status_content = vbox({
        RegFieldVerbElem(0, 31, fmt::format(" Lane(s) with error detected: {:#04x}",
                                            sec_pcie_cap->lane_err_stat.lane_err_status),
                         sec_pcie_cap->lane_err_stat.lane_err_status),
    });
    lower.push_back(CreateCapRegInfo(fmt::format("[extended][{:#02x}] Secondary PCIe", off),
                                     "Lane Error Status +0x8",
                                     std::move(lane_err_status_content), &vis[i - 1]));

    // Check of port supports 8.0GT/s link speed or higher
    if (pcie_cap->link_cap2.supported_speed_vec & 0x4) {
        upper.push_back(
                Container::Horizontal({
                    RegButtonComp(fmt::format("Lane Equalization Control [{} lane(s)] +0xc",
                                  max_link_width), &vis[i++]),
        }));

        for (uint32_t cur_link = 0; cur_link < max_link_width; cur_link++) {
            auto lane_eq_ctl_reg = reinterpret_cast<const RegLaneEqCtl *>
                (dev->cfg_space_.get() + off + cur_link * sizeof(RegLaneEqCtl));
            auto lane_eq_ctl_content = vbox({
                RegFieldVerbElem(0, 3,
                    fmt::format(" Downstream port 8GT/s transmitter preset: {}",
                                TransPresHint8gtsDesc(lane_eq_ctl_reg->ds_port_8gts_trans_pres)),
                                lane_eq_ctl_reg->ds_port_8gts_trans_pres
                ),
                RegFieldVerbElem(4, 6,
                    fmt::format(" Downstream port 8GT/s receiver preset: {}",
                                RecvPresHint8gtsDesc(lane_eq_ctl_reg->ds_port_8gts_recv_pres_h)),
                                lane_eq_ctl_reg->ds_port_8gts_recv_pres_h
                ),
                RegFieldCompElem(7, 7),
                RegFieldVerbElem(8, 11,
                    fmt::format(" Upstream port 8GT/s transmitter preset: {}",
                                TransPresHint8gtsDesc(lane_eq_ctl_reg->us_port_8gts_trans_pres)),
                                lane_eq_ctl_reg->us_port_8gts_trans_pres
                ),
                RegFieldVerbElem(4, 6,
                    fmt::format(" Upstream port 8GT/s receiver preset: {}",
                                RecvPresHint8gtsDesc(lane_eq_ctl_reg->us_port_8gts_recv_pres_h)),
                                lane_eq_ctl_reg->us_port_8gts_recv_pres_h
                ),
                RegFieldCompElem(15, 15),
            });
            lower.push_back(
                    CreateCapRegInfo(fmt::format("[extended][{:#02x}] Secondary PCIe", off),
                                     fmt::format("Lane #{} Equalization Control +{:#01x}",
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
                         fmt::format(" Local data link feature(s): Local Scaled Flow Ctl[{}]",
                                local_dlink_feat_map[0] ? '+' : '-'),
                         dlink_feature_cap->dlink_feat_cap.local_data_link_feat_supp),
        RegFieldCompElem(23, 30),
        RegFieldCompElem(31, 31, "Data link feature exchange enable",
                         dlink_feature_cap->dlink_feat_cap.data_link_feat_xchg_ena == 1)
    });
    lower.push_back(
            CreateCapRegInfo(fmt::format(
                                "[extended][{:#02x}] Data Link Feature", off),
                             "Data Link Feature Capabilities +0x4",
                             std::move(dlink_feature_caps_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("Data Link Feature Status +0x8", &vis[i++]),
                    }));

    std::bitset<23> rem_dlink_feat_map {dlink_feature_cap->dlink_feat_stat.rem_data_link_feat_supp};

    auto dlink_feature_stat_content = vbox({
        RegFieldVerbElem(0, 22,
                         fmt::format(" Remote data link feature(s): Remote Scaled Flow Ctl[{}]",
                                rem_dlink_feat_map[0] ? '+' : '-'),
                         dlink_feature_cap->dlink_feat_stat.rem_data_link_feat_supp),
        RegFieldCompElem(23, 30),
        RegFieldCompElem(31, 31, "Remote Data link feature supported valid",
                         dlink_feature_cap->dlink_feat_stat.rem_data_link_feat_supp_valid == 1)
    });
    lower.push_back(
            CreateCapRegInfo(fmt::format(
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
        RegFieldCompElem(8, 15, fmt::format(" Next function: {:#0x}",
                                            ari_cap->ari_cap.next_func_num))
    });
    lower.push_back(
            CreateCapRegInfo(fmt::format("[extended][{:#02x}] ARI", off),
                             "ARI Capabilities +0x4",
                             std::move(ari_cap_content), &vis[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp("ARI Control +0x6", &vis[i++]),
                    }));
    auto ari_ctl_content = vbox({
        RegFieldCompElem(0, 0, " MFVC function groups enable", ari_cap->ari_ctl.mfvc_func_grps_ena == 1),
        RegFieldCompElem(1, 1, " ACS function groups enable", ari_cap->ari_ctl.acs_func_grps_ena == 1),
        RegFieldCompElem(2, 3),
        RegFieldCompElem(4, 6, fmt::format(" Function group: {:#01x}",
                                            ari_cap->ari_ctl.func_grp)),
        RegFieldCompElem(7, 15),
    });
    lower.push_back(
            CreateCapRegInfo(fmt::format("[extended][{:#02x}] ARI", off),
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
        RegFieldCompElem(8, 12, fmt::format(" Max PASID width: {:#01x}",
                                            pasid_cap->pasid_cap.max_pasid_width)),
        RegFieldCompElem(13, 15)
    });
    lower.push_back(
            CreateCapRegInfo(fmt::format("[extended][{:#02x}] PASID", off),
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
            CreateCapRegInfo(fmt::format("[extended][{:#02x}] PASID", off),
                             "PASID Control +0x6",
                             std::move(pasid_ctl_content), &vis[i - 1]));

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
        return NotImplCap();
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
