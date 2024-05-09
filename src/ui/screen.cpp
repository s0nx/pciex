// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include <bitset>
#include "screen.h"

using namespace ftxui;

namespace ui {

void CanvasVisibleArea::shift(CnvShiftDir dir, Box &box)
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
void ScrollableCanvas::DrawBoxLine(ShapeDesc desc, const Color &color)
{
    auto [x, y, len, height] = desc;
    DrawPointLine(      x,          y,       x, y + height, color);
    DrawPointLine(      x, y + height, x + len, y + height, color);
    DrawPointLine(x + len, y + height, x + len,          y, color);
    DrawPointLine(x + len,          y,       x,          y, color);
}

void ScrollableCanvas::DrawBoxLine(ShapeDesc desc,
                                   const Canvas::Stylizer &style)
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
                                   const Canvas::Stylizer &box_style,
                                   const Canvas::Stylizer &text_style)
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
        canvas.DrawBoxLine(points_, [](Pixel &p) {
            p.foreground_color = Color::Palette256::Orange1;
            p.bold = true;
        });
    else
        // to remove element highlight, 'inverse' highlight style has to
        // be explicitly specified
        canvas.DrawBoxLine(points_, [](Pixel &p) {
            p.foreground_color = Color::Default;
            p.bold = false;
        });

    // draw text
    auto x1 = std::get<0>(points_);
    auto y1 = std::get<1>(points_);
    x1 += 4;
    y1 += 4;

    for (size_t i = 0; const auto &text_str : text_data_) {
        if (i++ == 0)
            canvas.DrawText(x1, y1, text_str, [](Pixel &p) {
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
    canvas.DrawBoxLine(points_, [](Pixel &p) {
        p.bold = true;
        p.foreground_color = Color::Magenta;
    });

    auto x1 = std::get<0>(points_);
    auto y1 = std::get<1>(points_);
    x1 += 4;
    y1 += 4;
    canvas.DrawText(x1, y1, bus_id_str_, [](Pixel &p) {
        p.bold = true;
        p.foreground_color = Color::Blue;
    });
}

// mouse click tracking stuff

bool CanvasDevBlockMap::Insert(std::shared_ptr<CanvasElemPCIDev> dev)
{
    auto [x, y, len, height] = dev->GetShapeDesc();
    auto res = blocks_y_dim_.insert({{y, height}, dev});
    return res.second;
}

void CanvasDevBlockMap::SelectDeviceByPos(const uint16_t mouse_x, const uint16_t mouse_y,
                                     ScrollableCanvas &canvas)
{
    auto y_iter = blocks_y_dim_.lower_bound({mouse_y * 4, 0});
    y_iter--;

    if (y_iter != blocks_y_dim_.end())
    {
        auto [x, y, len, height] = y_iter->second->GetShapeDesc();
        auto x_in_pixels = mouse_x * sym_width;

        if (x_in_pixels >= x && x_in_pixels <= x + len) {
            if (selected_dev_iter_ != y_iter) {
                selected_dev_iter_->second->selected_ = false;
                // redraw previously selected element since it's not selected now
                selected_dev_iter_->second->Draw(canvas);

                selected_dev_iter_ = y_iter;
                selected_dev_iter_->second->selected_ = true;

                // redraw newly selected element
                selected_dev_iter_->second->Draw(canvas);
            }
        }
    }
}

void CanvasDevBlockMap::SelectNextPrevDevice(ScrollableCanvas &canvas, bool select_next)
{
    if (select_next) { // move selector to next device
        auto it_next = std::next(selected_dev_iter_);
        if (it_next != blocks_y_dim_.end()) {
            selected_dev_iter_->second->selected_ = false;
            selected_dev_iter_->second->Draw(canvas);

            selected_dev_iter_ = it_next;
            selected_dev_iter_->second->selected_ = true;
            selected_dev_iter_->second->Draw(canvas);
        }
    } else { // move selector to previous device
        if (selected_dev_iter_ != blocks_y_dim_.begin()) {
            selected_dev_iter_->second->selected_ = false;
            selected_dev_iter_->second->Draw(canvas);

            selected_dev_iter_--;
            selected_dev_iter_->second->selected_ = true;
            selected_dev_iter_->second->Draw(canvas);
        }
    }
}

void CanvasDevBlockMap::Reset()
{
    // temporarily preserve currently selected device
    selected_dev_ = selected_dev_iter_->second;
    blocks_y_dim_.clear();
}

// UI topology element impl

static Element
scrollable_canvas(ConstRef<ScrollableCanvas> canvas)
{
    class Impl : public CanvasScrollNodeBase
    {
    public:
        explicit Impl(ConstRef<ScrollableCanvas> canvas) : canvas_(std::move(canvas))
        {
            requirement_.min_x = (canvas->width() + 1) / 2;
            requirement_.min_y = (canvas->height() + 3) / 4;
        }

        const ScrollableCanvas &canvas() final { return *canvas_; }
        ConstRef<ScrollableCanvas> canvas_;
    };

    return std::make_shared<Impl>(canvas);
}


Element PCITopoUIComp::Render()
{
    const bool is_focused = Focused();
    const bool is_active = Active();
    auto focus_mgmt = is_focused ? ftxui::focus :
                      is_active ? ftxui::select :
                      ftxui::nothing;

    return scrollable_canvas(canvas_) |
           focus_mgmt | size(WIDTH, LESS_THAN, max_width_) |
           reflect(box_);
}

bool PCITopoUIComp::OnEvent(Event event)
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

        if (event.mouse().button == Mouse::WheelDown) {
            if (event.mouse().shift)
                area->shift(CnvShiftDir::RIGHT, box_);
            else
                area->shift(CnvShiftDir::DOWN, box_);
        }

        if (event.mouse().button == Mouse::WheelUp) {
            if (event.mouse().shift)
                area->shift(CnvShiftDir::LEFT, box_);
            else
                area->shift(CnvShiftDir::UP, box_);
        }

        if (event.mouse().button == Mouse::Left)
            block_map_.SelectDeviceByPos(event.mouse().x + area->off_x_,
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
        // select next/prev device via 'j'/'k' + shift
        case 'J':
            block_map_.SelectNextPrevDevice(canvas_, true);
            break;
        case 'K':
            block_map_.SelectNextPrevDevice(canvas_, false);
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
    if (block_map_.selected_dev_iter_ == block_map_.blocks_y_dim_.end()) {
        block_map_.selected_dev_iter_ = block_map_.blocks_y_dim_.begin();
        block_map_.selected_dev_iter_->second->selected_ = true;
    } else {
        auto cur_dev = block_map_.selected_dev_->dev_;
        for (auto it = block_map_.blocks_y_dim_.begin();
                    it != block_map_.blocks_y_dim_.end(); it++) {
            if (it->second->dev_ == cur_dev) {
                block_map_.selected_dev_iter_ = it;
                block_map_.selected_dev_iter_->second->selected_ = true;
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
Element PushPullButton::Render()
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

[[maybe_unused]] static Component
NotImplComp()
{
    return Renderer([&] { return text("< not impl >") | bold; });
}

//class FocusableComp : public ComponentBase
//{
//public:
//    FocusableComp(Element elem) : elem_(elem) {}
//    Element Render() final
//    {
//        const bool is_focused = Focused();
//        const bool is_active = Active();
//        auto focus_mgmt = is_focused ? focus :
//                          is_active ? select :
//                          nothing;
//        return elem_ | focus_mgmt; // | reflect(box_);
//    }
//
//    bool Focusable() const final { return true; }
//private:
//    Element elem_;
//};
//
//static auto MakeFocusableComp(Element elem)
//{
//    return Make<FocusableComp>(elem);
//}

// scrollable component
Element ScrollableComp::Render() {
    auto focused = Focused() ? ftxui::focus : ftxui::select;
    auto style = nothing;

    Element background = ComponentBase::Render();
    background->ComputeRequirement();
    size_ = background->requirement().min_y;

    return dbox({
        std::move(background),
        vbox({
            text(L"") | size(HEIGHT, EQUAL, selected_),
            text(L"") | style | focused
        })
    }) |
    vscroll_indicator | yframe | yflex | reflect(box_);
}

bool ScrollableComp::OnEvent(Event event) {
    if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
        TakeFocus();

    int selected_old = selected_;
    if (event == Event::ArrowUp || event == Event::Character('k') ||
        (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
        selected_ -= 2;
    }

    if ((event == Event::ArrowDown || event == Event::Character('j') ||
        (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
        selected_ += 2;
    }

    if (event == Event::PageDown)
        selected_ += box_.y_max - box_.y_min;
    if (event == Event::PageUp)
        selected_ -= box_.y_max - box_.y_min;
    if (event == Event::Home)
        selected_ = 0;
    if (event == Event::End)
        selected_ = size_;

    selected_ = std::max(0, std::min(size_ - 1, selected_));
    return selected_old != selected_;
}

static Component MakeScrollableComp(Component child)
{
    return Make<ScrollableComp>(std::move(child));
}


static Component
GetCompMaybe(Element elem, const uint8_t *on_click)
{
    return Renderer([=] {
                return vbox({std::move(elem), separatorEmpty()});
           }) | Maybe([on_click] { return *on_click == 1; });
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

static Element
RegFieldCompElem(const uint8_t fb, const uint8_t lb, std::string desc = " - ",
                   bool should_highlight = false)
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

// Verbose register field element
static Element
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

        if (vm_info.InfoAvailable()) {
            auto pa_start = cur_bar_res.phys_addr_;
            auto pa_end = cur_bar_res.phys_addr_ + cur_bar_res.len_;
            auto va2pa_info = vm_info.GetMappingInRange(pa_start, pa_end);

            if (!va2pa_info.empty()) {
                content_elems.push_back(separatorEmpty());
                content_elems.push_back(
                        text(fmt::format("v2p mappings for PA range [{:#x} - {:#x}]:",
                                         pa_start, pa_end)));
                for (const auto &vm_e : va2pa_info) {
                    content_elems.push_back(
                        text(fmt::format("VA range [{:#x} - {:#x}] -> PA {:#x} len {:#x}",
                                         vm_e.start_, vm_e.end_, vm_e.pa_, vm_e.len_))
                    );
                }
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

Element PCIRegsComponent::Render()
{
    auto selected_device = topology_component_->GetSelectedDev();
    if (cur_dev_ == selected_device) {
        return ComponentBase::Render();
    } else {
        cur_dev_ = selected_device;
        DetachAllChildren();
        vis_state_.clear();

        // Internal reallocation would break element visibility state tracking,
        // so reserve some space in advance
        vis_state_.reserve(interactive_elem_max_);

        CreateComponent();

        Add(split_comp_);
        assert(ChildCount() == 1);

        return ComponentBase::Render();
    }

}

std::string PCIRegsComponent::PrintCurDevInfo() noexcept
{
    auto selected_device = topology_component_->GetSelectedDev();

    return selected_device->dev_id_str_;
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

static Component RegButtonComp(std::string label, uint8_t *on_click = nullptr)
{
    if (on_click == nullptr)
        return PPButton(label, [] {}, ui::RegButtonDefaultOption());
    else
        return PPButton(label, [on_click] { *on_click = *on_click ? false : true; },
                        ui::RegButtonDefaultOption());
}

// Create type0/type1 configuration space header
void PCIRegsComponent::AddCompatHeaderRegs()
{
    // Determine amount of clickable elements -> number of compat registers
    auto e_cnt = (cur_dev_->type_ == pci::pci_dev_type::TYPE0) ?
                    type0_compat_reg_cnt : type1_compat_reg_cnt;
    std::ranges::fill_n(std::back_inserter(vis_state_), e_cnt, 0);

    size_t i = 0;

    // Some registers in PCI-compatible config space are identical for both type 0 and type 1 devices
    upper_split_comp_->Add(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::dev_id), &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::vid),    &vis_state_[i++])
                      }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::dev_id, &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::vid,    &vis_state_[i - 1]));

    upper_split_comp_->Add(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::status),  &vis_state_[i++]),
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::command), &vis_state_[i++])
                     }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::status,  &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::command, &vis_state_[i - 1]));

    upper_split_comp_->Add(Container::Horizontal({
                        RegButtonComp(ui::RegTypeLabel(Type0Cfg::class_code),   &vis_state_[i++]),
                        Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::revision), &vis_state_[i++])
                        })
                     }));

    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::class_code, &vis_state_[i - 2]));
    lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::revision,   &vis_state_[i - 1]));

    upper_split_comp_->Add(Container::Horizontal({
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
        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar0), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar0, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar1), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar1, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar2), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar2, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar3), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar3, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar4), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar4, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::bar5), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::bar5, &vis_state_[i++]));

        // Cardbus CIS ptr
        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::cardbus_cis_ptr), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::cardbus_cis_ptr,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::subsys_dev_id), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::subsys_vid),    &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::subsys_dev_id,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::subsys_vid,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type0Cfg::exp_rom_bar), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::exp_rom_bar,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp("Rsvd (0x35)"),
                            Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type0Cfg::cap_ptr), &vis_state_[i++])
                            })
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type0Cfg::cap_ptr,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp("Rsvd (0x38)")
                         }));

        upper_split_comp_->Add(Container::Horizontal({
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
        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bar0), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::bar0, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bar1), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::bar1, &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
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

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::sec_status), &vis_state_[i++]),
                            Container::Horizontal({
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

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::mem_limit), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::mem_base), &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::mem_limit,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::mem_base,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_mem_limit), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_mem_base),  &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_mem_limit,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_mem_base,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_base_upper), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_base_upper,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::pref_limit_upper), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::pref_limit_upper,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_limit_upper), &vis_state_[i++]),
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::io_base_upper),  &vis_state_[i++])
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::io_limit_upper,
                                                      &vis_state_[i - 2]));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::io_base_upper,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp("Rsvd (0x35)"),
                            Container::Horizontal({
                                RegButtonComp(ui::RegTypeLabel(Type1Cfg::cap_ptr), &vis_state_[i++])
                            })
                         }));

        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::cap_ptr,
                                                      &vis_state_[i - 1]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::exp_rom_bar), &vis_state_[i])
                         }));
        lower_split_comp_->Add(CompatRegInfoComponent(cur_dev_.get(), Type1Cfg::exp_rom_bar,
                                                      &vis_state_[i++]));

        upper_split_comp_->Add(Container::Horizontal({
                            RegButtonComp(ui::RegTypeLabel(Type1Cfg::bridge_ctl), &vis_state_[i++]),
                            Container::Horizontal({
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

    logger.info("{} -> vis_state size {} i = {}", cur_dev_->dev_id_str_, vis_state_.size(), i);
}

// Create an element representing a hex dump of some buffer
static Element
GetHexDumpElem(std::string desc, const uint8_t *buf, const size_t len,
               uint32_t bytes_per_line = 16)
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

typedef std::pair<Components, Components> capability_comp_ctx;

[[maybe_unused]] static capability_comp_ctx NotImplCap()
{
    return {{}, {}};
}

// Create particular capability delimiter
static Component
CapDelimComp(const pci::CapDesc &cap)
{
    auto type = std::get<0>(cap);
    auto off = std::get<3>(cap);

    std::string label;
    if (type == CapType::compat) {
        auto compat_cap_type = CompatCapID {std::get<1>(cap)};
        label = fmt::format(" {} [compat] ", CompatCapName(compat_cap_type));
    } else {
        auto ext_cap_type = ExtCapID {std::get<1>(cap)};
        label = fmt::format(" {} [extended] ", ExtCapName(ext_cap_type));
    }

    auto comp = Renderer([=] {
        return vbox({
            hbox({
                text(label),
                text(fmt::format("[{:#02x}]", off)) | bold |
                bgcolor(Color::Magenta) | color(Color::Grey15)
            }),
            separatorLight()
        });
    });
    return comp;
}

// Capabilities region delimiter
static Component
CapsDelimComp(const CapType type, const uint8_t caps_num)
{
    auto comp = Renderer([=]{
        return vbox({
                    separatorEmpty(),
                    hbox({
                        separatorLight() | flex,
                        text(fmt::format("[{} {} cap(s)]", caps_num,
                                         type == CapType::compat ?
                                         "compatible" : "extended")) |
                        bold | inverted | center,
                        separatorLight() | flex
                    }),
                    separatorEmpty()
                });
    });
    return comp;
}

// Capability header component
using cap_hdr_type_t = std::variant<CompatCapHdr, ExtCapHdr>;
static Component
CapHdrComp(const cap_hdr_type_t cap_hdr)
{
    Element elem;
    std::visit([&] (auto &&hdr) {
        using T = std::decay_t<decltype(hdr)>;
        if constexpr (std::is_same_v<T, CompatCapHdr>) {
            elem = hbox({
                text(fmt::format("next: {:#02x}", hdr.next_cap)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Green),
                separator(),
                text(fmt::format("id: {:#02x}", hdr.cap_id)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Blue)
            }) | border;
        } else {
            elem = hbox({
                text(fmt::format("next: {:#02x}", hdr.next_cap)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Green),
                separator(),
                text(fmt::format("ver: {:#02x}", hdr.cap_ver)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Yellow),
                separator(),
                text(fmt::format("id: {:#02x}", hdr.cap_id)) |
                     bold | center |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta)
            }) | border;
        }
    }, cap_hdr);

    return Renderer([=] {
        return std::move(elem);
    });
}

static Component
EmptyCapRegComp(const std::string desc)
{
    return Renderer([=] {
        return text(desc) | strikethrough | center | border | flex;
    });
}

static Component
CreateCapRegInfo(const std::string &cap_desc, const std::string &cap_reg, Element content,
                 const uint8_t *on_click)
{
    auto title = text(fmt::format("{} -> {}", cap_desc, cap_reg)) |
                             inverted | align_right | bold;
    auto info_window = window(std::move(title), std::move(content));
    return GetCompMaybe(std::move(info_window), on_click);
}

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

    auto buf_dump_elem = GetHexDumpElem(fmt::format("data [len {:#02x}] >>>", vspec->len),
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
                logger.warn("{}: unexpected virtio cfg type ({}) in vendor spec cap (off {:02x})",
                            dev->dev_id_str_, virtio_struct->cfg_type, off);
            } else {
                content_elems.push_back(separatorEmpty());
                auto vhdr = text("[VirtIO]") | bold | bgcolor(Color::Blue) | color(Color::Grey15);
                content_elems.push_back(hbox({std::move(vhdr), separatorEmpty()}));

                virtio::VirtIOCapID cap_id {virtio_struct->cfg_type};
                content_elems.push_back(text(fmt::format("struct type: [{:#01x}] {}",
                                                         virtio_struct->cfg_type,
                                                         virtio::VirtIOCapIDName(cap_id))));
                // "PCI conf access" layout (0x5) can't be mapped by BAR.
                // It's an alternative access method to conf regions
                if (cap_id != virtio::VirtIOCapID::pci_cfg_acc) {
                    content_elems.push_back(text(fmt::format("        BAR:  {:#01x}", virtio_struct->bar_idx)));
                    content_elems.push_back(text(fmt::format("         id:  {:#02x}", virtio_struct->id)));
                    content_elems.push_back(text(fmt::format(" BAR offset:  {:#x}", virtio_struct->bar_off)));
                    content_elems.push_back(text(fmt::format(" struct len:  {:#x}", virtio_struct->length)));
                }
            }
        }
    }

    auto content_elem = vbox(content_elems);

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] Vendor-Specific", off),
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
        RegFieldVerbElem(0,  2,  fmt::format(" version: {}", pm_cap->pmcap.version),
                                 pm_cap->pmcap.version),
        RegFieldCompElem(3,  3,  " PME clock", pm_cap->pmcap.pme_clk == 1),
        RegFieldCompElem(4,  4,  " imm ready on D0", pm_cap->pmcap.imm_readiness_on_ret_d0 == 1),
        RegFieldCompElem(5,  5,  " device specific init", pm_cap->pmcap.dsi == 1),
        RegFieldVerbElem(6,  8,  fmt::format(" aux current: {} mA",
                                 aux_max_current[pm_cap->pmcap.aux_cur]),
                                 pm_cap->pmcap.aux_cur),
        RegFieldCompElem(9,  9,  " D1 state support", pm_cap->pmcap.d1_support == 1),
        RegFieldCompElem(10, 10, " D2 state support", pm_cap->pmcap.d2_support == 1),
        RegFieldVerbElem(11, 15,
                         fmt::format(" PME support: D0[{}] D1[{}] D2[{}] D3hot[{}] D3cold[{}]",
                                     pme_state_map[0] ? '+' : '-',
                                     pme_state_map[1] ? '+' : '-',
                                     pme_state_map[2] ? '+' : '-',
                                     pme_state_map[3] ? '+' : '-',
                                     pme_state_map[4] ? '+' : '-'),
                         pm_cap->pmcap.pme_support)
    });

    auto pm_ctrl_stat_content = vbox({
        RegFieldVerbElem(0,  1,   fmt::format(" power state: D{}", pm_cap->pmcs.pwr_state),
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

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] Power Management", off),
                                     "PM Capabilities +0x2", std::move(pm_cap_content), &vis[i - 2]));
    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] Power Management", off),
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
        RegFieldVerbElem(1,  3, fmt::format(" multiple msg capable: {}",
                                            multi_msg_map[msi_msg_ctrl_reg->multi_msg_capable]),
                                msi_msg_ctrl_reg->multi_msg_capable),
        RegFieldVerbElem(4,  6, fmt::format(" multiple msg enable: {}",
                                            multi_msg_map[msi_msg_ctrl_reg->multi_msg_ena]),
                                msi_msg_ctrl_reg->multi_msg_ena),
        RegFieldCompElem(7,  7, " 64-bit address", msi_msg_ctrl_reg->addr_64_bit_capable == 1),
        RegFieldCompElem(8,  8, " per-vector masking", msi_msg_ctrl_reg->per_vector_mask_capable == 1),
        RegFieldCompElem(9,  9, " extended msg capable", msi_msg_ctrl_reg->ext_msg_data_capable == 1),
        RegFieldCompElem(10, 10, " extended msg enable", msi_msg_ctrl_reg->ext_msg_data_ena == 1),
        RegFieldCompElem(11, 15)
    });

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
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
                    text(fmt::format("{:030b}", msg_addr_lower)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                    separator(),
                    text(fmt::format("{:02b}", msg_addr_lower & 0x3)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
                }) | border,
                filler()
            }),
            text(fmt::format(" Full address: {:#x}",
                 (uint64_t)msg_addr_upper << 32 | (uint64_t)msg_addr_lower))
        });

        auto uaddr_content = vbox({
            hbox({
                hbox({
                    text(fmt::format("{:032b}", msg_addr_upper)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta)
                }) | border,
                filler()
            }),
            text(fmt::format(" address: {:#x}", msg_addr_upper))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
                                         "Message Address +0x4", std::move(laddr_content),
                                         &vis[i - 2]));
        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
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
                    text(fmt::format("{:030b}", msg_addr_lower)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Green),
                    separator(),
                    text(fmt::format("{:02b}", msg_addr_lower & 0x3)) |
                         color(Color::Grey15) |
                         bgcolor(Color::Magenta),
                }) | border,
                filler()
            }),
            text(fmt::format(" Address: {:#x}", msg_addr_lower))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
                                         "Message Address +0x4", std::move(laddr_content),
                                         &vis[i - 1]));
    }

    // (extended) message data
    std::ranges::fill_n(std::back_inserter(vis), 2, 0);

    auto msg_data_off = msi_msg_ctrl_reg->addr_64_bit_capable ? 0xc : 0x8;
    upper.push_back(Container::Horizontal({
                        RegButtonComp(fmt::format("Extended Message Data +{:#x}", msg_data_off + 0x2),
                                      &vis[i++]),
                        RegButtonComp(fmt::format("Message Data +{:#x}", msg_data_off),
                                      &vis[i++])
                    }));

    auto msg_data = *reinterpret_cast<const uint16_t *>(dev->cfg_space_.get() + off + msg_data_off);
    auto ext_msg_data = *reinterpret_cast<const uint16_t *>
                        (dev->cfg_space_.get() + off + msg_data_off + 0x2);
    auto data_content = text(fmt::format("data: {:#x}", msg_data)) | bold;
    auto ext_data_content = text(fmt::format("extended data: {:#x}", ext_msg_data)) | bold;

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
                                     fmt::format("Extended Message Data +{:#x}", msg_data_off + 0x2),
                                     std::move(ext_data_content), &vis[i - 2]));

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
                                     fmt::format("Message Data +{:#x}", msg_data_off),
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
                            RegButtonComp(fmt::format("Mask Bits +{:#x}", mask_bits_off),
                                          &vis[i++]),
                        }));
        upper.push_back(Container::Horizontal({
                            RegButtonComp(fmt::format("Pending Bits +{:#x}", pending_bits_off),
                                          &vis[i++]),
                        }));

        auto mask_bits_content = hbox({
            hbox({
                text(fmt::format("{:32b}", mask_bits)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta)
            }) | border,
            filler()
        });

        auto pending_bits_content = hbox({
            hbox({
                text(fmt::format("{:32b}", pending_bits)) |
                     color(Color::Grey15) |
                     bgcolor(Color::Magenta)
            }) | border,
            filler()
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
                                         fmt::format("Mask Bits +{:#x}", mask_bits_off),
                                         std::move(mask_bits_content), &vis[i - 2]));

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] MSI", off),
                                         fmt::format("Pending Bits +{:#x}", pending_bits_off),
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
        RegFieldCompElem(0, 3, fmt::format(" Version: {}",
                                           pcie_cap->pcie_cap_reg.cap_ver)),
        RegFieldVerbElem(4, 7, fmt::format(" Device/Port type: '{}'",
                                       dev->type_ == pci::pci_dev_type::TYPE0 ?
                                       PciEDevPortDescType0(pcie_cap->pcie_cap_reg.dev_port_type) :
                                       PciEDevPortDescType1(pcie_cap->pcie_cap_reg.dev_port_type)),
                                pcie_cap->pcie_cap_reg.dev_port_type),
        RegFieldCompElem(8, 8, " Slot implemented", pcie_cap->pcie_cap_reg.slot_impl == 1),
        RegFieldCompElem(9, 13, fmt::format(" ITR message number: {}",
                                pcie_cap->pcie_cap_reg.itr_msg_num)),
        RegFieldCompElem(14, 15)
    });

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
                                     "PCIe Capabilities +0x2", std::move(pcie_cap_reg_content),
                                     &vis[i - 1]));

    // device capabilities
    upper.push_back(Container::Horizontal({
                        RegButtonComp("Device Capabilities +0x4", &vis[i++]),
                    }));

    std::array<uint16_t, 8> pyld_sz_map { 128, 256, 512, 1024, 2048, 4096, 0, 0};

    auto dev_caps_content = vbox({
        RegFieldVerbElem(0, 2, fmt::format(" Max payload size: {}",
                                           pyld_sz_map[pcie_cap->dev_cap.max_pyld_size_supported]),
                                pcie_cap->dev_cap.max_pyld_size_supported),
        RegFieldCompElem(3, 4, fmt::format(" Phantom functions: MSB num {:02b} | {}",
                                           pcie_cap->dev_cap.phan_func_supported,
                                           pcie_cap->dev_cap.phan_func_supported)),
        RegFieldCompElem(5, 5, " Ext tag field supported",
                         pcie_cap->dev_cap.ext_tag_field_supported == 1),
        RegFieldCompElem(6, 8, fmt::format(" EP L0s acceptable latency: {}",
                                           EpL0sAcceptLatDesc(pcie_cap->dev_cap.ep_l0s_accept_lat))),
        RegFieldCompElem(9, 11, fmt::format(" EP L1 acceptable latency: {}",
                                            EpL1AcceptLatDesc(pcie_cap->dev_cap.ep_l1_accept_lat))),
        RegFieldCompElem(12, 14),
        RegFieldCompElem(15, 15, " Role-based error reporting",
                         pcie_cap->dev_cap.role_based_err_rep == 1),
        RegFieldCompElem(16, 17),
        RegFieldCompElem(18, 25, fmt::format(" Captured slot power limit: {:#x}",
                                             pcie_cap->dev_cap.cap_slot_pwr_lim_val)),
        RegFieldVerbElem(26, 27, fmt::format(" Captured slot power scale: {}",
                                        CapSlotPWRScale(pcie_cap->dev_cap.cap_slot_pwr_lim_scale)),
                        pcie_cap->dev_cap.cap_slot_pwr_lim_scale),
        RegFieldCompElem(28, 28, " FLR capable", pcie_cap->dev_cap.flr_cap == 1),
        RegFieldCompElem(29, 31)
    });

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
        RegFieldCompElem(5, 7, fmt::format(" Max TLP payload size: {} bytes",
                                           pyld_sz_map[pcie_cap->dev_ctl.max_pyld_size])),
        RegFieldCompElem(8, 8, " Extended tag field",
                         pcie_cap->dev_ctl.ext_tag_field_ena == 1),
        RegFieldCompElem(9, 9, " Phantom functions",
                         pcie_cap->dev_ctl.phan_func_ena == 1),
        RegFieldCompElem(10, 10, " Aux power PM",
                         pcie_cap->dev_ctl.aux_power_pm_ena == 1),
        RegFieldCompElem(11, 11, " No snoop",
                         pcie_cap->dev_ctl.no_snoop_ena == 1),
        RegFieldCompElem(12, 14, fmt::format(" max READ request size: {} bytes",
                                           pyld_sz_map[pcie_cap->dev_ctl.max_read_req_size])),
        RegFieldCompElem(15, 15, fmt::format("{}",
                                 (dev->type_ == pci::pci_dev_type::TYPE1 &&
                                  pcie_cap->pcie_cap_reg.dev_port_type == 0b0111) ?
                                 " Bridge configuration retry" :
                                 (dev->type_ == pci::pci_dev_type::TYPE0 &&
                                  pcie_cap->pcie_cap_reg.dev_port_type != 0b1010) ?
                                 " Initiate FLR" :
                                 " - "),
                         pcie_cap->dev_ctl.brd_conf_retry_init_flr == 1)
    });

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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

    lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldVerbElem(4, 9, fmt::format(" Max link width: {}",
                                               LinkWidthDesc(pcie_cap->link_cap.max_link_width)),
                             pcie_cap->link_cap.max_link_width),
            RegFieldCompElem(10, 11,
                             fmt::format(" ASPM support [{}]: L0s[{}] L1[{}]",
                                         pcie_cap->link_cap.aspm_support ? '+' : '-',
                                         (pcie_cap->link_cap.aspm_support & 0x1) ? '+' : '-',
                                         (pcie_cap->link_cap.aspm_support & 0x2) ? '+' : '-')),
            RegFieldVerbElem(12, 14, fmt::format(" L0s exit latency: {}",
                                                LinkCapL0sExitLat(pcie_cap->link_cap.l0s_exit_lat)),
                             pcie_cap->link_cap.l0s_exit_lat),
            RegFieldVerbElem(15, 17, fmt::format(" L1 exit latency: {}",
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
            RegFieldCompElem(23, 31, fmt::format(" Port number: {}", pcie_cap->link_cap.port_num))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
                             fmt::format(" ASPM ctrl [{}]: L0s[{}] L1[{}]",
                                         pcie_cap->link_ctl.aspm_ctl ? "+" : "disabled",
                                         (pcie_cap->link_ctl.aspm_ctl & 0x1) ? '+' : '-',
                                         (pcie_cap->link_ctl.aspm_ctl & 0x2) ? '+' : '-')),
            RegFieldCompElem(2, 2),
            RegFieldCompElem(3, 3, fmt::format(" RCB: {}",
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
            RegFieldCompElem(14, 15, fmt::format(" DRS: {}",
                                     LinkCtlDrsSigCtlDesc(pcie_cap->link_ctl.drs_signl_ctl)))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Control +0x10", std::move(link_ctrl_content),
                                         &vis[i - 1]));

        auto link_status_content = vbox({
            RegFieldVerbElem(0, 3, LinkSpeedDesc(LinkSpeedRepType::current,
                                                 pcie_cap->link_status.curr_link_speed,
                                                 pcie_cap->link_cap2),
                             pcie_cap->link_status.curr_link_speed),
            RegFieldVerbElem(4, 9, fmt::format(" Negotiated link width: {}",
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

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldVerbElem(7, 14, fmt::format(" Slot PL value: {}",
                                    SlotCapPWRLimitDesc(pcie_cap->slot_cap.slot_pwr_lim_val)),
                             pcie_cap->slot_cap.slot_pwr_lim_val),
            RegFieldCompElem(15, 16, fmt::format(" Slot PL scale: {}",
                                      CapSlotPWRScale(pcie_cap->slot_cap.slot_pwr_lim_val))),
            RegFieldCompElem(17, 17, " EM interlock present",
                             pcie_cap->slot_cap.em_interlock_pres == 1),
            RegFieldCompElem(18, 18, " No command completed",
                             pcie_cap->slot_cap.no_cmd_cmpl_support == 1),
            RegFieldCompElem(19, 31, fmt::format(" Physical slot number: {:#x}",
                                                 pcie_cap->slot_cap.phys_slot_num))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldCompElem(5, 5, fmt::format(" MRL sensor state: {}",
                             pcie_cap->slot_status.mrl_sens_state == 0x0 ? "closed" : "open")),
            RegFieldCompElem(6, 6, fmt::format(" Presence detect state: {}",
                             pcie_cap->slot_status.pres_detect_state == 0x0 ?
                             "slot empty" : "adapter present")),
            RegFieldCompElem(7, 7, fmt::format(" EM interlock status: {}",
                             pcie_cap->slot_status.em_interlock_status == 0x0 ?
                             "disengaged" : "engaged")),
            RegFieldCompElem(8, 8, " Data link layer state changed",
                             pcie_cap->slot_status.dlink_layer_state_changed == 1),
            RegFieldCompElem(9, 15)
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldCompElem(6, 7, fmt::format(" Attention indicator ctrl: {}",
                             SlotCtlIndCtrlDesc(pcie_cap->slot_ctl.attn_ind_ctl))),
            RegFieldCompElem(8, 9, fmt::format(" Power indicator ctrl: {}",
                             SlotCtlIndCtrlDesc(pcie_cap->slot_ctl.pwr_ind_ctl))),
            RegFieldCompElem(10, 10, fmt::format(" Power controller ctrl: {}",
                             pcie_cap->slot_ctl.pwr_ctl_ctl == 0x0 ? "ON" : "OFF")),
            RegFieldCompElem(11, 11, " EM interlock ctrl"),
            RegFieldCompElem(12, 12, " Data link layer state changed enable",
                             pcie_cap->slot_ctl.dlink_layer_state_changed_ena == 1),
            RegFieldCompElem(13, 13, " Auto slot power limit disabled",
                             pcie_cap->slot_ctl.auto_slow_prw_lim_dis == 1),
            RegFieldCompElem(14, 15)
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldVerbElem(0, 15, fmt::format(" PME requester ID: {:#x}",
                                                pcie_cap->root_status.pme_req_id),
                             pcie_cap->root_status.pme_req_id),
            RegFieldCompElem(16, 16, " PME status", pcie_cap->root_status.pme_status),
            RegFieldCompElem(17, 17, " PME pending", pcie_cap->root_status.pme_pending),
            RegFieldCompElem(18, 31)
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldCompElem(0, 3, fmt::format(" Cmpl timeout ranges: {}",
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
            RegFieldCompElem(12, 13, fmt::format(" TPH completer: TPH[{}] eTPH[{}]",
                             (pcie_cap->dev_cap2.tph_cmpl_support & 0x1) ? '+' : '-',
                             (pcie_cap->dev_cap2.tph_cmpl_support & 0x2) ? '+' : '-')),
            RegFieldCompElem(14, 15, fmt::format(" LN system CLS: {}",
                             DevCap2LNSysCLSDesc(pcie_cap->dev_cap2.ln_sys_cls))),
            RegFieldCompElem(16, 16, " 10-bit tag completer",
                             pcie_cap->dev_cap2.tag_10bit_cmpl_support == 1),
            RegFieldCompElem(17, 17, " 10-bit tag requester",
                             pcie_cap->dev_cap2.tag_10bit_req_support == 1),
            RegFieldCompElem(18, 19, fmt::format(" OBFF[{}]: msg signal [{}] WAKE# signal [{}]",
                             (pcie_cap->dev_cap2.obff_supported == 0x0) ? '-' : '+',
                             (pcie_cap->dev_cap2.obff_supported & 0x1) ? '+' : '-',
                             (pcie_cap->dev_cap2.obff_supported & 0x2) ? '+' : '-')),
            RegFieldCompElem(20, 20, " Ext fmt field",
                             pcie_cap->dev_cap2.ext_fmt_field_support == 1),
            RegFieldCompElem(21, 21, " end-end TLP prefix",
                             pcie_cap->dev_cap2.end_end_tlp_pref_support == 1),
            RegFieldCompElem(22, 23, fmt::format(" max end-end TLP prefixes: {}",
                             (pcie_cap->dev_cap2.max_end_end_tlp_pref == 0) ?
                             0x4 : pcie_cap->dev_cap2.max_end_end_tlp_pref)),
            RegFieldCompElem(24, 25, fmt::format(" Emerg power reduction state: {:#x}",
                              pcie_cap->dev_cap2.emerg_pwr_reduct_support)),
            RegFieldCompElem(26, 26, " Emerg power reduction init required",
                             pcie_cap->dev_cap2.emerg_pwr_reduct_init_req == 1),
            RegFieldCompElem(27, 30),
            RegFieldCompElem(31, 31, " FRS supported",
                             pcie_cap->dev_cap2.frs_support == 1),
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldCompElem(0, 3, fmt::format(" Cmpl timeout value: {}",
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
            RegFieldCompElem(13, 14, fmt::format(" OBFF enable: {}",
                             DevCtl2ObffDesc(pcie_cap->dev_ctl2.obff_ena))),
            RegFieldCompElem(15, 15, fmt::format(" end-end TLP prefix blocking: {}",
                             (pcie_cap->dev_ctl2.end_end_tlp_pref_block == 1) ?
                             "fwd blocked" : "fwd enabled"))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldVerbElem(1, 7, fmt::format(" Supported link speeds: {}",
                             SuppLinkSpeedDesc(pcie_cap->link_cap2.supported_speed_vec)),
                             pcie_cap->link_cap2.supported_speed_vec),
            RegFieldCompElem(8, 8, " Crosslink", pcie_cap->link_cap2.crosslink_support == 1),
            RegFieldVerbElem(9, 15, fmt::format(" Lower SKP OS gen speeds: {}",
                             SuppLinkSpeedDesc(pcie_cap->link_cap2.low_skp_os_gen_supp_speed_vec)),
                             pcie_cap->link_cap2.low_skp_os_gen_supp_speed_vec),
            RegFieldVerbElem(16, 22, fmt::format(" Lower SKP OS reception speeds: {}",
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

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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
            RegFieldCompElem(0, 0, fmt::format(" Current de-emphasis level: {}",
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
            RegFieldCompElem(8, 9, fmt::format(" Crosslink resolution: {}",
                             CrosslinkResDesc(pcie_cap->link_status2.crosslink_resolution))),
            RegFieldCompElem(10, 11),
            RegFieldCompElem(12, 14, fmt::format(" Downstream comp presence: {}",
                              DownstreamCompPresDesc(pcie_cap->link_status2.downstream_comp_pres))),
            RegFieldCompElem(15, 15, " DRS msg received",
                             pcie_cap->link_status2.drs_msg_recv == 1),
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
                                         "Link Status 2 +0x32", std::move(link_stat2_content),
                                         &vis[i - 2]));

        auto link_ctrl2_content = vbox({
            RegFieldCompElem(0, 3, LinkSpeedDesc(LinkSpeedRepType::target,
                                                 pcie_cap->link_ctl2.tgt_link_speed,
                                                 pcie_cap->link_cap2)),
            RegFieldCompElem(4, 4, " Enter Compliance", pcie_cap->link_ctl2.enter_compliance == 1),
            RegFieldCompElem(5, 5, " HW autonomous speed disable",
                             pcie_cap->link_ctl2.hw_auto_speed_dis == 1),
            RegFieldCompElem(6, 6, fmt::format(" Selectable de-emphasis level: {}",
                             pcie_cap->link_ctl2.select_de_emph == 0x0 ?
                             "-6 dB" : "-3.5 dB")),
            RegFieldCompElem(7, 9, fmt::format(" Transmit margin: {}",
                             pcie_cap->link_ctl2.trans_margin == 0x0 ?
                             "normal operation" : "other(tbd)")),
            RegFieldCompElem(10, 10, " Enter modified compliance",
                             pcie_cap->link_ctl2.enter_mod_compliance == 1),
            RegFieldCompElem(11, 11, " Compliance sos",
                             pcie_cap->link_ctl2.compliance_sos == 1),
            RegFieldCompElem(12, 15, fmt::format(" Compliance preset/de-emph: {:#x}",
                             pcie_cap->link_ctl2.compliance_preset_de_emph))
        });

        lower.push_back(CreateCapRegInfo(fmt::format("[compat][{:#02x}] PCI Express", off),
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


// Create clickable components and descriptions for capability
// in PCI-compatible config space (first 256 bytes)
// Each capability might be composed of multiple registers.
static capability_comp_ctx
CompatCapComponents(const pci::PciDevBase *dev, const CompatCapID cap_id,
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
        return NotImplCap();
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

static capability_comp_ctx
ExtendedCapComponents(const pci::PciDevBase *dev, const ExtCapID cap_id,
                      const pci::CapDesc &cap, std::vector<uint8_t> &vis)
{
    return NotImplCap();
}

void PCIRegsComponent::AddCapabilities()
{
    bool compat_delim_present {false}, ext_delim_present {false};
    Components upper_comps, lower_comps;

    for (const auto &cap : cur_dev_->caps_) {
        auto type = std::get<0>(cap);
        auto id   = std::get<1>(cap);

        if (type == CapType::compat) {
            if (!compat_delim_present) {
                upper_split_comp_->Add(CapsDelimComp(CapType::compat,
                                                     cur_dev_->compat_caps_num_));
                compat_delim_present = true;
            }
            CompatCapID cap_id {id};
            std::tie(upper_comps, lower_comps) = CompatCapComponents(cur_dev_.get(), cap_id,
                                                                     cap, vis_state_);
        } else {
            if (!ext_delim_present) {
                upper_split_comp_->Add(CapsDelimComp(CapType::extended,
                                                     cur_dev_->extended_caps_num_));
                ext_delim_present = true;
            }
            ExtCapID cap_id {id};
            std::tie(upper_comps, lower_comps) = ExtendedCapComponents(cur_dev_.get(), cap_id,
                                                                       cap, vis_state_);
        }

        for (const auto &el : upper_comps)
            upper_split_comp_->Add(el);

        for (const auto &el : lower_comps)
            lower_split_comp_->Add(el);
    }
}

void PCIRegsComponent::CreateComponent()
{
    upper_split_comp_ = Container::Vertical({});
    lower_split_comp_ = Container::Vertical({});

    AddCompatHeaderRegs();
    AddCapabilities();

    FinalizeComponent();
}

void PCIRegsComponent::FinalizeComponent()
{
    auto lower_comp = MakeScrollableComp(lower_split_comp_);

    auto upper_comp_renderer = Renderer(upper_split_comp_, [&] {
        return upper_split_comp_->Render() | vscroll_indicator |
               hscroll_indicator | frame;
    });

    split_comp_ = ResizableSplit({
            .main = upper_comp_renderer,
            .back = lower_comp,
            .direction = Direction::Up,
            .main_size = &split_off_,
            .separator_func = [] { return separatorHeavy(); }
    });
}

static Element GetLogo()
{
  std::vector<std::string> lvt {
    R"(  ______   ______     __     ______     __  __    )",
    R"( /\  == \ /\  ___\   /\ \   /\  ___\   /\_\_\_\   )",
    R"( \ \  _-/ \ \ \____  \ \ \  \ \  __\   \/_/\_\/_  )",
    R"(  \ \_\    \ \_____\  \ \_\  \ \_____\   /\_\/\_\ )",
    R"(   \/_/     \/_____/   \/_/   \/_____/   \/_/\/_/ )",
    R"(                                                  )"
  };

  Elements elems;

  for (const auto &el : lvt)
    elems.push_back(text(el) | bold);

  auto logo = vbox(std::move(elems));
  logo |= bgcolor(LinearGradient()
                    .Angle(45)
                    .Stop(Color::Yellow3Bis)
                    .Stop(Color::DeepSkyBlue2)
                 );
  logo |= color(Color::Grey15);

  return logo;
}

static Element GetHelpElem()
{
  std::vector<std::string> lvt {
    R"( General navigation/actions:                                  )",
    R"(                  │ device regs /                             )",
    R"(      device tree │   capabilities                            )",
    R"(         pane     ├────────────────                           )",
    R"(                  │ reg / cap                                 )",
    R"(                  │ detailed info                             )",
    R"(                                                              )",
    R"(  resize pane(s) - drag the border while holding left mouse   )",
    R"(                   button                                     )",
    R"(  TAB/h/k/left click - move focus to specific pane            )",
    R"(                                                              )",
    R"( Pane navigation:                                             )",
    R"(  [h, j, k, l] or arrows - scroll left, down, up, right       )",
    R"(  shift + j/k - select next/previous device in the hierarchy  )",
    R"(                can also be selected with mouse               )",
    R"(                (device tree pane only)                       )",
    R"(  mouse wheel up/down         - vertical scroll               )",
    R"(  shift + mouse wheel up/down - horizontal scroll             )",
    R"(                                (device tree pane only)       )",
    R"(  left click / enter          - show/hide detailed info       )",
    R"(                                (device regs/caps pane only)  )",
    R"( Other hotkeys:                                               )",
    R"(  c/v - device tree pane compact/verbose drawing mode switch  )",
    R"(  ?   - help open/close                                       )",
    R"(                                                              )"
  };

  Elements elems;
  for (const auto &el : lvt)
    elems.push_back(text(el));

  return vbox(std::move(elems));
}

Component GetHelpScreenComp(std::function<void()> hide_fn)
{
    auto help_comp = ftxui::Renderer([&] {
        return ftxui::vbox({
          GetLogo() | center,
          separatorEmpty(),
          GetHelpElem()
        }) | borderDouble;
    });

    return Container::Vertical({
        std::move(help_comp),
        Button("[ X ]", hide_fn, ui::RegButtonDefaultOption())
    });
}

} //namespace ui
