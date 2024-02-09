// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <map>
#include <memory>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/screen/color.hpp>

#include "../pciex.h"

namespace ui {

enum class CnvShiftDir
{
    UP,
    DOWN,
    RIGHT,
    LEFT
};

struct CanvasVisibleArea
{
    int x_max_ {0};
    int y_max_ {0};
    int off_x_ {0};
    int off_y_ {0};

    void shift(CnvShiftDir dir, ftxui::Box &);
};

const auto NoStyle = [](ftxui::Pixel &){};
//                    X         Y        len      height
typedef std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> ShapeDesc;

// XXX: values below were obtained experimentally
// Length of the box to fully fit text
constexpr uint16_t tbox_additional_len = 5 * 2 + 1;
// Height of the box to fit signle line of text
constexpr uint16_t tbox_height = 6;

// X,Y text offsets within the box
constexpr uint16_t tbox_txt_xoff = 4;
constexpr uint16_t tbox_txt_yoff = 4;

class ScrollableCanvas : public ftxui::Canvas
{
public:
    ScrollableCanvas() = delete;
    ScrollableCanvas(int width, int height)
        : ftxui::Canvas(width, height) {}

    // draw a simple box
    void DrawBoxLine(ShapeDesc desc, const ftxui::Color &color);
    void DrawBoxLine(ShapeDesc desc,
                     const ftxui::Canvas::Stylizer &style = NoStyle);

    // draw a box with the text inside it
    // FIXME: text is expected to be single line for now
    // @pos {x, y}: x is expected to be mulitple of 2,
    //              y is expected to be multiple of 4
    void DrawTextBox(std::pair<uint16_t, uint16_t> pos, std::string text,
                     const ftxui::Canvas::Stylizer &box_style = NoStyle,
                     const ftxui::Canvas::Stylizer &text_style = NoStyle);

    int x_off() const { return area_.off_x_; }
    int y_off() const { return area_.off_y_; }

    // Set max [x, y] values in pixels
    void VisibleAreaSetMax(const uint16_t x_max, const uint16_t y_max) noexcept
    {
        area_.x_max_ = x_max;
        area_.y_max_ = y_max;
    }
    CanvasVisibleArea *GetVisibleAreaDesc() { return &area_; }

private:
    CanvasVisibleArea area_;
};

struct CanvasElementBase
{
    virtual void Draw(ScrollableCanvas &canvas) = 0;
};

typedef std::pair<uint16_t, uint16_t> PointDesc;
constexpr uint16_t sym_height = 4;
constexpr uint16_t sym_width = 2;

// Visually connects parent to child elements on canvas
struct CanvasElemConnector : public CanvasElementBase
{
    std::vector<std::pair<PointDesc, PointDesc>> lines_;

    void AddLine(std::pair<PointDesc, PointDesc> line_desc);
    void Draw(ScrollableCanvas &canvas) override;
};

// PCI device element
struct CanvasElemPCIDev : public CanvasElementBase
{
    std::shared_ptr<pci::PciDevBase> dev_;
    std::vector<std::string>         text_data_;
    ShapeDesc                        points_;
    bool                             selected_ {false};

    CanvasElemPCIDev() = delete;
    CanvasElemPCIDev(std::shared_ptr<pci::PciDevBase> dev, uint16_t x, uint16_t y);

    uint16_t GetHeight() noexcept;
    PointDesc GetConnPosParent() noexcept;
    PointDesc GetConnPosChild() noexcept;
    ShapeDesc GetShapeDesc() noexcept;

    void Draw(ScrollableCanvas &canvas) override;
};

struct CanvasElemBus : public CanvasElementBase
{
    std::string bus_id_str_;
    ShapeDesc   points_;

    CanvasElemBus() = delete;
    CanvasElemBus(const pci::PCIBus &bus, uint16_t x, uint16_t y)
        : bus_id_str_(fmt::format("[ {:04x}:{:02x} ]", bus.dom_, bus.bus_nr_))
    {
        auto hlen = bus_id_str_.length() * sym_width + 2 * 2;
        auto vlen = sym_height + 2;
        points_ = { x + 1, y + 3, hlen, vlen };
    }

    PointDesc GetConnPos() noexcept;

    void Draw(ScrollableCanvas &canvas) override;
};

class CanvasScrollNodeBase : public ftxui::Node
{
public:
    CanvasScrollNodeBase() = default;
    void Render(ftxui::Screen &screen) override
    {
        const auto &c = canvas();

        const int y_max = std::min(c.height() / 4, box_.y_max - box_.y_min + 1);
        const int x_max = std::min(c.width() / 2, box_.x_max - box_.x_min + 1);

        //logger.info("CANVAS RENDER: BOX -> y_max {} y_min {} x_max {} x_min {}",
        //            box_.y_max, box_.y_min, box_.x_max, box_.x_min);
        //logger.info("Coff: X {} - Y {}", c.x_off(), c.y_off());
        //logger.info("CR: screen.stencil -> y_max {} y_min {} x_max {} x_min {}",
        //            screen.stencil.y_max, screen.stencil.y_min,
        //            screen.stencil.x_max, screen.stencil.x_min);

        for (int y = 0; y < y_max; ++y) {
          for (int x = 0; x < x_max; ++x) {
            screen.PixelAt(box_.x_min + x, box_.y_min + y) =
                           c.GetPixel(x + c.x_off(), y + c.y_off());
          }
        }
    }
    virtual const ScrollableCanvas &canvas() = 0;
};

inline ftxui::Element scrollable_canvas(ftxui::ConstRef<ScrollableCanvas> canvas)
{
    class Impl : public CanvasScrollNodeBase
    {
    public:
        explicit Impl(ftxui::ConstRef<ScrollableCanvas> canvas) : canvas_(std::move(canvas))
        {
            requirement_.min_x = (canvas->width() + 1) / 2;
            requirement_.min_y = (canvas->height() + 3) / 4;
        }

        const ScrollableCanvas &canvas() final { return *canvas_; }
        ftxui::ConstRef<ScrollableCanvas> canvas_;
    };

    return std::make_shared<Impl>(canvas);
}

// mouse tracking stuff
typedef std::pair<uint16_t, uint16_t> BlockSnglDimDesc;

inline auto bd_comp = [](const BlockSnglDimDesc &bd1, const BlockSnglDimDesc &bd2) {
                        return bd1.first < bd2.first; };
struct CanvasDevBlockMap
{
    std::map<BlockSnglDimDesc, std::shared_ptr<CanvasElemPCIDev>,
                    decltype(bd_comp)> blocks_y_dim_;
    std::shared_ptr<CanvasElemPCIDev> selected_dev_;

    bool Insert(std::shared_ptr<CanvasElemPCIDev> dev);
    void SelectDevice(const uint16_t mouse_x, const uint16_t mouse_y, ScrollableCanvas &canvas);
};

constexpr uint16_t child_elem_xoff = 16;

class PCITopoUIComp : public ftxui::ComponentBase
{
public:
    PCITopoUIComp() = delete;
    PCITopoUIComp(int width, int height) :
        ftxui::ComponentBase(), canvas_(width, height) {}

    ftxui::Element Render() override;

    bool OnEvent(ftxui::Event event) override final;
    bool Focusable() const final { return true; }
    void AddTopologyElements(const pci::PCITopologyCtx &ctx);

private:
    std::vector<std::shared_ptr<CanvasElementBase>> canvas_elems_;
    void DrawElements() noexcept;
    int max_width_ {0};

    std::pair<PointDesc, PointDesc>
    AddDevice(std::shared_ptr<pci::PciDevBase> dev, uint16_t x, uint16_t *y);
    PointDesc
    AddRootBus(const pci::PCIBus &bus, uint16_t *x, uint16_t *y);

    void AddBusDevices(const pci::PCIBus &current_bus,
                       const std::map<uint16_t, pci::PCIBus> &bus_map,
                       PointDesc parent_conn_pos,
                       uint16_t x_off, uint16_t *y_off);

    CanvasDevBlockMap block_map;
    bool hovered_ { false };
    ftxui::Box box_;
    ScrollableCanvas canvas_;
};

inline auto MakeTopologyComp(int width, int height)
{
    return ftxui::Make<PCITopoUIComp>(width, height);
}

std::pair<uint16_t, uint16_t>
GetCanvasSizeEstimate(const pci::PCITopologyCtx &ctx) noexcept;

} // namespace ui
