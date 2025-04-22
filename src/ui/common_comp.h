// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include "pci_dev.h"
#include "pci_regs.h"
#include <ftxui/component/component.hpp>

namespace ui {

// XXX: This is essentialy the same as ftxui::ButtonBase with the couple of differences:
// - it can track pressed/released state the same way ftxui::Checkbox does.
// - it can be switched on and off externally by modifying @ena_flag_ flag
// TODO: merge to mainline
class PushPullButton : public ftxui::ComponentBase, public ftxui::ButtonOption
{
public:
    explicit PushPullButton(ButtonOption option, uint8_t *on_click)
        : ButtonOption(std::move(option)),
          ena_flag_(on_click)
    {}

    ftxui::Element OnRender() override;

    ftxui::Decorator AnimatedColorStyle();
    void SetAnimationTarget(float target);
    void OnAnimation(ftxui::animation::Params& p) override;
    void OnClick();
    bool OnEvent(ftxui::Event event) override;
    bool OnMouseEvent(ftxui::Event event);
    bool Focusable() const final { return true; }

private:
    bool is_pressed_ = false;
    bool mouse_hover_ = false;
    uint8_t *ena_flag_;
    ftxui::Box box_;
    ftxui::ButtonOption option_;
    float animation_background_ = 0;
    float animation_foreground_ = 0;
    ftxui::animation::Animator animator_background_ =
      ftxui::animation::Animator(&animation_background_);
    ftxui::animation::Animator animator_foreground_ =
      ftxui::animation::Animator(&animation_foreground_);
};

ftxui::Component RegButtonComp(std::string label, uint8_t *on_click = nullptr);

// Compact register field element
ftxui::Element
RegFieldCompElem(const uint8_t fb, const uint8_t lb, std::string desc = " - ",
                 bool should_highlight = false);

// Verbose register field element
ftxui::Element
RegFieldVerbElem(const uint8_t fb, const uint8_t lb, std::string desc,
                 uint16_t val);

// Create component out of Element, which would only be shown if @on_click == 1
ftxui::Component
GetCompMaybe(ftxui::Element elem, const uint8_t *on_click);

// Create an element representing a hex dump of some buffer
ftxui::Element
GetHexDumpElem(std::string desc, const uint8_t *buf, const size_t len,
               uint32_t bytes_per_line = 16);

// Holds two vectors of components for upper and lower parts of the split
typedef std::pair<ftxui::Components, ftxui::Components> capability_comp_ctx;

[[maybe_unused]] inline capability_comp_ctx NotImplCap() { return {{}, {}}; }
[[maybe_unused]] inline ftxui::Component NotImplComp()
{
    return ftxui::Renderer([&] { return ftxui::text("< not impl >") | ftxui::bold; });
}

// Create components for type0/type1 config space header
capability_comp_ctx
GetCompatHeaderRegsComponents(const pci::PciDevBase *dev, std::vector<uint8_t> &vis_state_vt);

// Create particular capability delimiter
ftxui::Component
CapDelimComp(const pci::CapDesc &cap);

// Capabilities region delimiter component
ftxui::Component
CapsDelimComp(const pci::CapType type, const uint8_t caps_num);

using cap_hdr_type_t = std::variant<CompatCapHdr, ExtCapHdr>;
// Capability header component
ftxui::Component
CapHdrComp(const cap_hdr_type_t cap_hdr);

// Create component representing inactive/undefined register within capability
ftxui::Component
EmptyCapRegComp(const std::string desc);

// Create component which incapsulates verbose register information within capability
ftxui::Component
CreateCapRegInfo(const std::string &cap_desc, const std::string &cap_reg,
                 ftxui::Element content, const uint8_t *on_click);

} // namespace ui
