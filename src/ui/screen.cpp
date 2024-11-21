// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#include <fmt/format.h>

#include "log.h"
#include "screen.h"
#include "common_comp.h"
#include "compat_cap_comp.h"
#include "ext_cap_comp.h"
#include "pciex_version.h"

#include <algorithm>

extern Logger logger;

using namespace ftxui;

namespace ui {

void CanvasVisibleArea::shift(UiElemShiftDir dir, Box &box)
{
    switch (dir) {
        case UiElemShiftDir::UP:
            if (off_y_ > 0)
                off_y_ -= 2;
            break;
        case UiElemShiftDir::LEFT:
            if (off_x_ > 0)
                off_x_ -= 2;
            break;
        case UiElemShiftDir::DOWN:
            if (box.y_max >= y_max_) {
                off_y_ = 0;
                return;
            }

            if (off_y_ + box.y_max > y_max_) {
                off_y_ = y_max_ - box.y_max;
                return;
            }

            if (off_y_ + box.y_max < y_max_) {
                off_y_ += 2;
            }

            break;
        case UiElemShiftDir::RIGHT:
            if (box.x_max >= x_max_) {
                off_x_ = 0;
                return;
            }

            if (off_x_ + box.x_max > x_max_) {
                off_x_ = x_max_ - box.x_max;
                return;
            }

            if (off_x_ + box.x_max < x_max_) {
                off_x_ += 2;
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

CanvasElemBus::CanvasElemBus(const pci::PCIBus &bus, uint16_t x, uint16_t y)
    : bus_id_str_(fmt::format("[ {:04x}:{:02x} ]", bus.dom_, bus.bus_nr_))
{
    auto hlen = bus_id_str_.length() * sym_width + 2 * 2;
    auto vlen = sym_height + 2;
    points_ = { x + 1, y + 3, hlen, vlen };
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
    if (event.is_mouse())
    {
        hovered_ = box_.Contain(event.mouse().x, event.mouse().y);

        // XXX: is it necessary ?
        //if (!CaptureMouse(event))
        //    return false;

        if (hovered_)
            TakeFocus();
        else
            return false;
    }

    auto area = canvas_.GetVisibleAreaDesc();

    if (event.is_mouse()) {
        //logger.log(Verbosity::INFO, "PCITopoUIComp -> mouse event: shift {} meta {} ctrl {}",
        //            event.mouse().shift, event.mouse().meta, event.mouse().control);

        if (event.mouse().button == Mouse::WheelDown) {
            if (event.mouse().shift)
                area->shift(UiElemShiftDir::RIGHT, box_);
            else
                area->shift(UiElemShiftDir::DOWN, box_);
        }

        if (event.mouse().button == Mouse::WheelUp) {
            if (event.mouse().shift)
                area->shift(UiElemShiftDir::LEFT, box_);
            else
                area->shift(UiElemShiftDir::UP, box_);
        }

        if (event.mouse().button == Mouse::Left)
            block_map_.SelectDeviceByPos(event.mouse().x + area->off_x_,
                                         event.mouse().y + area->off_y_,
                                         canvas_);

        return true;
    }

    if (event.is_character()) {
        //logger.log(Verbosity::INFO, "PCITopoUIComp -> char event");

        switch (event.character()[0]) {
        // scrolling
        case 'j':
            area->shift(UiElemShiftDir::DOWN, box_);
            break;
        case 'k':
            area->shift(UiElemShiftDir::UP, box_);
            break;
        case 'h':
            area->shift(UiElemShiftDir::LEFT, box_);
            break;
        case 'l':
            area->shift(UiElemShiftDir::RIGHT, box_);
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

    // special events handlers

    // scroll across the canvas with arrows
    if (event == Event::ArrowDown) {
        area->shift(UiElemShiftDir::DOWN, box_);
        return true;
    }

    if (event == Event::ArrowUp) {
        area->shift(UiElemShiftDir::UP, box_);
        return true;
    }

    if (event == Event::ArrowLeft) {
        area->shift(UiElemShiftDir::LEFT, box_);
        return true;
    }

    // FIXME: with this mapping it's no longer possible to switch to right panes
    if (event == Event::ArrowRight) {
        area->shift(UiElemShiftDir::RIGHT, box_);
        return true;
    }

    // select next/prev device via Ctrl + 'Up'/'Down'
    if (event == Event::ArrowUpCtrl) {
        block_map_.SelectNextPrevDevice(canvas_, false);
        return true;
    }

    if (event == Event::ArrowDownCtrl) {
        block_map_.SelectNextPrevDevice(canvas_, true);
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
        logger.log(Verbosity::WARN, "Failed to add {} device to block tracking map", dev->dev_id_str_);

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
    logger.log(Verbosity::INFO, "Estimated canvas size: {} x {}", x_size, y_size);

    return {x_size, y_size};
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
        selected_ -= 3;
    }

    if ((event == Event::ArrowDown || event == Event::Character('j') ||
        (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
        selected_ += 3;
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

Element PCIRegsComponent::Render()
{
    auto selected_device = topology_component_->GetSelectedDev();
    if (cur_dev_ == selected_device) {
        return ComponentBase::Render();
    } else {
        cur_dev_ = selected_device;
        DetachAllChildren();
        vis_state_.clear();

        // FIXME: Internal reallocation would break element visibility state tracking,
        // so reserve some space in advance
        vis_state_.reserve(interactive_elem_max_);

        CreateComponent();

        Add(split_comp_);

        return ComponentBase::Render();
    }

}

std::string PCIRegsComponent::PrintCurDevInfo() noexcept
{
    auto selected_device = topology_component_->GetSelectedDev();

    return selected_device->dev_id_str_;
}

// Create type0/type1 configuration space header
void PCIRegsComponent::AddCompatHeaderRegs()
{
    Components upper_comps, lower_comps;
    std::tie(upper_comps, lower_comps) = GetCompatHeaderRegsComponents(cur_dev_.get(), vis_state_);

    for (const auto &el : upper_comps)
        upper_split_comp_->Add(el);

    for (const auto &el : lower_comps)
        lower_split_comp_->Add(el);

    logger.log(Verbosity::INFO, "{} -> vis_state size {}", cur_dev_->dev_id_str_, vis_state_.size());
}

void PCIRegsComponent::AddCapabilities()
{
    bool compat_delim_present {false}, ext_delim_present {false};
    Components upper_comps, lower_comps;

    for (const auto &cap : cur_dev_->caps_) {
        auto type = std::get<0>(cap);
        auto id   = std::get<1>(cap);

        if (type == pci::CapType::compat) {
            if (!compat_delim_present) {
                upper_split_comp_->Add(CapsDelimComp(pci::CapType::compat,
                                                     cur_dev_->compat_caps_num_));
                compat_delim_present = true;
            }
            CompatCapID cap_id {id};
            std::tie(upper_comps, lower_comps) = GetCompatCapComponents(cur_dev_.get(), cap_id,
                                                                        cap, vis_state_);
        } else {
            if (!ext_delim_present) {
                upper_split_comp_->Add(CapsDelimComp(pci::CapType::extended,
                                                     cur_dev_->extended_caps_num_));
                ext_delim_present = true;
            }
            ExtCapID cap_id {id};
            std::tie(upper_comps, lower_comps) = GetExtendedCapComponents(cur_dev_.get(), cap_id,
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

    auto upper_comp_hoverable = MakeBorderedHoverComp(upper_comp_renderer);
    auto lower_comp_hoverable = MakeBorderedHoverComp(lower_comp);

    split_comp_ = ResizableSplit({
            .main = upper_comp_hoverable,
            .back = lower_comp_hoverable,
            .direction = Direction::Up,
            .main_size = &split_off_,
            .separator_func = [] { return separatorHeavy(); }
    });

    auto cur_hor_split_off = &split_off_;
    split_comp_ |= CatchEvent([=](Event ev) {
        if (ev == Event::F2)
                upper_comp_hoverable->TakeFocus();
        else if (ev == Event::F3)
                lower_comp_hoverable->TakeFocus();

        if (ev == Event::AltJ)
            SeparatorShift(UiElemShiftDir::DOWN, cur_hor_split_off);
        if (ev == Event::AltK)
            SeparatorShift(UiElemShiftDir::UP, cur_hor_split_off);

        return false;
    });

}

Element BorderedHoverComp::Render()
{
    const bool is_focused = Focused();
    const bool is_active = Active();

    auto focus_mgmt = is_focused ? ftxui::focus :
                      is_active ? ftxui::select :
                      ftxui::nothing;

    Element base_rendered_elem = ComponentBase::Render();
    base_rendered_elem |= focus_mgmt;

    if (is_focused)
        base_rendered_elem |= borderStyled(BorderStyle::HEAVY, Color::Green);
    else if (hovered_)
        base_rendered_elem |= borderStyled(Color::Palette16::White);

    return base_rendered_elem | reflect(box_);
}

bool BorderedHoverComp::OnEvent(Event event)
{
    if (event.is_mouse())
    {
        const bool hover = box_.Contain(event.mouse().x, event.mouse().y) &&
                           CaptureMouse(event);
        hovered_ = hover;
    }

    return ComponentBase::OnEvent(event);
}

static Element GetVersion()
{
    return text(fmt::format(" ver: {} {}", pciex_current_version,
                                           pciex_current_hash));
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

  elems.push_back(GetVersion());

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
    R"(  resize pane(s) - drag the border using the mouse            )",
    R"(                   or                                         )",
    R"(                   Alt + [h, j, k, l]                         )",
    R"(  TAB/h/k/left click - move focus to specific pane            )",
    R"(                                                              )",
    R"( Pane navigation:                                             )",
    R"(  Fx keys - move focus to specific pane:                      )",
    R"(     F1 - device tree pane                                    )",
    R"(     F2 - device regs/caps pane                               )",
    R"(     F3 - reg/cap detailed info pane                          )",
    R"(  [h, j, k, l] or arrows - scroll left, down, up, right       )",
    R"(  ------------                                                )",
    R"(   ctrl + Up/Down                                             )",
    R"(  shift + j/k - select next/previous device in the hierarchy  )",
    R"(                can also be selected with mouse               )",
    R"(                (device tree pane only)                       )",
    R"(  mouse wheel up/down         - vertical scroll               )",
    R"(  shift + mouse wheel up/down - horizontal scroll             )",
    R"(                                (device tree pane only)       )",
    R"(  left click / enter          - show/hide detailed info       )",
    R"(                                (device regs/caps pane only)  )",
    R"( Other hotkeys:                                               )",
    R"(      c/v - device tree pane compact/verbose                  )",
    R"(            drawing mode switch                               )",
    R"(        ? - help open                                         )",
    R"(  ?/Esc/q - help close                                        )",
    R"(                                                              )"
  };

  Elements elems;
  for (const auto &el : lvt)
    elems.push_back(text(el));

  return vbox(std::move(elems));
}

Component GetHelpScreenComp()
{
    auto help_comp = Renderer([&] {
        return vbox({
          GetLogo() | center,
          separatorEmpty(),
          GetHelpElem()
        });
    });

    // XXX: Seems like @Modal component rendering doesn't
    // clear background color explicitly, so it needs to be set here
    auto scrollable_help_comp = MakeScrollableComp(help_comp);
    return Renderer(scrollable_help_comp, [=] {
        return vbox({
            scrollable_help_comp->Render(),
        }) | borderRounded | bgcolor(Color::Grey15);
    });
}

void SeparatorShift(UiElemShiftDir direction, int *cur_sep_pos)
{
    auto cur_term_dim = Terminal::Size();

    switch (direction) {
    case UiElemShiftDir::UP:
    case UiElemShiftDir::LEFT:
        if (*cur_sep_pos > 10)
            *cur_sep_pos -= 5;
        break;
    case UiElemShiftDir::DOWN:
        if (*cur_sep_pos + 10 < cur_term_dim.dimy)
            *cur_sep_pos += 5;
        break;
    case UiElemShiftDir::RIGHT:
        if (*cur_sep_pos + 10 < cur_term_dim.dimx)
            *cur_sep_pos += 5;
        break;
    }
}

ScreenCompCtx::ScreenCompCtx(const pci::PCITopologyCtx &topo_ctx) :
      topo_ctx_(topo_ctx),
      topo_canvas_(nullptr),
      topo_canvas_comp_(nullptr),
      main_comp_split_(nullptr),
      pci_regs_comp_(nullptr),
      vert_split_off_(60),
      show_help_(false)
{}

Component
ScreenCompCtx::Create()
{
    auto [width, height] = GetCanvasSizeEstimate(topo_ctx_, ElemReprMode::Compact);
    topo_canvas_ = MakeTopologyComp(width, height, topo_ctx_);
    topo_canvas_comp_ = MakeBorderedHoverComp(topo_canvas_);

    pci_regs_comp_ = std::make_shared<PCIRegsComponent>(topo_canvas_);

    auto main_comp_split_ = ResizableSplit({
        .main = topo_canvas_comp_,
        .back = pci_regs_comp_,
        .direction = Direction::Left,
        .main_size = &vert_split_off_,
        .separator_func = [] { return separatorHeavy(); }
    });

    main_comp_split_ |= CatchEvent([&](Event ev) {
        if (ev.is_character()) {
            if (ev.character()[0] == '?')
                show_help_ = show_help_ ? false : true;
        }

        // handle pane selection
        if (ev == Event::F1)
            topo_canvas_comp_->TakeFocus();
        else if (ev == Event::F2 || ev == Event::F3)
            pci_regs_comp_->TakeFocus();

        // handle pane resize
        if (ev == Event::AltH)
            SeparatorShift(UiElemShiftDir::LEFT, &vert_split_off_);
        if (ev == Event::AltL)
            SeparatorShift(UiElemShiftDir::RIGHT, &vert_split_off_);
        if (ev == Event::AltJ || ev == Event::AltK)
            pci_regs_comp_->TakeFocus();

        return false;
    });

    auto help_component = GetHelpScreenComp();
    help_component |= CatchEvent([&](Event ev) {
        if (ev == Event::Escape ||
            (ev.is_character() &&
             (ev.character()[0] == '?' || ev.character()[0] == 'q')))
        {
                show_help_ = show_help_ ? false : true;
        }

        return false;
    });

    main_comp_split_ |= Modal(help_component, &show_help_);
    return main_comp_split_;
}

} //namespace ui
