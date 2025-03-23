// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Petr Vyazovik <xen@f-m.fm>

#include <variant>
#include <fmt/format.h>

#include "common_comp.h"

#include "ftxui/component/event.hpp"

using namespace ftxui;

namespace ui {

//
// push-pull button stuff
//
Element PushPullButton::OnRender()
{
    const bool active = Active();
    const bool focused = Focused();
    const bool focused_or_hover = focused || mouse_hover_;

    float target = focused_or_hover ? 1.f : 0.f;  // NOLINT
    if (target != animator_background_.to()) {
      SetAnimationTarget(target);
    }

    auto focus_mgmt = focused ? ftxui::focus :
                      active ? ftxui::select :
                      ftxui::nothing;

    const EntryState state = {
        *label,
        is_pressed_,
        active,
        focused_or_hover,
        -1
    };

    auto element = transform(state);
    return element | AnimatedColorStyle() | focus_mgmt | reflect(box_);
}

Decorator PushPullButton::AnimatedColorStyle()
{
    Decorator style = nothing;
    if (animated_colors.background.enabled) {
      style = style | bgcolor(Color::Interpolate(animation_foreground_,  //
                                                animated_colors.background.inactive,
                                                animated_colors.background.active));
    }

    if (animated_colors.foreground.enabled) {
        style = style |
            color(Color::Interpolate(animation_foreground_,  //
                                           animated_colors.foreground.inactive,
                                           animated_colors.foreground.active));
    }
    return style;
}

void PushPullButton::SetAnimationTarget(float target) {
    if (animated_colors.foreground.enabled) {
      animator_foreground_ = animation::Animator(
          &animation_foreground_, target, animated_colors.foreground.duration,
          animated_colors.foreground.function);
    }
    if (animated_colors.background.enabled) {
      animator_background_ = animation::Animator(
          &animation_background_, target, animated_colors.background.duration,
          animated_colors.background.function);
    }
}

void PushPullButton::OnAnimation(animation::Params& p)
{
    animator_background_.OnAnimation(p);
    animator_foreground_.OnAnimation(p);
}

void PushPullButton::OnClick()
{
    on_click();
    animation_background_ = 0.5F;
    animation_foreground_ = 0.5F;
    SetAnimationTarget(1.F);
}

bool PushPullButton::OnEvent(Event event)
{
    if (event.is_mouse()) {
        return OnMouseEvent(event);
    }

    if (event == Event::Return) {
        is_pressed_ = !is_pressed_;
        OnClick();
        return true;
    }
    return false;
}

bool PushPullButton::OnMouseEvent(Event event)
{
    mouse_hover_ =
        box_.Contain(event.mouse().x, event.mouse().y) && CaptureMouse(event);

    if (!mouse_hover_) {
        return false;
    }

    if (event.mouse().button == Mouse::Left &&
        event.mouse().motion == Mouse::Pressed) {
        is_pressed_ = !is_pressed_;
        TakeFocus();
        OnClick();
        return true;
    }

    return false;
}

// Custom button style is necessary to make the button flexible
// within the container
// TODO: configurable colors
static ButtonOption RegButtonDefaultOption() {
    auto option = ButtonOption::Animated(Color::Grey15, Color::Cornsilk1);
    option.transform = [](const EntryState& s) {
        auto element = text(s.label);
        if (s.focused) {
            element |= bold;
        }

        element |= center;
        element |= border;
        element |= flex;

        if (s.state)
            element |= bgcolor(Color::LightSalmon1) |
                       color(Color::Grey15);
        return element;
    };
    return option;
}

static Component
PPButton(ConstStringRef label, std::function<void()> on_click,
         ButtonOption option)
{
  option.label = label;
  option.on_click = std::move(on_click);
  return Make<PushPullButton>(std::move(option));
}

Component RegButtonComp(std::string label, uint8_t *on_click)
{
    if (on_click == nullptr)
        return PPButton(label, [] {}, ui::RegButtonDefaultOption());
    else
        return PPButton(label, [on_click] { *on_click = *on_click ? false : true; },
                        ui::RegButtonDefaultOption());
}






////////////////////////

Element
RegFieldCompElem(const uint8_t fb, const uint8_t lb, std::string desc,
                 bool should_highlight)
{
    auto field_pos_desc = text(fmt::format("[{:>2} : {:<2}]", fb, lb));

    if (should_highlight)
        field_pos_desc |= bgcolor(Color::Green) |
                          color(Color::Grey15);
    auto desc_text = text(desc);

    if (desc == " - ") {
        field_pos_desc |= dim;
        desc_text |= dim;
    }

    auto elem = hbox({
        std::move(field_pos_desc),
        separator(),
        std::move(desc_text)
    });

    return elem;
}

Element
RegFieldVerbElem(const uint8_t fb, const uint8_t lb, std::string desc,
                 uint16_t val)
{
    auto field_pos_desc = text(fmt::format("[{:>2} : {:<2}]", fb, lb));

    auto field_line = hbox({
        std::move(field_pos_desc),
        separator(),
        separatorEmpty(),
        text(fmt::format("{:#08b}", val)) | bold |
             bgcolor(Color::Blue) | color(Color::Grey15),
        separatorEmpty(),
        ftxui::text("|"),
        separatorEmpty(),
        text(fmt::format("{:#02x}", val)) | bold |
             bgcolor(Color::Green) | color(Color::Grey15),
    });

    auto desc_line = hbox({
        text("         "),
        separator(),
        text(desc) | bold
    });

    auto elem = vbox({
        hbox({
            text("         "),
            separator(),
            separatorEmpty(),
            separator() | dim | flex,
            separatorEmpty(),
        }),
        std::move(field_line),
        std::move(desc_line),
        hbox({
            text("         "),
            separator(),
            separatorEmpty(),
            separator() | dim | flex,
            separatorEmpty(),
        })
    });

    return elem;
}

using compat_reg_type_t = std::variant<Type0Cfg, Type1Cfg>;
static std::string RegTypeLabel(const compat_reg_type_t reg_type)
{
    std::string reg_label;
    std::visit([&] (auto &&reg) {
        using T = std::decay_t<decltype(reg)>;
        if constexpr (std::is_same_v<T, Type0Cfg>)
            reg_label = fmt::format("{} ({:#02x})", Type0RegName(reg), e_to_type(reg));
        else
            reg_label = fmt::format("{} ({:#02x})", Type1RegName(reg), e_to_type(reg));
    }, reg_type);
    return reg_label;
}

Component
GetCompMaybe(Element elem, const uint8_t *on_click)
{
    return Renderer([=] {
                return vbox({std::move(elem), separatorEmpty()});
           }) | Maybe([on_click] { return *on_click == 1; });
}

Element
GetHexDumpElem(std::string desc, const uint8_t *buf, const size_t len,
               uint32_t bytes_per_line)
{
    Elements lines;

    auto buf_desc = text(desc) | bold;
    lines.push_back(std::move(buf_desc));

    if (len != 0) {
        const auto line_buf_size = 2 + 4 + bytes_per_line * 3 + 3;
        std::vector<char> line_buf(line_buf_size);
        std::vector<char> text_buf(bytes_per_line);

        auto line_complete = [&] {
            fmt::format_to(std::back_inserter(line_buf), " | ");
            auto text_dump = text(std::string(text_buf.begin(), text_buf.end())) |
                             bgcolor(Color::Grey15) | color(Color::Green);
            auto bytes = text(std::string(line_buf.begin(), line_buf.end()));
            lines.push_back(hbox({
                std::move(bytes),
                std::move(text_dump)
            }));
            line_buf.clear();
            text_buf.clear();
        };


        size_t i;
        for (i = 0; i < len; i++) {
            if ((i % bytes_per_line) == 0) {
                if (i != 0)
                    line_complete();

                fmt::format_to(std::back_inserter(line_buf), " {:04x} ", i);
            }

            fmt::format_to(std::back_inserter(line_buf), " {:02x}", buf[i]);
            fmt::format_to(std::back_inserter(text_buf), "{:c}",
                           std::isprint(buf[i]) ? buf[i] : '.');
        }

        while ((i++ % bytes_per_line) != 0)
            line_buf.insert(line_buf.end(), 3, ' ');

        if ((i % bytes_per_line) != 0)
            line_complete();
    }

    return vbox(std::move(lines));
}

static Component
CreateRegInfoCompat(const compat_reg_type_t reg_type, Element content,
                    const uint8_t *on_click)
{
    auto title = text(fmt::format("Compat Cfg Space Hdr -> {}", RegTypeLabel(reg_type))) |
                             inverted | align_right | bold;
    auto info_window = window(std::move(title), std::move(content));
    return GetCompMaybe(std::move(info_window), on_click);
}

static Component
RegInfoVIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = text(fmt::format("[{:02x}] -> {}", dev->get_vendor_id(),
                                        dev->ids_names_[pci::VENDOR].empty() ?
                                        "( empty )" : dev->ids_names_[pci::VENDOR]));
    return CreateRegInfoCompat(Type0Cfg::vid, std::move(content), on_click);
}

static Component
RegInfoDevIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = text(fmt::format("[{:02x}] -> {}", dev->get_device_id(),
                                        dev->ids_names_[pci::DEVICE].empty() ?
                                        "( empty )" : dev->ids_names_[pci::DEVICE]));
    return CreateRegInfoCompat(Type0Cfg::dev_id, std::move(content), on_click);
}

static Component
RegInfoCommandComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto command = dev->get_command();
    auto reg = reinterpret_cast<const RegCommand *>(&command);

    auto content = vbox({
        RegFieldCompElem(0, 0, " i/o space enabled", reg->io_space_ena == 1),
        RegFieldCompElem(1, 1, " mem space enabled", reg->mem_space_ena == 1),
        RegFieldCompElem(2, 2, " bus master enabled", reg->bus_master_ena == 1),
        RegFieldCompElem(3, 5),
        RegFieldCompElem(6, 6, " parity err response", reg->parity_err_resp == 1),
        RegFieldCompElem(7, 7),
        RegFieldCompElem(8, 8, " serr# enabled", reg->serr_ena == 1),
        RegFieldCompElem(9, 9),
        RegFieldCompElem(10, 10, " intr disabled", reg->itr_disable == 1),
        RegFieldCompElem(11, 15)
    });

    return CreateRegInfoCompat(Type0Cfg::command, std::move(content), on_click);
}

static Component
RegInfoStatusComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto status = dev->get_status();
    auto reg = reinterpret_cast<const RegStatus *>(&status);

    auto content = vbox({
        RegFieldCompElem(0, 0, " immediate readiness", reg->imm_readiness == 1),
        RegFieldCompElem(1, 2),
        RegFieldCompElem(3, 3, " interrupt status", reg->itr_status == 1),
        RegFieldCompElem(4, 4, " capabilities list", reg->cap_list == 1),
        RegFieldCompElem(5, 7),
        RegFieldCompElem(8, 8, " master data parity error", reg->master_data_parity_err == 1),
        RegFieldCompElem(9, 10),
        RegFieldCompElem(11, 11, " signaled target abort", reg->signl_tgt_abort == 1),
        RegFieldCompElem(12, 12, " received target abort", reg->received_tgt_abort == 1),
        RegFieldCompElem(13, 13, " received master abort", reg->recevied_master_abort == 1),
        RegFieldCompElem(14, 14, " signaled system error", reg->signl_sys_err == 1),
        RegFieldCompElem(15, 15, " detected parity error", reg->detected_parity_err == 1),
    });

    return CreateRegInfoCompat(Type0Cfg::status, std::move(content), on_click);
}

static Component
RegInfoRevComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = text(fmt::format("{:02x}", dev->get_rev_id()));
    return CreateRegInfoCompat(Type0Cfg::revision, std::move(content), on_click);
}

static Component
RegInfoCCComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto cc = dev->get_class_code();

    auto cname = dev->ids_names_[pci::CLASS];
    auto sub_cname = dev->ids_names_[pci::SUBCLASS];
    auto prog_iface = dev->ids_names_[pci::PROG_IFACE];

    auto reg = reinterpret_cast<RegClassCode *>(&cc);

    auto content = vbox({
        hbox({
            hbox({
                text(fmt::format("{:02x}", reg->base_class_code)) | bold,
                separatorLight(),
                text(fmt::format("{:02x}", reg->sub_class_code)) | bold,
                separatorLight(),
                text(fmt::format("{:02x}", reg->prog_iface)) | bold,
            }) | borderStyled(ROUNDED, Color::Blue),
            filler()
        }),
        separatorEmpty(),
        vbox({
            text(fmt::format("     class: {:02x} -> {}", reg->base_class_code, cname)),
            text(fmt::format("  subclass: {:02x} -> {}", reg->sub_class_code, sub_cname)),
            text(fmt::format("prog-iface: {:02x} -> {}", reg->prog_iface, prog_iface))
        })
    });

    return CreateRegInfoCompat(Type0Cfg::class_code, std::move(content), on_click);
}

static Component
RegInfoClSizeComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = text(fmt::format("Cache Line size: {} bytes",
                               dev->get_cache_line_size() * 4));
    return CreateRegInfoCompat(Type0Cfg::cache_line_size, std::move(content), on_click);
}

static Component
RegInfoLatTmrComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = text(fmt::format("Latency Tmr: {:02x}", dev->get_lat_timer()));
    return CreateRegInfoCompat(Type0Cfg::latency_timer, std::move(content), on_click);
}

static Component
RegInfoHdrTypeComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto hdr_type = dev->get_header_type();
    auto reg = reinterpret_cast<const RegHdrType *>(&hdr_type);
    auto desc = fmt::format(" header layout: {}",
                            reg->hdr_layout ? "Type 1" : "Type 0");

    auto content = vbox({
        RegFieldCompElem(0, 6, desc),
        RegFieldCompElem(7, 7, " multi-function device", reg->is_mfd == 1)
    });
    return CreateRegInfoCompat(Type0Cfg::header_type, std::move(content), on_click);
}

static Component
RegInfoBISTComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto bist = dev->get_bist();
    auto reg = reinterpret_cast<const RegBIST *>(&bist);

    auto content = vbox({
        RegFieldCompElem(0, 3, fmt::format(" completion code: {}", reg->cpl_code)),
        RegFieldCompElem(4, 5),
        RegFieldCompElem(6, 6, " start BIST", reg->start_bist == 1),
        RegFieldCompElem(7, 7, " BIST capable", reg->bist_cap == 1)
    });

    return CreateRegInfoCompat(Type0Cfg::header_type, std::move(content), on_click);
}

enum class UIBarElemType
{
    Empty,
    IoSpace,
    Memory,
    Exp
};

static Element
GetBarElem(UIBarElemType type, uint32_t bar)
{
    Element reg_box;

    if (type == UIBarElemType::Empty) {
        reg_box = hbox({
            hbox({
                text(fmt::format("{:032b}", bar)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
            }) | border,
            filler()
        });
    } else if (type == UIBarElemType::IoSpace) {
        auto reg = reinterpret_cast<const RegBARIo *>(&bar);

        // double hbox is needed to align element
        reg_box = hbox({
            hbox({
                text(fmt::format("{:030b}", reg->addr)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                separator(),
                text(std::to_string(0)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Yellow),
                separator(),
                text(fmt::format("{:01b}", reg->space_type)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
            }) | border,
            filler()
        });
    } else if (type == UIBarElemType::Memory) {
        auto reg = reinterpret_cast<const RegBARMem *>(&bar);
        reg_box = hbox({
            hbox({
                text(fmt::format("{:028b}", reg->addr)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                separator(),
                text(fmt::format("{:01b}", reg->prefetch)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Yellow),
                separator(),
                text(fmt::format("{:02b}", reg->type)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Blue),
                separator(),
                text(fmt::format("{:01b}", reg->space_type)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
            }) | border,
            filler()
        });
    } else { // expansion rom
        auto reg = reinterpret_cast<const RegExpROMBar *>(&bar);
        reg_box = hbox({
            hbox({
                text(fmt::format("{:021b}", reg->bar)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                separator(),
                text(fmt::format("{:010b}", reg->rsvd)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Yellow),
                separator(),
                text(fmt::format("{:01b}", reg->ena)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
            }) | border,
            filler()
        });
    }

    return reg_box;
}

static Component
RegInfoBARComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
               const uint8_t *on_click)
{
    uint32_t bar, bar_idx;

    if (dev->type_ == pci::pci_dev_type::TYPE0) {
        auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);

        switch (std::get<Type0Cfg>(reg_type)) {
        case Type0Cfg::bar0:
            bar = type0_dev->get_bar0();
            bar_idx = 0;
            break;
        case Type0Cfg::bar1:
            bar = type0_dev->get_bar1();
            bar_idx = 1;
            break;
        case Type0Cfg::bar2:
            bar = type0_dev->get_bar2();
            bar_idx = 2;
            break;
        case Type0Cfg::bar3:
            bar = type0_dev->get_bar3();
            bar_idx = 3;
            break;
        case Type0Cfg::bar4:
            bar = type0_dev->get_bar4();
            bar_idx = 4;
            break;
        case Type0Cfg::bar5:
            bar = type0_dev->get_bar5();
            bar_idx = 5;
            break;
        default:
            assert(false);
        }
    } else {
        auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);

        switch (std::get<Type1Cfg>(reg_type)) {
        case Type1Cfg::bar0:
            bar = type1_dev->get_bar0();
            bar_idx = 0;
            break;
        case Type1Cfg::bar1:
            bar = type1_dev->get_bar1();
            bar_idx = 1;
            break;
        default:
            assert(false);
        }
    }

    Element content;

    uint32_t prev_bar_idx = (bar_idx > 0) ? (bar_idx - 1) : 0;

    const auto &cur_bar_res = dev->bar_res_[bar_idx];
    const auto &prev_bar_res = dev->bar_res_[prev_bar_idx];

    if (cur_bar_res.type_ == pci::ResourceType::IO) {
        auto reg_box = GetBarElem(UIBarElemType::IoSpace, bar);
        auto desc = vbox({
            text(fmt::format("phys address: {:#x}", cur_bar_res.phys_addr_)),
            text(fmt::format("size: {:#x}", cur_bar_res.len_))
        });

        content = vbox({
            text("I/O space:") | bold,
            separatorLight(),
            reg_box,
            desc
        });
    } else if (cur_bar_res.type_ == pci::ResourceType::EMPTY) {
        // It can be that current BAR is indeed not initialized/used by device
        // or it should be interpreted as upper 32 bits of the address in the
        // previous BAR.
        auto reg_box = GetBarElem(UIBarElemType::Empty, bar);

        if (prev_bar_idx != bar_idx &&
            prev_bar_res.type_ == pci::ResourceType::MEMORY &&
            prev_bar_res.is_64bit_ == true) {
            content = vbox({
                text(fmt::format("Upper 32 bits of address in BAR{}:",
                                        prev_bar_idx)) | bold,
                separatorLight(),
                reg_box,
                text(fmt::format("{:#x}", bar))
            });
        } else {
            content = vbox({
                text("Uninitialized BAR: ") | dim
                            | bgcolor(Color::Red)
                            | color(Color::Grey15),
                separatorLight(),
                reg_box
            });
        }
    } else { // Memory BAR
        auto reg_box = GetBarElem(UIBarElemType::Memory, bar);
        auto desc = vbox({
            text(fmt::format("phys address: {:#x}", cur_bar_res.phys_addr_)),
            text(fmt::format("        size: {:#x}", cur_bar_res.len_)),
            text(fmt::format("      64-bit: {}", cur_bar_res.is_64bit_ ? "▣ " : "☐ ")),
            text(fmt::format("prefetchable: {}", cur_bar_res.is_prefetchable_ ? "▣ " : "☐ "))
        });

        Elements content_elems {
            text("Memory space:") | bold, 
            separatorLight(),
            reg_box,
            desc
        };

        if (cur_bar_res.has_v2p_info_) {
            auto pa_start = cur_bar_res.phys_addr_;
            auto pa_end = cur_bar_res.phys_addr_ + cur_bar_res.len_;

            content_elems.push_back(separatorEmpty());
            content_elems.push_back(
                    text(fmt::format("v2p mappings for PA range [{:#x} - {:#x}]:",
                                     pa_start, pa_end)));
            for (const auto &vm_e : dev->v2p_bar_map_info_[bar_idx]) {
                content_elems.push_back(
                    text(fmt::format("VA range [{:#x} - {:#x}] -> PA {:#x} len {:#x}",
                                     vm_e.start_, vm_e.end_, vm_e.pa_, vm_e.len_))
                );
            }
        }

        content = vbox(content_elems);
    }

    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static Component
RegInfoCardbusCISComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto content = text(fmt::format("{:02x}", type0_dev->get_cardbus_cis()));
    return CreateRegInfoCompat(Type0Cfg::cardbus_cis_ptr, std::move(content), on_click);
}

static Component
RegInfoSubsysVIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto content = text(fmt::format("[{:04x}] -> {}", type0_dev->get_subsys_vid(),
                                        dev->ids_names_[pci::SUBSYS_NAME].empty() ?
                                        dev->ids_names_[pci::SUBSYS_VENDOR] :
                                        dev->ids_names_[pci::SUBSYS_NAME]));
    return CreateRegInfoCompat(Type0Cfg::subsys_vid, std::move(content), on_click);
}

static Component
RegInfoSubsysIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto content = text(fmt::format("[{:04x}] -> {}", type0_dev->get_subsys_dev_id(),
                                        dev->ids_names_[pci::SUBSYS_NAME].empty() ?
                                        dev->ids_names_[pci::SUBSYS_VENDOR] :
                                        dev->ids_names_[pci::SUBSYS_NAME]));
    return CreateRegInfoCompat(Type0Cfg::subsys_dev_id, std::move(content), on_click);
}

static Component
RegInfoExpROMComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                  const uint8_t *on_click)
{
    Element content;
    auto exp_rom_bar = dev->get_exp_rom_bar();

    if (exp_rom_bar != 0) {
        auto reg_box = GetBarElem(UIBarElemType::Exp, exp_rom_bar);
        auto [start, end, flags] = dev->resources_[6];
        content = vbox({
            reg_box,
            text(fmt::format("phys address: {:#x}", start)),
            text(fmt::format("        size: {:#x}", end - start + 1)),
            text(fmt::format("     enabled: {}", (exp_rom_bar & 0x1) ? "▣ " : "☐ "))
        });
    } else {
        auto reg_box = GetBarElem(UIBarElemType::Empty, exp_rom_bar);
        content = vbox({
            text("Uninitialized Expansion ROM: ") | dim
                        | bgcolor(Color::Red)
                        | color(Color::Grey15),
            separatorLight(),
            reg_box
        });
    }

    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static Component
RegInfoCapPtrComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                  const uint8_t *on_click)
{
    auto cap_ptr = dev->get_cap_ptr();
    cap_ptr &= 0xfc;
    auto content = hbox({
            text("Address of the first capability within PCI-compat cfg space: "),
            text(fmt::format("[{:#x}]", cap_ptr)) | bold
                        | bgcolor(Color::Green)
                        | color(Color::Grey15)
    });
    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static Component
RegInfoItrLineComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                   const uint8_t *on_click)
{
    auto itr_line = dev->get_itr_line();
    auto content = text(fmt::format("IRQ [{:#x}]", itr_line));
    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static Component
RegInfoItrPinComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                   const uint8_t *on_click)
{
    auto itr_pin = dev->get_itr_pin();
    std::string desc;
    switch(itr_pin) {
    case 0x1:
        desc = "INTA";
        break;
    case 0x2:
        desc = "INTB";
        break;
    case 0x3:
        desc = "INTC";
        break;
    case 0x4:
        desc = "INTD";
        break;
    case 0x0:
        desc = "no lagacy ITR msg";
        break;
    default:
        desc = "rsvd";
    }

    auto content = text(fmt::format("[{:#x}] -> {}", itr_pin, desc));
    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static Component
RegInfoMinGntComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto min_gnt = type0_dev->get_min_gnt();
    auto content = text(fmt::format("[{:#x}]", min_gnt));
    return CreateRegInfoCompat(Type0Cfg::min_gnt, std::move(content), on_click);
}

static Component
RegInfoMaxLatComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto max_lat = type0_dev->get_max_lat();
    auto content = text(fmt::format("[{:#x}]", max_lat));
    return CreateRegInfoCompat(Type0Cfg::max_lat, std::move(content), on_click);
}

static Component
RegInfoPrimBusNumComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto prim_bus = type1_dev->get_prim_bus_num();
    auto content = text(fmt::format("[{:#x}]", prim_bus));
    return CreateRegInfoCompat(Type1Cfg::prim_bus_num, std::move(content), on_click);
}

static Component
RegInfoSecBusNumComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto sec_bus = type1_dev->get_sec_bus_num();
    auto content = text(fmt::format("[{:#x}]", sec_bus));
    return CreateRegInfoCompat(Type1Cfg::sec_bus_num, std::move(content), on_click);
}

static Component
RegInfoSubBusNumComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto sub_bus = type1_dev->get_sub_bus_num();
    auto content = text(fmt::format("[{:#x}]", sub_bus));
    return CreateRegInfoCompat(Type1Cfg::sub_bus_num, std::move(content), on_click);
}

static Component
RegInfoSecLatTmrComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto content = text(fmt::format("Sec Latency Tmr: {:02x}", type1_dev->get_sec_lat_timer()));
    return CreateRegInfoCompat(Type1Cfg::sec_lat_timer, std::move(content), on_click);
}

static Component
RegInfoIOBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_base = type1_dev->get_io_base();
    Element content;
    if (io_base == 0) {
        content = text(fmt::format("[{:02x}] -> uninitialized", io_base));
    } else {
        auto io_base_reg = reinterpret_cast<const RegIOBase *>(&io_base);
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:04b}", io_base_reg->addr)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                separator(),
                text(fmt::format("{:04b}", io_base_reg->cap)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
            }) | border,
            filler()
        });

        uint32_t addr = io_base_reg->addr << 12;
        if (io_base_reg->cap == 1) {
            auto io_base_upper_reg = type1_dev->get_io_base_upper();
            addr |= io_base_upper_reg << 16;
        }

        auto desc = vbox({
            text(fmt::format("i/o base address:     {:#04x}", addr)),
            text(fmt::format("i/o addressing width: {}", io_base_reg->cap == 0 ?
                                                                "16-bit" : "32-bit")),
        });

        content = vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_base, std::move(content), on_click);
}

static Component
RegInfoIOLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_base = type1_dev->get_io_base();
    auto io_limit = type1_dev->get_io_limit();
    Element content;
    if (io_limit == 0 && !io_base) {
        content = text(fmt::format("[{:02x}] -> uninitialized", io_limit));
    } else {
        auto io_limit_reg = reinterpret_cast<const RegIOLimit *>(&io_limit);
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:04b}", io_limit_reg->addr)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                separator(),
                text(fmt::format("{:04b}", io_limit_reg->cap)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
            }) | border,
            filler()
        });

        uint32_t addr = (io_limit_reg->addr << 12) | 0xfff;
        if (io_limit_reg->cap == 1) {
            auto io_limit_upper_reg = type1_dev->get_io_limit_upper();
            addr |= io_limit_upper_reg << 16;
        }

        auto desc = vbox({
            text(fmt::format("           i/o limit: {:#04x}", addr)),
            text(fmt::format("i/o addressing width: {}", io_limit_reg->cap == 0 ?
                                                         "16-bit" : "32-bit")),
        });

        content = vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_limit, std::move(content), on_click);
}

static Component
RegInfoUpperIOBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_base = type1_dev->get_io_base();
    auto io_base_reg = reinterpret_cast<const RegIOBase *>(&io_base);

    Element content;

    if (io_base_reg->cap == 1) {
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:016b}", type1_dev->get_io_base_upper())) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green)
            }) | border,
            filler()
        });

        content = vbox({
            reg_box,
            text(fmt::format("Upper 16 bits of I/O Base: {:04x}",
                             type1_dev->get_io_base_upper()))
        });
    } else {
        content = vbox({
            text(fmt::format("[{}]: 32-bit addressing is not supported",
                             type1_dev->get_io_base_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_base_upper, std::move(content), on_click);
}

static Component
RegInfoUpperIOLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_limit = type1_dev->get_io_limit();
    auto io_limit_reg = reinterpret_cast<const RegIOLimit *>(&io_limit);

    Element content;

    if (io_limit_reg->cap == 1) {
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:16b}", type1_dev->get_io_limit_upper())) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green)
            }) | border,
            filler()
        });

        content = vbox({
            reg_box,
            text(fmt::format("Upper 16 bits of I/O Limit: {:04x}",
                             type1_dev->get_io_limit_upper()))
        });
    } else {
        content = vbox({
            text(fmt::format("[{}]: 32-bit addressing is not supported",
                             type1_dev->get_io_limit_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_limit_upper, std::move(content), on_click);
}

static Component
RegInfoSecStatusComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto sec_status = type1_dev->get_sec_status();
    auto reg = reinterpret_cast<const RegSecStatus *>(&sec_status);

    auto content = vbox({
        RegFieldCompElem(0, 4),
        RegFieldCompElem(5, 5,   " 66 MHz capable", reg->mhz66_cap == 1),
        RegFieldCompElem(6, 6),
        RegFieldCompElem(7, 7,   " fast b2b transactions capable", reg->fast_b2b_trans_cap == 1),
        RegFieldCompElem(8, 8,   " master data parity error", reg->master_data_par_err == 1),
        RegFieldCompElem(9, 10,  fmt::format(" DEVSEL timing: {}", reg->devsel_timing)),
        RegFieldCompElem(11, 11, " signaled target abort", reg->signaled_tgt_abort == 1),
        RegFieldCompElem(12, 12, " received target abort", reg->recv_tgt_abort == 1),
        RegFieldCompElem(13, 13, " received master abort", reg->recv_master_abort == 1),
        RegFieldCompElem(14, 14, " received system error", reg->recv_sys_err == 1),
        RegFieldCompElem(15, 15, " detected parity error", reg->detect_parity_err == 1),
    });

    return CreateRegInfoCompat(Type1Cfg::sec_status, std::move(content), on_click);
}

static Component
RegInfoMemoryBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto mem_base = type1_dev->get_mem_base();
    Element content;
    if (mem_base == 0) {
        content = text(fmt::format("[{:02x}] -> uninitialized", mem_base));
    } else {
        auto mem_base_reg = reinterpret_cast<const RegMemBL *>(&mem_base);
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:012b}", mem_base_reg->addr)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
                separator(),
                text(fmt::format("{:04b}", mem_base_reg->rsvd)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
            }) | border,
            filler()
        });

        content = vbox({
            reg_box,
            text(fmt::format("mem base address: {:#x}",
                        (uint32_t)mem_base_reg->addr << 20))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::mem_base, std::move(content), on_click);
}

static Component
RegInfoMemoryLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto mem_base = type1_dev->get_mem_base();
    auto mem_limit = type1_dev->get_mem_limit();
    Element content;
    if (mem_limit == 0 && !mem_base) {
        content = text(fmt::format("[{:02x}] -> uninitialized", mem_limit));
    } else {
        auto mem_limit_reg = reinterpret_cast<const RegMemBL *>(&mem_limit);
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:12b}", mem_limit_reg->addr)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta),
                separator(),
                text(fmt::format("{:04b}", mem_limit_reg->rsvd)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Green),
            }) | border,
            filler()
        });

        content = vbox({
            reg_box,
            text(fmt::format("mem limit: {:#x}",
                        (uint32_t)mem_limit_reg->addr << 20 | 0xfffff)),
        });
    }

    return CreateRegInfoCompat(Type1Cfg::mem_limit, std::move(content), on_click);
}

static Component
RegInfoPrefMemBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_base = type1_dev->get_pref_mem_base();
    Element content;
    if (pref_mem_base == 0) {
        content = text(fmt::format("[{:02x}] -> uninitialized", pref_mem_base));
    } else {
        auto pref_mem_base_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_base);
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:012b}", pref_mem_base_reg->addr)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                separator(),
                text(fmt::format("{:04b}", pref_mem_base_reg->cap)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
            }) | border,
            filler()
        });

        uint64_t addr = (uint64_t)pref_mem_base_reg->addr << 20;
        if (pref_mem_base_reg->cap == 1) {
            auto pref_mem_base_upper = type1_dev->get_pref_base_upper();
            addr |= ((uint64_t)pref_mem_base_upper << 32);
        }

        auto desc = vbox({
            text(fmt::format("prefetchable mem base address:     {:#x}", addr)),
            text(fmt::format("prefetchable mem addressing width: {}",
                             pref_mem_base_reg->cap == 0 ? "32-bit" : "64-bit"))
        });

        content = vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_mem_base, std::move(content), on_click);
}

static Component
RegInfoPrefMemLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_base = type1_dev->get_pref_mem_base();
    auto pref_mem_limit = type1_dev->get_pref_mem_limit();
    Element content;
    if (pref_mem_limit == 0 && !pref_mem_base) {
        content = text(fmt::format("[{:02x}] -> uninitialized", pref_mem_limit));
    } else {
        auto pref_mem_limit_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_limit);
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:012b}", pref_mem_limit_reg->addr)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Green),
                separator(),
                text(fmt::format("{:04b}", pref_mem_limit_reg->cap)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta),
            }) | border,
            filler()
        });

        uint64_t limit = (uint64_t)pref_mem_limit_reg->addr << 20 | 0xfffff;
        if (pref_mem_limit_reg->cap == 1) {
            auto pref_mem_limit_upper = type1_dev->get_pref_limit_upper();
            limit |= (uint64_t)pref_mem_limit_upper << 32;
        }

        auto desc = vbox({
            text(fmt::format("prefetchable mem limit:            {:#x}", limit)),
            text(fmt::format("prefetchable mem addressing width: {}",
                             pref_mem_limit_reg->cap == 0 ? "32-bit" : "64-bit"))
        });

        content = vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_mem_limit, std::move(content), on_click);
}

static Component
RegInfoPrefBaseUpperComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_base = type1_dev->get_pref_mem_base();
    auto pref_mem_base_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_base);

    Element content;

    if (pref_mem_base_reg->cap == 1) {
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:032b}", type1_dev->get_pref_base_upper())) |
                     color(Color::Grey15) |
                     bgcolor(Color::Green)
            }) | border,
            filler()
        });

        content = vbox({
            reg_box,
            text(fmt::format("Upper 32 bits of prefetchable mem base: {:#x}",
                             type1_dev->get_pref_base_upper()))
        });
    } else {
        content = vbox({
            text(fmt::format("[{}]: 64-bit addressing is not supported",
                             type1_dev->get_pref_base_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_base_upper, std::move(content), on_click);
}

static Component
RegInfoPrefLimitUpperComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_limit = type1_dev->get_pref_mem_limit();
    auto pref_mem_limit_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_limit);

    Element content;

    if (pref_mem_limit_reg->cap == 1) {
        auto reg_box = hbox({
            hbox({
                text(fmt::format("{:032b}", type1_dev->get_pref_limit_upper())) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green)
            }) | border,
            filler()
        });

        content = vbox({
            reg_box,
            text(fmt::format("Upper 32 bits of prefetchable mem limit: {:#x}",
                                    type1_dev->get_pref_limit_upper()))
        });
    } else {
        content = vbox({
            text(fmt::format("[{}]: 64-bit addressing is not supported",
                                    type1_dev->get_pref_limit_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_limit_upper, std::move(content), on_click);
}

static Component
RegInfoBridgeCtrlComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto bridge_ctrl = type1_dev->get_bridge_ctl();
    auto reg = reinterpret_cast<const RegBridgeCtl *>(&bridge_ctrl);

    auto content = vbox({
        RegFieldCompElem(0, 0,   " parity error response enable", reg->parity_err_resp_ena == 1),
        RegFieldCompElem(1, 1,   " #SERR enable", reg->serr_ena == 1),
        RegFieldCompElem(2, 2,   " ISA enable", reg->isa_ena == 1),
        RegFieldCompElem(3, 3,   " VGA enable", reg->vga_ena == 1),
        RegFieldCompElem(4, 4,   " VGA 16-bit decode", reg->vga_16bit_decode == 1),
        RegFieldCompElem(5, 5,   " master abort mode", reg->master_abort_mode == 1),
        RegFieldCompElem(6, 6,   " secondary bus reset", reg->sec_bus_reset),
        RegFieldCompElem(7, 7,   " fast b2b transactions enable", reg->fast_b2b_trans_ena == 1),
        RegFieldCompElem(8, 8,   " primary discard timer", reg->prim_discard_tmr == 1),
        RegFieldCompElem(9, 9,   " secondary discard timer", reg->sec_discard_tmr == 1),
        RegFieldCompElem(10, 10, " discard timer status", reg->discard_tmr_status == 1),
        RegFieldCompElem(11, 11, " discard timer #serr enable", reg->discard_tmr_serr_ena == 1),
    });

    return CreateRegInfoCompat(Type1Cfg::bridge_ctl, std::move(content), on_click);
}

// Create a component showing detailed info about the register in PCI-compatible
// configuration region (first 64 bytes)
static Component
CompatRegInfoComponent(const pci::PciDevBase *dev, const Type0Cfg reg_type, const uint8_t *on_click)
{
    switch (reg_type) {
    case Type0Cfg::vid:
        return RegInfoVIDComp(dev, on_click);
    case Type0Cfg::dev_id:
        return RegInfoDevIDComp(dev, on_click);
    case Type0Cfg::command:
        return RegInfoCommandComp(dev, on_click);
    case Type0Cfg::status:
        return RegInfoStatusComp(dev, on_click);
    case Type0Cfg::revision:
        return RegInfoRevComp(dev, on_click);
    case Type0Cfg::class_code:
        return RegInfoCCComp(dev, on_click);
    case Type0Cfg::cache_line_size:
        return RegInfoClSizeComp(dev, on_click);
    case Type0Cfg::latency_timer:
        return RegInfoLatTmrComp(dev, on_click);
    case Type0Cfg::header_type:
        return RegInfoHdrTypeComp(dev, on_click);
    case Type0Cfg::bist:
        return RegInfoBISTComp(dev, on_click);
    case Type0Cfg::bar0:
    case Type0Cfg::bar1:
    case Type0Cfg::bar2:
    case Type0Cfg::bar3:
    case Type0Cfg::bar4:
    case Type0Cfg::bar5:
        return RegInfoBARComp(dev, reg_type, on_click);
    case Type0Cfg::cardbus_cis_ptr:
        return RegInfoCardbusCISComp(dev, on_click);
    case Type0Cfg::subsys_vid:
        return RegInfoSubsysVIDComp(dev, on_click);
    case Type0Cfg::subsys_dev_id:
        return RegInfoSubsysIDComp(dev, on_click);
    case Type0Cfg::exp_rom_bar:
        return RegInfoExpROMComp(dev, reg_type, on_click);
    case Type0Cfg::cap_ptr:
        return RegInfoCapPtrComp(dev, reg_type, on_click);
    case Type0Cfg::itr_line:
        return RegInfoItrLineComp(dev, reg_type, on_click);
    case Type0Cfg::itr_pin:
        return RegInfoItrPinComp(dev, reg_type, on_click);
    case Type0Cfg::min_gnt:
        return RegInfoMinGntComp(dev, on_click);
    case Type0Cfg::max_lat:
        return RegInfoMaxLatComp(dev, on_click);
    default:
        assert(false);
    }

}

static Component
CompatRegInfoComponent(const pci::PciDevBase *dev, const Type1Cfg reg_type, const uint8_t *on_click)
{
    switch (reg_type) {
    case Type1Cfg::vid:
        return RegInfoVIDComp(dev, on_click);
    case Type1Cfg::dev_id:
        return RegInfoDevIDComp(dev, on_click);
    case Type1Cfg::command:
        return RegInfoCommandComp(dev, on_click);
    case Type1Cfg::status:
        return RegInfoStatusComp(dev, on_click);
    case Type1Cfg::revision:
        return RegInfoRevComp(dev, on_click);
    case Type1Cfg::class_code:
        return RegInfoCCComp(dev, on_click);
    case Type1Cfg::cache_line_size:
        return RegInfoClSizeComp(dev, on_click);
    case Type1Cfg::prim_lat_timer:
        return RegInfoLatTmrComp(dev, on_click);
    case Type1Cfg::header_type:
        return RegInfoHdrTypeComp(dev, on_click);
    case Type1Cfg::bist:
        return RegInfoBISTComp(dev, on_click);
    case Type1Cfg::bar0:
    case Type1Cfg::bar1:
        return RegInfoBARComp(dev, reg_type, on_click);
    case Type1Cfg::prim_bus_num:
        return RegInfoPrimBusNumComp(dev, on_click);
    case Type1Cfg::sec_bus_num:
        return RegInfoSecBusNumComp(dev, on_click);
    case Type1Cfg::sub_bus_num:
        return RegInfoSubBusNumComp(dev, on_click);
    case Type1Cfg::sec_lat_timer:
        return RegInfoSecLatTmrComp(dev, on_click);
    case Type1Cfg::io_base:
        return RegInfoIOBaseComp(dev, on_click);
    case Type1Cfg::io_limit:
        return RegInfoIOLimitComp(dev, on_click);
    case Type1Cfg::sec_status:
        return RegInfoSecStatusComp(dev, on_click);
    case Type1Cfg::mem_base:
        return RegInfoMemoryBaseComp(dev, on_click);
    case Type1Cfg::mem_limit:
        return RegInfoMemoryLimitComp(dev, on_click);
    case Type1Cfg::pref_mem_base:
        return RegInfoPrefMemBaseComp(dev, on_click);
    case Type1Cfg::pref_mem_limit:
        return RegInfoPrefMemLimitComp(dev, on_click);
    case Type1Cfg::pref_base_upper:
        return RegInfoPrefBaseUpperComp(dev, on_click);
    case Type1Cfg::pref_limit_upper:
        return RegInfoPrefLimitUpperComp(dev, on_click);
    case Type1Cfg::io_base_upper:
        return RegInfoUpperIOBaseComp(dev, on_click);
    case Type1Cfg::io_limit_upper:
        return RegInfoUpperIOLimitComp(dev, on_click);
    case Type1Cfg::cap_ptr:
        return RegInfoCapPtrComp(dev, reg_type, on_click);
    case Type1Cfg::exp_rom_bar:
        return RegInfoExpROMComp(dev, reg_type, on_click);
    case Type1Cfg::itr_line:
        return RegInfoItrLineComp(dev, reg_type, on_click);
    case Type1Cfg::itr_pin:
        return RegInfoItrPinComp(dev, reg_type, on_click);
    case Type1Cfg::bridge_ctl:
        return RegInfoBridgeCtrlComp(dev, on_click);
    default:
        assert(false);
    }
}

capability_comp_ctx
GetCompatHeaderRegsComponents(const pci::PciDevBase *dev, std::vector<uint8_t> &vis_state_vt)
{
    Components upper, lower;
    size_t i = vis_state_vt.size();

    // Determine amount of clickable elements -> number of compat registers
    auto e_cnt = (dev->type_ == pci::pci_dev_type::TYPE0) ?
                    type0_compat_reg_cnt : type1_compat_reg_cnt;
    std::ranges::fill_n(std::back_inserter(vis_state_vt), e_cnt, 0);

    // Add PCI-compatible config space header delimiter
    upper.push_back(Renderer([] {
        return hbox({
            separatorLight() | flex,
            text("[compatible cfg space header]") | bold | inverted | center,
            separatorLight() | flex
        });
    }));

    // Some registers in PCI-compatible config space are identical for both type 0 and type 1 devices
    upper.push_back(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::dev_id), &vis_state_vt[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::vid),    &vis_state_vt[i++])
                      }));

    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::dev_id, &vis_state_vt[i - 2]));
    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::vid,    &vis_state_vt[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::status),  &vis_state_vt[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::command), &vis_state_vt[i++])
                     }));

    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::status,  &vis_state_vt[i - 2]));
    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::command, &vis_state_vt[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::class_code),   &vis_state_vt[i++]),
                        Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::revision), &vis_state_vt[i++])
                        })
                     }));

    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::class_code, &vis_state_vt[i - 2]));
    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::revision,   &vis_state_vt[i - 1]));

    upper.push_back(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::bist),            &vis_state_vt[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::header_type),     &vis_state_vt[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::latency_timer),   &vis_state_vt[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::cache_line_size), &vis_state_vt[i++])
                     }));

    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bist,
                                                                  &vis_state_vt[i - 4]));
    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::header_type,
                                                                  &vis_state_vt[i - 3]));
    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::latency_timer,
                                                                  &vis_state_vt[i - 2]));
    lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::cache_line_size,
                                                                  &vis_state_vt[i - 1]));

    if (dev->type_ == pci::pci_dev_type::TYPE0) {
        // BARs
        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar0), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bar0, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar1), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bar1, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar2), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bar2, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar3), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bar3, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar4), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bar4, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar5), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::bar5, &vis_state_vt[i++]));

        // Cardbus CIS ptr
        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::cardbus_cis_ptr), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::cardbus_cis_ptr,
                                                      &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::subsys_dev_id), &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::subsys_vid),    &vis_state_vt[i++])
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::subsys_dev_id,
                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::subsys_vid,
                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::exp_rom_bar), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::exp_rom_bar,
                                                      &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("Rsvd (0x35)"),
                            Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type0Cfg::cap_ptr), &vis_state_vt[i++])
                            })
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::cap_ptr,
                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("Rsvd (0x38)")
                         }));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::max_lat),  &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::min_gnt),  &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::itr_pin),  &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::itr_line), &vis_state_vt[i++])
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::max_lat,
                                                                      &vis_state_vt[i - 4]));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::min_gnt,
                                                                      &vis_state_vt[i - 3]));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::itr_pin,
                                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type0Cfg::itr_line,
                                                                      &vis_state_vt[i - 1]));
    } else { // type 1
        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bar0), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::bar0, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bar1), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::bar1, &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_lat_timer), &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sub_bus_num),   &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_bus_num),   &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::prim_bus_num),  &vis_state_vt[i++])
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::sec_lat_timer,
                                                                      &vis_state_vt[i - 4]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::sub_bus_num,
                                                                      &vis_state_vt[i - 3]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::sec_bus_num,
                                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::prim_bus_num,
                                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_status), &vis_state_vt[i++]),
                            Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_limit), &vis_state_vt[i++]),
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_base), &vis_state_vt[i++])
                            })
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::sec_status,
                                                                      &vis_state_vt[i - 3]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::io_limit,
                                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::io_base,
                                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::mem_limit), &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::mem_base), &vis_state_vt[i++])
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::mem_limit,
                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::mem_base,
                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_mem_limit), &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_mem_base),  &vis_state_vt[i++])
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::pref_mem_limit,
                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::pref_mem_base,
                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_base_upper), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::pref_base_upper,
                                                      &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_limit_upper), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::pref_limit_upper,
                                                      &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_limit_upper), &vis_state_vt[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_base_upper),  &vis_state_vt[i++])
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::io_limit_upper,
                                                      &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::io_base_upper,
                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp("Rsvd (0x35)"),
                            Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::cap_ptr), &vis_state_vt[i++])
                            })
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::cap_ptr,
                                                      &vis_state_vt[i - 1]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::exp_rom_bar), &vis_state_vt[i])
                         }));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::exp_rom_bar,
                                                      &vis_state_vt[i++]));

        upper.push_back(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bridge_ctl), &vis_state_vt[i++]),
                            Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::itr_pin), &vis_state_vt[i++]),
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::itr_line), &vis_state_vt[i++])
                            })
                         }));

        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::bridge_ctl,
                                               &vis_state_vt[i - 3]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::itr_pin,
                                               &vis_state_vt[i - 2]));
        lower.push_back(CompatRegInfoComponent(dev, Type1Cfg::itr_line,
                                               &vis_state_vt[i - 1]));
    }

    return {std::move(upper), std::move(lower)};
}

Component
CapDelimComp(const pci::CapDesc &cap)
{
    auto type = std::get<0>(cap);
    auto off = std::get<3>(cap);

    std::string label;
    if (type == pci::CapType::compat) {
        auto compat_cap_type = CompatCapID {std::get<1>(cap)};
        label = fmt::format(">>> {} [compat] ", CompatCapName(compat_cap_type));
    } else {
        auto ext_cap_type = ExtCapID {std::get<1>(cap)};
        label = fmt::format(">>> {} [extended] ", ExtCapName(ext_cap_type));
    }

    auto comp = Renderer([=] {
        return vbox({
            hbox({
                text(label) | inverted | bold,
                text(fmt::format("[{:#02x}]", off)) | bold |
                bgcolor(Color::Magenta) | color(Color::Grey15)
            }),
            separatorLight()
        });
    });
    return comp;
}

Component
CapsDelimComp(const pci::CapType type, const uint8_t caps_num)
{
    auto comp = Renderer([=]{
        return vbox({
                    separatorEmpty(),
                    hbox({
                        separatorHeavy() | flex,
                        text(fmt::format("[{} {} cap(s)]", caps_num,
                                         type == pci::CapType::compat ?
                                         "compatible" : "extended")) |
                        bold | inverted | center,
                        separatorHeavy() | flex
                    }),
                    separatorEmpty()
                });
    });
    return comp;
}

Component
CapHdrComp(const cap_hdr_type_t cap_hdr)
{
    Element elem;
    std::visit([&] (auto &&hdr) {
        using T = std::decay_t<decltype(hdr)>;
        if constexpr (std::is_same_v<T, CompatCapHdr>) {
            elem = hbox({
                text(fmt::format("next: {:#3x}", hdr.next_cap)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Green),
                separatorEmpty(),
                text(fmt::format("id: {:#3x}", hdr.cap_id)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Blue)
            }) | border;
        } else {
            elem = hbox({
                text(fmt::format("next: {:#5x}", hdr.next_cap)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Green),
                separatorEmpty(),
                text(fmt::format("ver: {:#5x}", hdr.cap_ver)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Yellow),
                separatorEmpty(),
                text(fmt::format("id: {:#5x}", hdr.cap_id)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Blue),
            }) | border;
        }
    }, cap_hdr);

    return Renderer([=] {
        return std::move(elem);
    });
}

Component
EmptyCapRegComp(const std::string desc)
{
    return Renderer([=] {
        return text(desc) | strikethrough | center | border | flex;
    });
}

Component
CreateCapRegInfo(const std::string &cap_desc, const std::string &cap_reg, Element content,
                 const uint8_t *on_click)
{
    auto title = text(fmt::format("{} -> {}", cap_desc, cap_reg)) |
                             inverted | align_right | bold;
    auto info_window = window(std::move(title), std::move(content));
    return GetCompMaybe(std::move(info_window), on_click);
}












} // namespace ui
