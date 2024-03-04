// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include "screen.h"

namespace ui {

void CanvasVisibleArea::shift(CnvShiftDir dir, ftxui::Box &box)
{
    switch (dir) {
        case CnvShiftDir::UP:
            if (off_y_ > 0)
                off_y_ -= 1;
            break;
        case CnvShiftDir::LEFT:
            if (off_x_ > 0)
                off_x_ -= 1;
            break;
        case CnvShiftDir::DOWN:
            if (box.y_max >= y_max_) {
                off_y_ = 0;
                return;
            }

            if (off_y_ + box.y_max > y_max_) {
                off_y_ = y_max_ - box.y_max;
                return;
            }

            if (off_y_ + box.y_max < y_max_) {
                off_y_ += 1;
            }

            break;
        case CnvShiftDir::RIGHT:
            if (box.x_max >= x_max_) {
                off_x_ = 0;
                return;
            }

            if (off_x_ + box.x_max > x_max_) {
                off_x_ = x_max_ - box.x_max;
                return;
            }

            if (off_x_ + box.x_max < x_max_) {
                off_x_ += 1;
            }
            break;
        default:
            return;
    }
}


// draw a simple box
void ScrollableCanvas::DrawBoxLine(ShapeDesc desc, const ftxui::Color &color)
{
    auto [x, y, len, height] = desc;
    DrawPointLine(      x,          y,       x, y + height, color);
    DrawPointLine(      x, y + height, x + len, y + height, color);
    DrawPointLine(x + len, y + height, x + len,          y, color);
    DrawPointLine(x + len,          y,       x,          y, color);
}

void ScrollableCanvas::DrawBoxLine(ShapeDesc desc,
                                   const ftxui::Canvas::Stylizer &style)
{
    auto [x, y, len, height] = desc;
    DrawPointLine(      x,          y,       x, y + height, style);
    DrawPointLine(      x, y + height, x + len, y + height, style);
    DrawPointLine(x + len, y + height, x + len,          y, style);
    DrawPointLine(x + len,          y,       x,          y, style);
}

// draw a box with the text inside it
// FIXME: text is expected to be single line for now
// @pos {x, y}: x is expected to be mulitple of 2,
//              y is expected to be multiple of 4
void ScrollableCanvas::DrawTextBox(std::pair<uint16_t, uint16_t> pos, std::string text,
                                   const ftxui::Canvas::Stylizer &box_style,
                                   const ftxui::Canvas::Stylizer &text_style)
{
    auto [x, y] = pos;
    // symbol width is two pixels + 1 pixel on both sides
    auto hlen = text.length() * 2 + 1 * 2;
    // symbol height is 4 pixels
    auto vlen = 4 + 2;

    DrawBoxLine({x + 2, y + 3, hlen, vlen}, box_style);
    DrawText(x + tbox_txt_xoff, y + tbox_txt_yoff , text, text_style);
}

void CanvasElemConnector::AddLine(std::pair<PointDesc, PointDesc> line_desc)
{
    lines_.push_back(line_desc);
}

void CanvasElemConnector::Draw(ScrollableCanvas &canvas)
{
    for (const auto &line_desc : lines_) {
        auto [p1, p2] = line_desc;
        auto [x1, y1] = p1;
        auto [x2, y2] = p2;
        canvas.DrawPointLine(x1, y1, x2, y2);
    }
}

CanvasElemPCIDev::CanvasElemPCIDev(std::shared_ptr<pci::PciDevBase> dev, ElemReprMode repr_mode,
                                   uint16_t x, uint16_t y)
    : dev_(dev), selected_(false)
{
    size_t max_hlen, max_vlen;

    // initialize text array
    auto bdf_id_str = fmt::format("{} | [{:04x}:{:04x}]", dev->dev_id_str_,
                                  dev->get_vendor_id(), dev->get_device_id());
    max_hlen = bdf_id_str.length();
    text_data_.push_back(std::move(bdf_id_str));

    if (repr_mode == ElemReprMode::Verbose) {
        if (!dev->ids_names_[pci::VENDOR].empty()) {
            auto vendor_name = fmt::format("{}", dev->ids_names_[pci::VENDOR]);
            if (vendor_name.length() > max_hlen)
                max_hlen = vendor_name.length();
            text_data_.push_back(std::move(vendor_name));
        }

        if (!dev->ids_names_[pci::DEVICE].empty()) {
            auto device_name = fmt::format("{}", dev->ids_names_[pci::DEVICE]);
            if (device_name.length() > max_hlen)
                max_hlen = device_name.length();
            text_data_.push_back(std::move(device_name));
        }
    }

    // symbol width is 2 pixels + 2 pixel on both sides
    max_hlen = max_hlen * sym_width + 2 * 2;
    // symbol height is 4 pixels
    max_vlen = text_data_.size() * sym_height + 2;

    auto x_ = x + 1;
    auto y_ = y + 3;

    // save box coordinates
    points_ = { x_, y_, max_hlen, max_vlen };
}



uint16_t CanvasElemPCIDev::GetHeight() noexcept
{
    return (text_data_.size() + 2) * 4;
}

PointDesc CanvasElemPCIDev::GetConnPosParent() noexcept
{
    auto x_pos = std::get<0>(points_) + 4;
    auto y_pos = std::get<1>(points_) + std::get<3>(points_);
    return { x_pos, y_pos };
}

PointDesc CanvasElemPCIDev::GetConnPosChild() noexcept
{
    auto x_pos = std::get<0>(points_);
    auto y_pos = std::get<1>(points_) + std::get<3>(points_) / 2;
    return { x_pos, y_pos };
}

ShapeDesc CanvasElemPCIDev::GetShapeDesc() noexcept
{
    return points_;
}

void CanvasElemPCIDev::Draw(ScrollableCanvas &canvas)
{
    // draw box
    if (selected_)
        canvas.DrawBoxLine(points_, [](ftxui::Pixel &p) {
            p.foreground_color = ftxui::Color::Palette256::Orange1;
            p.bold = true;
        });
    else
        // to remove element highlight, 'inverse' highlight style has to
        // be explicitly specified
        canvas.DrawBoxLine(points_, [](ftxui::Pixel &p) {
            p.foreground_color = ftxui::Color::Default;
            p.bold = false;
        });

    // draw text
    auto x1 = std::get<0>(points_);
    auto y1 = std::get<1>(points_);
    x1 += 4;
    y1 += 4;

    for (size_t i = 0; const auto &text_str : text_data_) {
        if (i++ == 0)
            canvas.DrawText(x1, y1, text_str, [](ftxui::Pixel &p) {
                p.bold = true;
            });
        else
            canvas.DrawText(x1, y1, text_str);

        y1 += 4;
    }
}

PointDesc CanvasElemBus::GetConnPos() noexcept
{
    auto x_pos = std::get<0>(points_) + 4;
    auto y_pos = std::get<1>(points_) + std::get<3>(points_);
    return { x_pos, y_pos };
}

void CanvasElemBus::Draw(ScrollableCanvas &canvas)
{
    canvas.DrawBoxLine(points_, [](ftxui::Pixel &p) {
        p.bold = true;
        p.foreground_color = ftxui::Color::Magenta;
    });

    auto x1 = std::get<0>(points_);
    auto y1 = std::get<1>(points_);
    x1 += 4;
    y1 += 4;
    canvas.DrawText(x1, y1, bus_id_str_, [](ftxui::Pixel &p) {
        p.bold = true;
        p.foreground_color = ftxui::Color::Blue;
    });
}

// mouse click tracking stuff

bool CanvasDevBlockMap::Insert(std::shared_ptr<CanvasElemPCIDev> dev)
{
    auto [x, y, len, height] = dev->GetShapeDesc();
    auto res = blocks_y_dim_.insert({{y, height}, dev});
    return res.second;
}

void CanvasDevBlockMap::SelectDevice(const uint16_t mouse_x, const uint16_t mouse_y,
                                     ScrollableCanvas &canvas)
{
    auto y_iter = blocks_y_dim_.lower_bound({mouse_y * 4, 0});
    y_iter--;

    if (y_iter != blocks_y_dim_.end())
    {
        auto [x, y, len, height] = y_iter->second->GetShapeDesc();
        auto x_in_pixels = mouse_x * sym_width;

        if (x_in_pixels >= x && x_in_pixels <= x + len) {
            if (selected_dev_ != nullptr) {
                selected_dev_->selected_ = false;
                // redraw previously selected element since it's not selected now
                selected_dev_->Draw(canvas);
            }

            selected_dev_ = y_iter->second;
            selected_dev_->selected_ = true;

            // redraw newly selected element
            selected_dev_->Draw(canvas);
        }
    }
}

void CanvasDevBlockMap::Reset()
{
    blocks_y_dim_.clear();
}

// UI topology element impl

ftxui::Element PCITopoUIComp::Render()
{
    const bool is_focused = Focused();
    const bool is_active = Active();
    auto focus_mgmt = is_focused ? ftxui::focus :
                      is_active ? ftxui::select :
                      ftxui::nothing;

    return scrollable_canvas(canvas_) |
           focus_mgmt | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, max_width_) |
           ftxui::reflect(box_);
}

bool PCITopoUIComp::OnEvent(ftxui::Event event)
{
    hovered_ = box_.Contain(event.mouse().x, event.mouse().y);

    // XXX: is it necessary ?
    //if (!CaptureMouse(event))
    //    return false;

    if (hovered_)
        TakeFocus();
    else
        return false;

    auto area = canvas_.GetVisibleAreaDesc();

    if (event.is_mouse()) {
        //logger.info("PCITopoUIComp -> mouse event: shift {} meta {} ctrl {}",
        //            event.mouse().shift, event.mouse().meta, event.mouse().control);

        if (event.mouse().button == ftxui::Mouse::WheelDown) {
            if (event.mouse().shift)
                area->shift(CnvShiftDir::RIGHT, box_);
            else
                area->shift(CnvShiftDir::DOWN, box_);
        }

        if (event.mouse().button == ftxui::Mouse::WheelUp) {
            if (event.mouse().shift)
                area->shift(CnvShiftDir::LEFT, box_);
            else
                area->shift(CnvShiftDir::UP, box_);
        }

        if (event.mouse().button == ftxui::Mouse::Left)
            block_map_.SelectDevice(event.mouse().x + area->off_x_,
                                   event.mouse().y + area->off_y_,
                                   canvas_);

        return true;
    }

    if (event.is_character()) {
        //logger.info("PCITopoUIComp -> char event");

        switch (event.character()[0]) {
        // scrolling
        case 'j':
            area->shift(CnvShiftDir::DOWN, box_);
            break;
        case 'k':
            area->shift(CnvShiftDir::UP, box_);
            break;
        case 'h':
            area->shift(CnvShiftDir::LEFT, box_);
            break;
        case 'l':
            area->shift(CnvShiftDir::RIGHT, box_);
            break;
        // change canvas elems representation mode
        case 'c':
            SwitchDrawingMode(ElemReprMode::Compact);
            break;
        case 'v':
            SwitchDrawingMode(ElemReprMode::Verbose);
            break;
        default:
            break;
        }

        return true;
    }
    return false;
}

void PCITopoUIComp::AddTopologyElements()
{
    uint16_t x = 2, y = 4;
    for (const auto &bus : topo_ctx_.buses_) {
        if (bus.second.is_root_) {
            auto conn_pos = AddRootBus(bus.second, &x, &y);
            AddBusDevices(bus.second, topo_ctx_.buses_, conn_pos, x + child_elem_xoff, &y);
        }
    }

    max_width_ = (max_width_ + 4) / sym_width;
    canvas_.VisibleAreaSetMax(max_width_, canvas_.height() / 4);

    // select element
    if (block_map_.selected_dev_ == nullptr) {
        block_map_.selected_dev_ = block_map_.blocks_y_dim_.begin()->second;
        block_map_.selected_dev_->selected_ = true;
    } else {
        auto cur_dev = block_map_.selected_dev_->dev_;
        for (const auto &dev : block_map_.blocks_y_dim_) {
            if (dev.second->dev_ == cur_dev) {
                block_map_.selected_dev_ = dev.second;
                block_map_.selected_dev_->selected_ = true;
                break;
            }
        }
    }
    DrawElements();
}

void PCITopoUIComp::DrawElements() noexcept
{
    for (const auto &ce : canvas_elems_)
        ce->Draw(canvas_);
}

std::pair<PointDesc, PointDesc>
PCITopoUIComp::AddDevice(std::shared_ptr<pci::PciDevBase> dev, uint16_t x, uint16_t *y)
{
    auto device = std::make_shared<CanvasElemPCIDev>(dev, current_drawing_mode_, x, *y);
    std::pair<PointDesc, PointDesc> conn_pos_pair {device->GetConnPosChild(),
                                                   device->GetConnPosParent()};
    *y += device->GetHeight();
    if (!block_map_.Insert(device))
        logger.warn("Failed to add {} device to block tracking map", dev->dev_id_str_);

    // figure out max width of useful data on canvas
    auto dev_xpos = std::get<0>(device->points_);
    auto dev_width = std::get<2>(device->points_);
    if ((dev_xpos + dev_width) > max_width_)
        max_width_ = dev_xpos + dev_width;

    canvas_elems_.push_back(std::move(device));
    return conn_pos_pair;
}

PointDesc
PCITopoUIComp::AddRootBus(const pci::PCIBus &bus, uint16_t *x, uint16_t *y)
{
    auto root_bus = std::make_shared<CanvasElemBus>(bus, *x, *y);
    auto conn_pos = root_bus->GetConnPos();
    canvas_elems_.push_back(std::move(root_bus));
    *y += (1 + 2) * 4;

    return conn_pos;
}

void PCITopoUIComp::AddBusDevices(const pci::PCIBus &current_bus,
                                  const std::map<uint16_t, pci::PCIBus> &bus_map,
                                  PointDesc parent_conn_pos,
                                  uint16_t x_off, uint16_t *y_off)
{
    auto bus_connector = std::make_shared<CanvasElemConnector>();

    for (const auto &dev : current_bus.devs_) {
        auto [conn_pos_as_child, conn_pos_as_parent] = AddDevice(dev, x_off, y_off);

        bus_connector->AddLine({ { parent_conn_pos.first, conn_pos_as_child.second },
                                 { conn_pos_as_child.first, conn_pos_as_child.second } });

        if (dev->type_ == pci::pci_dev_type::TYPE1) {
            auto type1_dev = dynamic_cast<pci::PciType1Dev *>(dev.get());
            auto sec_bus_iter = bus_map.find(type1_dev->get_sec_bus_num());
            if (sec_bus_iter != bus_map.end()) {
                AddBusDevices(sec_bus_iter->second, bus_map, conn_pos_as_parent,
                              x_off + 16, y_off);
            }
        }
    }

    if (!bus_connector->lines_.empty()) {
        auto last_line = bus_connector->lines_.back();
        auto [x1_last, y1_last] = last_line.first;

        bus_connector->AddLine({ { parent_conn_pos.first, parent_conn_pos.second },
                                 { x1_last, y1_last} });
        canvas_elems_.push_back(std::move(bus_connector));
    }
}

void PCITopoUIComp::SwitchDrawingMode(ElemReprMode new_mode)
{
    if (current_drawing_mode_ == new_mode)
        return;

    current_drawing_mode_ = new_mode;

    block_map_.Reset();
    canvas_elems_.clear();
    max_width_ = 0;

    auto [w, h] = GetCanvasSizeEstimate(topo_ctx_, current_drawing_mode_);
    canvas_ = ScrollableCanvas(w, h);

    AddTopologyElements();
}

// Returns approximate canvas size based on the actual topology
// X, Y is in dots
std::pair<uint16_t, uint16_t>
GetCanvasSizeEstimate(const pci::PCITopologyCtx &ctx, ElemReprMode mode) noexcept
{
    uint16_t x_size = 0, y_size = 0;
    uint16_t root_bus_elem_height = 3 * sym_height;
    uint16_t dev_elem_height = mode == ElemReprMode::Verbose ?
                                       5 * sym_height :
                                       3 * sym_height;

    auto root_bus_num = std::ranges::count_if(ctx.buses_, [](auto bus) { return bus.second.is_root_; });
    y_size += root_bus_num * root_bus_elem_height;

    auto dev_cnt = ctx.devs_.size();
    y_size += dev_cnt * dev_elem_height;

    y_size += 16;

    // Width of the canvas depends on the actual devices placement,
    // so it's a constant for now
    x_size = 500;
    logger.info("Estimated canvas size: {} x {}", x_size, y_size);

    return {x_size, y_size};
}

//
// push-pull button stuff
//
ftxui::Element PushPullButton::Render()
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

    const ftxui::EntryState state = {
        *label,
        is_pressed_,
        active,
        focused_or_hover,
    };

    auto element = transform(state);
    return element | AnimatedColorStyle() | focus_mgmt | ftxui::reflect(box_);
}

ftxui::Decorator PushPullButton::AnimatedColorStyle()
{
    ftxui::Decorator style = ftxui::nothing;
    if (animated_colors.background.enabled) {
      style = style | ftxui::bgcolor(ftxui::Color::Interpolate(animation_foreground_,  //
                                                animated_colors.background.inactive,
                                                animated_colors.background.active));
    }

    if (animated_colors.foreground.enabled) {
        style = style |
            ftxui::color(ftxui::Color::Interpolate(animation_foreground_,  //
                                           animated_colors.foreground.inactive,
                                           animated_colors.foreground.active));
    }
    return style;
}

void PushPullButton::SetAnimationTarget(float target) {
    if (animated_colors.foreground.enabled) {
      animator_foreground_ = ftxui::animation::Animator(
          &animation_foreground_, target, animated_colors.foreground.duration,
          animated_colors.foreground.function);
    }
    if (animated_colors.background.enabled) {
      animator_background_ = ftxui::animation::Animator(
          &animation_background_, target, animated_colors.background.duration,
          animated_colors.background.function);
    }
}

void PushPullButton::OnAnimation(ftxui::animation::Params& p)
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

bool PushPullButton::OnEvent(ftxui::Event event)
{
    if (event.is_mouse()) {
        return OnMouseEvent(event);
    }

    if (event == ftxui::Event::Return) {
        is_pressed_ = !is_pressed_;
        OnClick();
        return true;
    }
    return false;
}

bool PushPullButton::OnMouseEvent(ftxui::Event event)
{
    mouse_hover_ =
        box_.Contain(event.mouse().x, event.mouse().y) && CaptureMouse(event);

    if (!mouse_hover_) {
        return false;
    }

    if (event.mouse().button == ftxui::Mouse::Left &&
        event.mouse().motion == ftxui::Mouse::Pressed) {
        is_pressed_ = !is_pressed_;
        TakeFocus();
        OnClick();
        return true;
    }

    return false;
}
////////////////////////

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

[[maybe_unused]]static ftxui::Component
RegNotImplComp()
{
    return ftxui::Renderer([&] { return ftxui::text("< not impl >") | ftxui::bold; });
}

//class FocusableComp : public ftxui::ComponentBase
//{
//public:
//    FocusableComp(ftxui::Element elem) : elem_(elem) {}
//    ftxui::Element Render() final
//    {
//        const bool is_focused = Focused();
//        const bool is_active = Active();
//        auto focus_mgmt = is_focused ? ftxui::focus :
//                          is_active ? ftxui::select :
//                          ftxui::nothing;
//        return elem_ | focus_mgmt; // | ftxui::reflect(box_);
//    }
//
//    bool Focusable() const final { return true; }
//private:
//    ftxui::Element elem_;
//};
//
//static auto MakeFocusableComp(ftxui::Element elem)
//{
//    return ftxui::Make<FocusableComp>(elem);
//}

// scrollable component
ftxui::Element ScrollableComp::Render() {
    auto focused = Focused() ? ftxui::focus : ftxui::select;
    auto style = ftxui::nothing;

    ftxui::Element background = ftxui::ComponentBase::Render();
    background->ComputeRequirement();
    size_ = background->requirement().min_y;

    return ftxui::dbox({
        std::move(background),
        ftxui::vbox({
            ftxui::text(L"") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, selected_),
            ftxui::text(L"") | style | focused,
        }),
    }) |
    ftxui::vscroll_indicator | ftxui::yframe | ftxui::yflex | ftxui::reflect(box_);
}

bool ScrollableComp::OnEvent(ftxui::Event event) {
    if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
        TakeFocus();

    int selected_old = selected_;
    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k') ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelUp)) {
        selected_ -= 2;
    }

    if ((event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j') ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelDown))) {
        selected_ += 2;
    }

    if (event == ftxui::Event::PageDown)
        selected_ += box_.y_max - box_.y_min;
    if (event == ftxui::Event::PageUp)
        selected_ -= box_.y_max - box_.y_min;
    if (event == ftxui::Event::Home)
        selected_ = 0;
    if (event == ftxui::Event::End)
        selected_ = size_;

    selected_ = std::max(0, std::min(size_ - 1, selected_));
    return selected_old != selected_;
}

static ftxui::Component MakeScrollableComp(ftxui::Component child)
{
    return ftxui::Make<ScrollableComp>(std::move(child));
}


static ftxui::Component
GetCompMaybe(ftxui::Element elem, const uint8_t *on_click)
{
    return ftxui::Renderer([=] {
                return ftxui::vbox({std::move(elem), ftxui::separatorEmpty()});
           }) | ftxui::Maybe([on_click] { return *on_click == 1; });
}

static ftxui::Component
CreateRegInfoCompat(const compat_reg_type_t reg_type, ftxui::Element content,
                    const uint8_t *on_click)
{
    auto title = ftxui::text(fmt::format("Compat Cfg Space Hdr -> {}", RegTypeLabel(reg_type))) |
                             ftxui::inverted | ftxui::align_right | ftxui::bold;
    auto info_window = ftxui::window(std::move(title), std::move(content));
    return GetCompMaybe(std::move(info_window), on_click);
}

static ftxui::Component
RegInfoVIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = ftxui::text(fmt::format("[{:02x}] -> {}", dev->get_vendor_id(),
                                        dev->ids_names_[pci::VENDOR].empty() ?
                                        "( empty )" : dev->ids_names_[pci::VENDOR]));
    return CreateRegInfoCompat(Type0Cfg::vid, std::move(content), on_click);
}

static ftxui::Component
RegInfoDevIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = ftxui::text(fmt::format("[{:02x}] -> {}", dev->get_device_id(),
                                        dev->ids_names_[pci::DEVICE].empty() ?
                                        "( empty )" : dev->ids_names_[pci::DEVICE]));
    return CreateRegInfoCompat(Type0Cfg::dev_id, std::move(content), on_click);
}

static ftxui::Element
CreateRegFieldElem(const uint8_t fb, const uint8_t lb, std::string desc = " - ",
                   bool should_highlight = false)
{
    auto field_pos_desc = ftxui::text(fmt::format("[ {:2} : {:2} ]", fb, lb));

    if (should_highlight)
        field_pos_desc |= ftxui::bgcolor(ftxui::Color::Green) |
                          ftxui::color(ftxui::Color::Grey15);
    auto desc_text = ftxui::text(desc);

    if (desc == " - ") {
        field_pos_desc |= ftxui::dim;
        desc_text |= ftxui::dim;
    }

    auto elem = ftxui::hbox({
        std::move(field_pos_desc),
        ftxui::separator(),
        std::move(desc_text)
    });

    return elem;
}

static ftxui::Component
RegInfoCommandComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto command = dev->get_command();
    auto reg = reinterpret_cast<const RegCommand *>(&command);

    auto content = ftxui::vbox({
        CreateRegFieldElem(0, 0, " i/o space enabled", reg->io_space_ena == 1),
        CreateRegFieldElem(1, 1, " mem space enabled", reg->mem_space_ena == 1),
        CreateRegFieldElem(2, 2, " bus master enabled", reg->bus_master_ena == 1),
        CreateRegFieldElem(3, 5),
        CreateRegFieldElem(6, 6, " parity err response", reg->parity_err_resp == 1),
        CreateRegFieldElem(7, 7),
        CreateRegFieldElem(8, 8, " serr# enabled", reg->serr_ena == 1),
        CreateRegFieldElem(9, 9),
        CreateRegFieldElem(10, 10, " intr disabled", reg->itr_disable == 1),
        CreateRegFieldElem(11, 15)
    });

    return CreateRegInfoCompat(Type0Cfg::command, std::move(content), on_click);
}

static ftxui::Component
RegInfoStatusComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto status = dev->get_status();
    auto reg = reinterpret_cast<const RegStatus *>(&status);

    auto content = ftxui::vbox({
        CreateRegFieldElem(0, 0, " immediate readiness", reg->imm_readiness == 1),
        CreateRegFieldElem(1, 2),
        CreateRegFieldElem(3, 3, " interrupt status", reg->itr_status == 1),
        CreateRegFieldElem(4, 4, " capabilities list", reg->cap_list == 1),
        CreateRegFieldElem(5, 7),
        CreateRegFieldElem(8, 8, " master data parity error", reg->master_data_parity_err == 1),
        CreateRegFieldElem(9, 10),
        CreateRegFieldElem(11, 11, " signaled target abort", reg->signl_tgt_abort == 1),
        CreateRegFieldElem(12, 12, " received target abort", reg->received_tgt_abort == 1),
        CreateRegFieldElem(13, 13, " received master abort", reg->recevied_master_abort == 1),
        CreateRegFieldElem(14, 14, " signaled system error", reg->signl_sys_err == 1),
        CreateRegFieldElem(15, 15, " detected parity error", reg->detected_parity_err == 1),
    });

    return CreateRegInfoCompat(Type0Cfg::status, std::move(content), on_click);
}

static ftxui::Component
RegInfoRevComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = ftxui::text(fmt::format("{:02x}", dev->get_rev_id()));
    return CreateRegInfoCompat(Type0Cfg::revision, std::move(content), on_click);
}

static ftxui::Component
RegInfoCCComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto cc = dev->get_class_code();

    auto cname = dev->ids_names_[pci::CLASS];
    auto sub_cname = dev->ids_names_[pci::SUBCLASS];
    auto prog_iface = dev->ids_names_[pci::PROG_IFACE];

    auto reg = reinterpret_cast<RegClassCode *>(&cc);

    auto content = ftxui::vbox({
        ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:02x}", reg->base_class_code)) | ftxui::bold,
                ftxui::separatorLight(),
                ftxui::text(fmt::format("{:02x}", reg->sub_class_code)) | ftxui::bold,
                ftxui::separatorLight(),
                ftxui::text(fmt::format("{:02x}", reg->prog_iface)) | ftxui::bold,
            }) | ftxui::borderStyled(ftxui::ROUNDED, ftxui::Color::Blue),
            ftxui::filler()
        }),
        ftxui::separatorEmpty(),
        ftxui::vbox({
            ftxui::text(fmt::format("     class: {:02x} -> {}", reg->base_class_code, cname)),
            ftxui::text(fmt::format("  subclass: {:02x} -> {}", reg->sub_class_code, sub_cname)),
            ftxui::text(fmt::format("prog-iface: {:02x} -> {}", reg->prog_iface, prog_iface))
        })
    });

    return CreateRegInfoCompat(Type0Cfg::class_code, std::move(content), on_click);
}

static ftxui::Component
RegInfoClSizeComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = ftxui::text(fmt::format("Cache Line size: {} bytes",
                               dev->get_cache_line_size() * 4));
    return CreateRegInfoCompat(Type0Cfg::cache_line_size, std::move(content), on_click);
}

static ftxui::Component
RegInfoLatTmrComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto content = ftxui::text(fmt::format("Latency Tmr: {:02x}", dev->get_lat_timer()));
    return CreateRegInfoCompat(Type0Cfg::latency_timer, std::move(content), on_click);
}

static ftxui::Component
RegInfoHdrTypeComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto hdr_type = dev->get_header_type();
    auto reg = reinterpret_cast<const RegHdrType *>(&hdr_type);
    auto desc = fmt::format(" header layout: {}",
                            reg->hdr_layout ? "Type 1" : "Type 0");

    auto content = ftxui::vbox({
        CreateRegFieldElem(0, 6, desc),
        CreateRegFieldElem(7, 7, " multi-function device", reg->is_mfd == 1)
    });
    return CreateRegInfoCompat(Type0Cfg::header_type, std::move(content), on_click);
}

static ftxui::Component
RegInfoBISTComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto bist = dev->get_bist();
    auto reg = reinterpret_cast<const RegBIST *>(&bist);

    auto content = ftxui::vbox({
        CreateRegFieldElem(0, 3, fmt::format(" completion code: {}", reg->cpl_code)),
        CreateRegFieldElem(4, 5),
        CreateRegFieldElem(6, 6, " start BIST", reg->start_bist == 1),
        CreateRegFieldElem(7, 7, " BIST capable", reg->bist_cap == 1)
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

static ftxui::Element
GetBarElem(UIBarElemType type, uint32_t bar)
{
    ftxui::Element reg_box;

    if (type == UIBarElemType::Empty) {
        reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:032b}", bar)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
            }) | ftxui::border,
            ftxui::filler()
        });
    } else if (type == UIBarElemType::IoSpace) {
        auto reg = reinterpret_cast<const RegBARIo *>(&bar);

        // double hbox is needed to align element
        reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:030b}", reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(std::to_string(0)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Yellow),
                ftxui::separator(),
                ftxui::text(fmt::format("{:01b}", reg->space_type)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });
    } else if (type == UIBarElemType::Memory) {
        auto reg = reinterpret_cast<const RegBARMem *>(&bar);
        reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:028b}", reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(fmt::format("{:01b}", reg->prefetch)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Yellow),
                ftxui::separator(),
                ftxui::text(fmt::format("{:02b}", reg->type)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Blue),
                ftxui::separator(),
                ftxui::text(fmt::format("{:01b}", reg->space_type)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });
    } else { // expansion rom
        auto reg = reinterpret_cast<const RegExpROMBar *>(&bar);
        reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:021b}", reg->bar)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(fmt::format("{:010b}", reg->rsvd)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Yellow),
                ftxui::separator(),
                ftxui::text(fmt::format("{:01b}", reg->ena)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });
    }

    return reg_box;
}

static ftxui::Component
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

    ftxui::Element content;

    uint32_t prev_bar_idx = (bar_idx > 0) ? (bar_idx - 1) : 0;

    const auto &cur_bar_res = dev->bar_res_[bar_idx];
    const auto &prev_bar_res = dev->bar_res_[prev_bar_idx];

    if (cur_bar_res.type_ == pci::ResourceType::IO) {
        auto reg_box = GetBarElem(UIBarElemType::IoSpace, bar);
        auto desc = ftxui::vbox({
            ftxui::text(fmt::format("phys address: {:#x}", cur_bar_res.phys_addr_)),
            ftxui::text(fmt::format("size: {:#x}", cur_bar_res.len_))
        });

        content = ftxui::vbox({
            ftxui::text("I/O space:") | ftxui::bold,
            ftxui::separatorLight(),
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
            content = ftxui::vbox({
                ftxui::text(fmt::format("Upper 32 bits of address in BAR{}:",
                                        prev_bar_idx)) | ftxui::bold,
                ftxui::separatorLight(),
                reg_box,
                ftxui::text(fmt::format("{:#x}", bar))
            });
        } else {
            content = ftxui::vbox({
                ftxui::text("Uninitialized BAR: ") | ftxui::dim
                            | ftxui::bgcolor(ftxui::Color::Red)
                            | ftxui::color(ftxui::Color::Grey15),
                ftxui::separatorLight(),
                reg_box
            });
        }
    } else { // Memory BAR
        auto reg_box = GetBarElem(UIBarElemType::Memory, bar);
        auto desc = ftxui::vbox({
            ftxui::text(fmt::format("phys address: {:#x}", cur_bar_res.phys_addr_)),
            ftxui::text(fmt::format("        size: {:#x}", cur_bar_res.len_)),
            ftxui::text(fmt::format("      64-bit: {}", cur_bar_res.is_64bit_ ? "▣ " : "☐ ")),
            ftxui::text(fmt::format("prefetchable: {}", cur_bar_res.is_prefetchable_ ? "▣ " : "☐ "))
        });

        content = ftxui::vbox({
            ftxui::text("Memory space:") | ftxui::bold,
            ftxui::separatorLight(),
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static ftxui::Component
RegInfoCardbusCISComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto content = ftxui::text(fmt::format("{:02x}", type0_dev->get_cardbus_cis()));
    return CreateRegInfoCompat(Type0Cfg::cardbus_cis_ptr, std::move(content), on_click);
}

static ftxui::Component
RegInfoSubsysVIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto content = ftxui::text(fmt::format("[{:04x}] -> {}", type0_dev->get_subsys_vid(),
                                        dev->ids_names_[pci::SUBSYS_NAME].empty() ?
                                        dev->ids_names_[pci::SUBSYS_VENDOR] :
                                        dev->ids_names_[pci::SUBSYS_NAME]));
    return CreateRegInfoCompat(Type0Cfg::subsys_vid, std::move(content), on_click);
}

static ftxui::Component
RegInfoSubsysIDComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto content = ftxui::text(fmt::format("[{:04x}] -> {}", type0_dev->get_subsys_dev_id(),
                                        dev->ids_names_[pci::SUBSYS_NAME].empty() ?
                                        dev->ids_names_[pci::SUBSYS_VENDOR] :
                                        dev->ids_names_[pci::SUBSYS_NAME]));
    return CreateRegInfoCompat(Type0Cfg::subsys_dev_id, std::move(content), on_click);
}

static ftxui::Component
RegInfoExpROMComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                  const uint8_t *on_click)
{
    ftxui::Element content;
    auto exp_rom_bar = dev->get_exp_rom_bar();

    if (exp_rom_bar != 0) {
        auto reg_box = GetBarElem(UIBarElemType::Exp, exp_rom_bar);
        auto [start, end, flags] = dev->resources_[6];
        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("phys address: {:#x}", start)),
            ftxui::text(fmt::format("        size: {:#x}", end - start + 1)),
            ftxui::text(fmt::format("     enabled: {}", (exp_rom_bar & 0x1) ? "▣ " : "☐ "))
        });
    } else {
        auto reg_box = GetBarElem(UIBarElemType::Empty, exp_rom_bar);
        content = ftxui::vbox({
            ftxui::text("Uninitialized Expansion ROM: ") | ftxui::dim
                        | ftxui::bgcolor(ftxui::Color::Red)
                        | ftxui::color(ftxui::Color::Grey15),
            ftxui::separatorLight(),
            reg_box
        });
    }

    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static ftxui::Component
RegInfoCapPtrComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                  const uint8_t *on_click)
{
    auto cap_ptr = dev->get_cap_ptr();
    cap_ptr &= 0xfc;
    auto content = ftxui::hbox({
            ftxui::text("Address of the first capability within PCI-compat cfg space: "),
            ftxui::text(fmt::format("[{:#x}]", cap_ptr)) | ftxui::bold
                        | ftxui::bgcolor(ftxui::Color::Green)
                        | ftxui::color(ftxui::Color::Grey15)
    });
    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static ftxui::Component
RegInfoItrLineComp(const pci::PciDevBase *dev, const compat_reg_type_t reg_type,
                   const uint8_t *on_click)
{
    auto itr_line = dev->get_itr_line();
    auto content = ftxui::text(fmt::format("IRQ [{:#x}]", itr_line));
    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static ftxui::Component
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

    auto content = ftxui::text(fmt::format("[{:#x}] -> {}", itr_pin, desc));
    return CreateRegInfoCompat(reg_type, std::move(content), on_click);
}

static ftxui::Component
RegInfoMinGntComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto min_gnt = type0_dev->get_min_gnt();
    auto content = ftxui::text(fmt::format("[{:#x}]", min_gnt));
    return CreateRegInfoCompat(Type0Cfg::min_gnt, std::move(content), on_click);
}

static ftxui::Component
RegInfoMaxLatComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type0_dev = dynamic_cast<const pci::PciType0Dev *>(dev);
    auto max_lat = type0_dev->get_max_lat();
    auto content = ftxui::text(fmt::format("[{:#x}]", max_lat));
    return CreateRegInfoCompat(Type0Cfg::max_lat, std::move(content), on_click);
}

static ftxui::Component
RegInfoPrimBusNumComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto prim_bus = type1_dev->get_prim_bus_num();
    auto content = ftxui::text(fmt::format("[{:#x}]", prim_bus));
    return CreateRegInfoCompat(Type1Cfg::prim_bus_num, std::move(content), on_click);
}

static ftxui::Component
RegInfoSecBusNumComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto sec_bus = type1_dev->get_sec_bus_num();
    auto content = ftxui::text(fmt::format("[{:#x}]", sec_bus));
    return CreateRegInfoCompat(Type1Cfg::sec_bus_num, std::move(content), on_click);
}

static ftxui::Component
RegInfoSubBusNumComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto sub_bus = type1_dev->get_sub_bus_num();
    auto content = ftxui::text(fmt::format("[{:#x}]", sub_bus));
    return CreateRegInfoCompat(Type1Cfg::sub_bus_num, std::move(content), on_click);
}

static ftxui::Component
RegInfoSecLatTmrComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto content = ftxui::text(fmt::format("Sec Latency Tmr: {:02x}", type1_dev->get_sec_lat_timer()));
    return CreateRegInfoCompat(Type1Cfg::sec_lat_timer, std::move(content), on_click);
}

static ftxui::Component
RegInfoIOBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_base = type1_dev->get_io_base();
    ftxui::Element content;
    if (io_base == 0) {
        content = ftxui::text(fmt::format("[{:02x}] -> uninitialized", io_base));
    } else {
        auto io_base_reg = reinterpret_cast<const RegIOBase *>(&io_base);
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:04b}", io_base_reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(fmt::format("{:04b}", io_base_reg->cap)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });

        uint32_t addr = io_base_reg->addr << 12;
        if (io_base_reg->cap == 1) {
            auto io_base_upper_reg = type1_dev->get_io_base_upper();
            addr |= io_base_upper_reg << 16;
        }

        auto desc = ftxui::vbox({
            ftxui::text(fmt::format("i/o base address:     {:#04x}", addr)),
            ftxui::text(fmt::format("i/o addressing width: {}", io_base_reg->cap == 0 ?
                                                                "16-bit" : "32-bit")),
        });

        content = ftxui::vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_base, std::move(content), on_click);
}

static ftxui::Component
RegInfoIOLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_base = type1_dev->get_io_base();
    auto io_limit = type1_dev->get_io_limit();
    ftxui::Element content;
    if (io_limit == 0 && !io_base) {
        content = ftxui::text(fmt::format("[{:02x}] -> uninitialized", io_limit));
    } else {
        auto io_limit_reg = reinterpret_cast<const RegIOLimit *>(&io_limit);
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:04b}", io_limit_reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(fmt::format("{:04b}", io_limit_reg->cap)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });

        uint32_t addr = (io_limit_reg->addr << 12) | 0xfff;
        if (io_limit_reg->cap == 1) {
            auto io_limit_upper_reg = type1_dev->get_io_limit_upper();
            addr |= io_limit_upper_reg << 16;
        }

        auto desc = ftxui::vbox({
            ftxui::text(fmt::format("           i/o limit: {:#04x}", addr)),
            ftxui::text(fmt::format("i/o addressing width: {}", io_limit_reg->cap == 0 ?
                                                                "16-bit" : "32-bit")),
        });

        content = ftxui::vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_limit, std::move(content), on_click);
}

static ftxui::Component
RegInfoUpperIOBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_base = type1_dev->get_io_base();
    auto io_base_reg = reinterpret_cast<const RegIOBase *>(&io_base);

    ftxui::Element content;

    if (io_base_reg->cap == 1) {
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:016b}", type1_dev->get_io_base_upper())) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green)
            }) | ftxui::border,
            ftxui::filler()
        });

        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("Upper 16 bits of I/O Base: {:04x}",
                                    type1_dev->get_io_base_upper()))
        });
    } else {
        content = ftxui::vbox({
            ftxui::text(fmt::format("[{}]: 32-bit addressing is not supported",
                                    type1_dev->get_io_base_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_base_upper, std::move(content), on_click);
}

static ftxui::Component
RegInfoUpperIOLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto io_limit = type1_dev->get_io_limit();
    auto io_limit_reg = reinterpret_cast<const RegIOLimit *>(&io_limit);

    ftxui::Element content;

    if (io_limit_reg->cap == 1) {
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:16b}", type1_dev->get_io_limit_upper())) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green)
            }) | ftxui::border,
            ftxui::filler()
        });

        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("Upper 16 bits of I/O Limit: {:04x}",
                                    type1_dev->get_io_limit_upper()))
        });
    } else {
        content = ftxui::vbox({
            ftxui::text(fmt::format("[{}]: 32-bit addressing is not supported",
                                    type1_dev->get_io_limit_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::io_limit_upper, std::move(content), on_click);
}

static ftxui::Component
RegInfoSecStatusComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto sec_status = type1_dev->get_sec_status();
    auto reg = reinterpret_cast<const RegSecStatus *>(&sec_status);

    auto content = ftxui::vbox({
        CreateRegFieldElem(0, 4),
        CreateRegFieldElem(5, 5,   " 66 MHz capable", reg->mhz66_cap == 1),
        CreateRegFieldElem(6, 6),
        CreateRegFieldElem(7, 7,   " fast b2b transactions capable", reg->fast_b2b_trans_cap == 1),
        CreateRegFieldElem(8, 8,   " master data parity error", reg->master_data_par_err == 1),
        CreateRegFieldElem(9, 10,  fmt::format(" DEVSEL timing: {}", reg->devsel_timing)),
        CreateRegFieldElem(11, 11, " signaled target abort", reg->signaled_tgt_abort == 1),
        CreateRegFieldElem(12, 12, " received target abort", reg->recv_tgt_abort == 1),
        CreateRegFieldElem(13, 13, " received master abort", reg->recv_master_abort == 1),
        CreateRegFieldElem(14, 14, " received system error", reg->recv_sys_err == 1),
        CreateRegFieldElem(15, 15, " detected parity error", reg->detect_parity_err == 1),
    });

    return CreateRegInfoCompat(Type1Cfg::sec_status, std::move(content), on_click);
}

static ftxui::Component
RegInfoMemoryBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto mem_base = type1_dev->get_mem_base();
    ftxui::Element content;
    if (mem_base == 0) {
        content = ftxui::text(fmt::format("[{:02x}] -> uninitialized", mem_base));
    } else {
        auto mem_base_reg = reinterpret_cast<const RegMemBL *>(&mem_base);
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:012b}", mem_base_reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
                ftxui::separator(),
                ftxui::text(fmt::format("{:04b}", mem_base_reg->rsvd)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
            }) | ftxui::border,
            ftxui::filler()
        });

        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("mem base address: {:#x}",
                        (uint32_t)mem_base_reg->addr << 20))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::mem_base, std::move(content), on_click);
}

static ftxui::Component
RegInfoMemoryLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto mem_base = type1_dev->get_mem_base();
    auto mem_limit = type1_dev->get_mem_limit();
    ftxui::Element content;
    if (mem_limit == 0 && !mem_base) {
        content = ftxui::text(fmt::format("[{:02x}] -> uninitialized", mem_limit));
    } else {
        auto mem_limit_reg = reinterpret_cast<const RegMemBL *>(&mem_limit);
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:12b}", mem_limit_reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
                ftxui::separator(),
                ftxui::text(fmt::format("{:04b}", mem_limit_reg->rsvd)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
            }) | ftxui::border,
            ftxui::filler()
        });

        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("mem limit: {:#x}",
                        (uint32_t)mem_limit_reg->addr << 20 | 0xfffff)),
        });
    }

    return CreateRegInfoCompat(Type1Cfg::mem_limit, std::move(content), on_click);
}

static ftxui::Component
RegInfoPrefMemBaseComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_base = type1_dev->get_pref_mem_base();
    ftxui::Element content;
    if (pref_mem_base == 0) {
        content = ftxui::text(fmt::format("[{:02x}] -> uninitialized", pref_mem_base));
    } else {
        auto pref_mem_base_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_base);
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:012b}", pref_mem_base_reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(fmt::format("{:04b}", pref_mem_base_reg->cap)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });

        uint64_t addr = (uint64_t)pref_mem_base_reg->addr << 20;
        if (pref_mem_base_reg->cap == 1) {
            auto pref_mem_base_upper = type1_dev->get_pref_base_upper();
            addr |= ((uint64_t)pref_mem_base_upper << 32);
        }

        auto desc = ftxui::vbox({
            ftxui::text(fmt::format("prefetchable mem base address:     {:#x}", addr)),
            ftxui::text(fmt::format("prefetchable mem addressing width: {}",
                                    pref_mem_base_reg->cap == 0 ? "32-bit" : "64-bit"))
        });

        content = ftxui::vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_mem_base, std::move(content), on_click);
}

static ftxui::Component
RegInfoPrefMemLimitComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_base = type1_dev->get_pref_mem_base();
    auto pref_mem_limit = type1_dev->get_pref_mem_limit();
    ftxui::Element content;
    if (pref_mem_limit == 0 && !pref_mem_base) {
        content = ftxui::text(fmt::format("[{:02x}] -> uninitialized", pref_mem_limit));
    } else {
        auto pref_mem_limit_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_limit);
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:012b}", pref_mem_limit_reg->addr)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green),
                ftxui::separator(),
                ftxui::text(fmt::format("{:04b}", pref_mem_limit_reg->cap)) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Magenta),
            }) | ftxui::border,
            ftxui::filler()
        });

        uint64_t limit = (uint64_t)pref_mem_limit_reg->addr << 20 | 0xfffff;
        if (pref_mem_limit_reg->cap == 1) {
            auto pref_mem_limit_upper = type1_dev->get_pref_limit_upper();
            limit |= (uint64_t)pref_mem_limit_upper << 32;
        }

        auto desc = ftxui::vbox({
            ftxui::text(fmt::format("prefetchable mem limit:            {:#x}", limit)),
            ftxui::text(fmt::format("prefetchable mem addressing width: {}",
                                    pref_mem_limit_reg->cap == 0 ? "32-bit" : "64-bit"))
        });

        content = ftxui::vbox({
            reg_box,
            desc
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_mem_limit, std::move(content), on_click);
}

static ftxui::Component
RegInfoPrefBaseUpperComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_base = type1_dev->get_pref_mem_base();
    auto pref_mem_base_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_base);

    ftxui::Element content;

    if (pref_mem_base_reg->cap == 1) {
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:032b}", type1_dev->get_pref_base_upper())) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green)
            }) | ftxui::border,
            ftxui::filler()
        });

        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("Upper 32 bits of prefetchable mem base: {:#x}",
                                    type1_dev->get_pref_base_upper()))
        });
    } else {
        content = ftxui::vbox({
            ftxui::text(fmt::format("[{}]: 64-bit addressing is not supported",
                                    type1_dev->get_pref_base_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_base_upper, std::move(content), on_click);
}

static ftxui::Component
RegInfoPrefLimitUpperComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto pref_mem_limit = type1_dev->get_pref_mem_limit();
    auto pref_mem_limit_reg = reinterpret_cast<const RegPrefMemBL *>(&pref_mem_limit);

    ftxui::Element content;

    if (pref_mem_limit_reg->cap == 1) {
        auto reg_box = ftxui::hbox({
            ftxui::hbox({
                ftxui::text(fmt::format("{:032b}", type1_dev->get_pref_limit_upper())) |
                         ftxui::color(ftxui::Color::Grey15) |
                         ftxui::bgcolor(ftxui::Color::Green)
            }) | ftxui::border,
            ftxui::filler()
        });

        content = ftxui::vbox({
            reg_box,
            ftxui::text(fmt::format("Upper 32 bits of prefetchable mem limit: {:#x}",
                                    type1_dev->get_pref_limit_upper()))
        });
    } else {
        content = ftxui::vbox({
            ftxui::text(fmt::format("[{}]: 64-bit addressing is not supported",
                                    type1_dev->get_pref_limit_upper()))
        });
    }

    return CreateRegInfoCompat(Type1Cfg::pref_limit_upper, std::move(content), on_click);
}

static ftxui::Component
RegInfoBridgeCtrlComp(const pci::PciDevBase *dev, const uint8_t *on_click)
{
    auto type1_dev = dynamic_cast<const pci::PciType1Dev *>(dev);
    auto bridge_ctrl = type1_dev->get_bridge_ctl();
    auto reg = reinterpret_cast<const RegBridgeCtl *>(&bridge_ctrl);

    auto content = ftxui::vbox({
        CreateRegFieldElem(0, 0,   " parity error response enable", reg->parity_err_resp_ena == 1),
        CreateRegFieldElem(1, 1,   " #SERR enable", reg->serr_ena == 1),
        CreateRegFieldElem(2, 2,   " ISA enable", reg->isa_ena == 1),
        CreateRegFieldElem(3, 3,   " VGA enable", reg->vga_ena == 1),
        CreateRegFieldElem(4, 4,   " VGA 16-bit decode", reg->vga_16bit_decode == 1),
        CreateRegFieldElem(5, 5,   " master abort mode", reg->master_abort_mode == 1),
        CreateRegFieldElem(6, 6,   " secondary bus reset", reg->sec_bus_reset),
        CreateRegFieldElem(7, 7,   " fast b2b transactions enable", reg->fast_b2b_trans_ena == 1),
        CreateRegFieldElem(8, 8,   " primary discard timer", reg->prim_discard_tmr == 1),
        CreateRegFieldElem(9, 9,   " secondary discard timer", reg->sec_discard_tmr == 1),
        CreateRegFieldElem(10, 10, " discard timer status", reg->discard_tmr_status == 1),
        CreateRegFieldElem(11, 11, " discard timer #serr enable", reg->discard_tmr_serr_ena == 1),
    });

    return CreateRegInfoCompat(Type1Cfg::bridge_ctl, std::move(content), on_click);
}

// Create a component showing detailed info about the register in PCI-compatible
// configuration region (first 64 bytes)
static ftxui::Component
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

static ftxui::Component
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

ftxui::Element PCIRegsComponent::Render()
{
    auto selected_device = topology_component_->GetSelectedDev();
    if (cur_dev_ == selected_device) {
        return ftxui::ComponentBase::Render();
    } else {
        cur_dev_ = selected_device;
        DetachAllChildren();
        vis_state_.clear();

        CreateComponent();

        Add(split_comp_);
        assert(ChildCount() == 1);

        return ftxui::ComponentBase::Render();
    }

}

std::string PCIRegsComponent::PrintCurDevInfo() noexcept
{
    auto selected_device = topology_component_->GetSelectedDev();

    return selected_device->dev_id_str_;
}

static ftxui::Component RegButtonComp(std::string label, uint8_t *on_click)
{
    return PPButton(label, [on_click] { *on_click = *on_click ? false : true; },
                    ui::RegButtonDefaultOption());
}

void PCIRegsComponent::CreateComponent()
{
    upper_split_comp_ = ftxui::Container::Vertical({});
    lower_split_comp_ = ftxui::Container::Vertical({});

    // Determine amount of clickable elements -> number of compat registers + number of capabilities
    auto e_cnt = (cur_dev_->type_ == pci::pci_dev_type::TYPE0) ?
                    type0_compat_reg_cnt : type1_compat_reg_cnt;
    e_cnt += cur_dev_->caps_.size();
    std::ranges::fill_n(std::back_inserter(vis_state_), e_cnt, 0);

    size_t i = 0;

    // Some registers in PCI-compatible config space are identical for both type 0 and type 1 devices
    upper_split_comp_->Add(ftxui::Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::dev_id), &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::vid),    &vis_state_[i++])
                      }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::dev_id, &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::vid,    &vis_state_[i - 1]));

    upper_split_comp_->Add(ftxui::Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::status),  &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::command), &vis_state_[i++])
                     }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::status,  &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::command, &vis_state_[i - 1]));

    upper_split_comp_->Add(ftxui::Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::class_code),   &vis_state_[i++]),
                        ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::revision), &vis_state_[i++])
                        })
                     }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::class_code, &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::revision,   &vis_state_[i - 1]));

    upper_split_comp_->Add(ftxui::Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::bist),            &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::header_type),     &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::latency_timer),   &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::cache_line_size), &vis_state_[i++])
                     }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bist,
                                                                  &vis_state_[i - 4]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::header_type,
                                                                  &vis_state_[i - 3]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::latency_timer,
                                                                  &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::cache_line_size,
                                                                  &vis_state_[i - 1]));

    if (cur_dev_->type_ == pci::pci_dev_type::TYPE0) {
        // BARs
        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar0), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar0, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar1), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar1, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar2), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar2, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar3), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar3, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar4), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar4, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar5), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar5, &vis_state_[i++]));

        // Cardbus CIS ptr
        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::cardbus_cis_ptr), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::cardbus_cis_ptr,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::subsys_dev_id), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::subsys_vid),    &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::subsys_dev_id,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::subsys_vid,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::exp_rom_bar), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::exp_rom_bar,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp("Rsvd (0x35)", &vis_state_[i++]),
                            ftxui::Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type0Cfg::cap_ptr), &vis_state_[i++])
                            })
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::cap_ptr,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp("Rsvd (0x38)", &vis_state_[i++])
                         }));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::max_lat),  &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::min_gnt),  &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::itr_pin),  &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::itr_line), &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::max_lat,
                                                                      &vis_state_[i - 4]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::min_gnt,
                                                                      &vis_state_[i - 3]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::itr_pin,
                                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::itr_line,
                                                                      &vis_state_[i - 1]));
    } else { // type 1
        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bar0), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::bar0, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bar1), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::bar1, &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_lat_timer), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sub_bus_num),   &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_bus_num),   &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::prim_bus_num),  &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::sec_lat_timer,
                                                                      &vis_state_[i - 4]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::sub_bus_num,
                                                                      &vis_state_[i - 3]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::sec_bus_num,
                                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::prim_bus_num,
                                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_status), &vis_state_[i++]),
                            ftxui::Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_limit), &vis_state_[i++]),
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_base), &vis_state_[i++])
                            })
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::sec_status,
                                                                      &vis_state_[i - 3]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::io_limit,
                                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::io_base,
                                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::mem_limit), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::mem_base), &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::mem_limit,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::mem_base,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_mem_limit), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_mem_base),  &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_mem_limit,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_mem_base,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_base_upper), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_base_upper,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_limit_upper), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_limit_upper,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_limit_upper), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_base_upper),  &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::io_limit_upper,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::io_base_upper,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp("Rsvd (0x35)", &vis_state_[i++]),
                            ftxui::Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::cap_ptr), &vis_state_[i++])
                            })
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::cap_ptr,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::exp_rom_bar), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::exp_rom_bar,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(ftxui::Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bridge_ctl), &vis_state_[i++]),
                            ftxui::Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::itr_pin), &vis_state_[i++]),
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::itr_line), &vis_state_[i++])
                            })
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::bridge_ctl,
                                                                      &vis_state_[i - 3]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::itr_pin,
                                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::itr_line,
                                                                      &vis_state_[i - 1]));
    }

    auto lower_comp = MakeScrollableComp(lower_split_comp_);

    auto upper_comp_renderer = ftxui::Renderer(upper_split_comp_, [&] {
        return upper_split_comp_->Render() | ftxui::vscroll_indicator |
               ftxui::hscroll_indicator | ftxui::frame;
    });

    split_comp_ = ftxui::ResizableSplit({
            .main = upper_comp_renderer,
            .back = lower_comp,
            .direction = ftxui::Direction::Up,
            .main_size = &split_off_,
            .separator_func = [] { return ftxui::separatorHeavy(); }
    });
}

} //namespace ui
