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

CanvasElemPCIDev::CanvasElemPCIDev(std::shared_ptr<pci::PciDevBase> dev,
                                   uint16_t x, uint16_t y)
    : dev_(dev), selected_(false)
{
    size_t max_hlen, max_vlen;

    // initialize text array
    auto bdf_id_str = fmt::format("{} | [{:04x}:{:04x}]", dev->dev_id_str_,
                                  dev->get_vendor_id(), dev->get_device_id());
    max_hlen = bdf_id_str.length();
    text_data_.push_back(std::move(bdf_id_str));

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

    if (!CaptureMouse(event))
        return false;

    if (!hovered_)
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
            block_map.SelectDevice(event.mouse().x + area->off_x_,
                                   event.mouse().y + area->off_y_,
                                   canvas_);

        return true;
    }

    if (event.is_character()) {
        //logger.info("PCITopoUIComp -> char event");

        switch (event.character()[0]) {
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
        default:
            break;
        }

        return true;
    }
    return false;
}

void PCITopoUIComp::AddTopologyElements(const pci::PCITopologyCtx &ctx)
{
    uint16_t x = 4, y = 4;
    for (const auto &bus : ctx.buses_) {
        if (bus.second.is_root_) {
            auto conn_pos = AddRootBus(bus.second, &x, &y);
            AddBusDevices(bus.second, ctx.buses_, conn_pos, x + child_elem_xoff, &y);
        }
    }

    max_width_ = (max_width_ + 4) / sym_width;
    canvas_.VisibleAreaSetMax(max_width_, canvas_.height() / 4);
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
    auto device = std::make_shared<CanvasElemPCIDev>(dev, x, *y);
    std::pair<PointDesc, PointDesc> conn_pos_pair {device->GetConnPosChild(),
                                                   device->GetConnPosParent()};
    *y += device->GetHeight();
    if (!block_map.Insert(device))
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

std::pair<uint16_t, uint16_t>
GetCanvasSizeEstimate(const pci::PCITopologyCtx &ctx) noexcept
{
    uint16_t x_size = 0, y_size = 0;
    uint16_t root_bus_elem_height = 3 * sym_height;
    uint16_t dev_elem_height = 5 * sym_height;


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

} //namespace ui
