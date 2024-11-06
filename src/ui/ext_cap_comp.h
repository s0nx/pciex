// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include "common_comp.h"

namespace ui {

// Create clickable components and descriptions for capability
// in PCI-compatible config space (first 256 bytes)
// Each capability might be composed of multiple registers.
capability_comp_ctx
GetExtendedCapComponents(const pci::PciDevBase *dev, const ExtCapID cap_id,
                         const pci::CapDesc &cap, std::vector<uint8_t> &vis);

} // namespace ui
