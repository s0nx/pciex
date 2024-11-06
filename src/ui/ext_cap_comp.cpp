// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#include <cassert>

#include "ext_cap_comp.h"
#include "log.h"

extern Logger logger;

using namespace ftxui;

namespace ui {

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
        return NotImplCap();
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
        return NotImplCap();
    case ExtCapID::pmux:
        return NotImplCap();
    case ExtCapID::pasid:
        return NotImplCap();
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
        return NotImplCap();
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
