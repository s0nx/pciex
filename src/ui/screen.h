// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023-2024 Petr Vyazovik <xen@f-m.fm>

#pragma once

#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>

#include "pci_dev.h"
#include "pci_topo.h"

namespace ui {

enum class UiElemShiftDir
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

    void shift(UiElemShiftDir dir, ftxui::Box &);
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

enum class ElemReprMode
{
    Compact,
    Verbose
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
    CanvasElemPCIDev(std::shared_ptr<pci::PciDevBase> dev, ElemReprMode repr_mode,
                     uint16_t x, uint16_t y);

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
    CanvasElemBus(const pci::PCIBus &bus, uint16_t x, uint16_t y);

    PointDesc GetConnPos() noexcept;

    void Draw(ScrollableCanvas &canvas) override;
};

struct CanvasElemMode : public CanvasElementBase
{
    bool        is_live_;
    std::string mode_text_;
    ShapeDesc   points_;

    CanvasElemMode() = delete;
    CanvasElemMode(bool is_live, uint16_t x, uint16_t y);

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

        for (int y = 0; y < y_max; ++y) {
          for (int x = 0; x < x_max; ++x) {
            screen.PixelAt(box_.x_min + x, box_.y_min + y) =
                           c.GetPixel(x + c.x_off(), y + c.y_off());
          }
        }
    }
    virtual const ScrollableCanvas &canvas() = 0;
};

// mouse tracking stuff
typedef std::pair<uint16_t, uint16_t> BlockSnglDimDesc;

inline auto bd_comp = [](const BlockSnglDimDesc &bd1, const BlockSnglDimDesc &bd2) {
                        return bd1.first < bd2.first; };
typedef std::map<BlockSnglDimDesc, std::shared_ptr<CanvasElemPCIDev>,
                 decltype(bd_comp)> CanvasSnglDimBlockMap;
typedef CanvasSnglDimBlockMap::iterator BlockMapIter;

struct CanvasDevBlockMap
{
    CanvasSnglDimBlockMap             blocks_y_dim_;
    std::shared_ptr<CanvasElemPCIDev> selected_dev_;
    BlockMapIter                      selected_dev_iter_;

    CanvasDevBlockMap() : selected_dev_iter_(blocks_y_dim_.end()) {}

    bool Insert(std::shared_ptr<CanvasElemPCIDev> dev);
    void SelectDeviceByPos(const uint16_t mouse_x, const uint16_t mouse_y,
                           ScrollableCanvas &canvas);
    void SelectNextPrevDevice(ScrollableCanvas &canvas, bool select_next);
    void Reset();
};

constexpr uint16_t child_elem_xoff = 16;

class PCITopoUIComp : public ftxui::ComponentBase
{
public:
    PCITopoUIComp() = delete;
    PCITopoUIComp(int width, int height, const pci::PCITopologyCtx &ctx) :
        ftxui::ComponentBase(),
        topo_ctx_(ctx),
        current_drawing_mode_(ElemReprMode::Compact),
        block_map_(),
        canvas_(width, height)
    {
        AddTopologyElements();
    }

    ftxui::Element Render() override final;

    bool OnEvent(ftxui::Event event) override final;
    bool Focusable() const final { return true; }
    std::shared_ptr<pci::PciDevBase> GetSelectedDev() noexcept
    {
        if (block_map_.selected_dev_iter_ == block_map_.blocks_y_dim_.end())
            return nullptr;
        return block_map_.selected_dev_iter_->second->dev_;
    }

private:
    std::vector<std::shared_ptr<CanvasElementBase>> canvas_elems_;
    int                                             max_width_ {0};
    const pci::PCITopologyCtx                       &topo_ctx_;
    ElemReprMode                                    current_drawing_mode_;
    CanvasDevBlockMap                               block_map_;
    bool                                            hovered_ { false };
    ftxui::Box                                      box_;
    ScrollableCanvas                                canvas_;

    void DrawElements() noexcept;
    void AddTopologyElements();
    std::pair<PointDesc, PointDesc>
    AddDevice(std::shared_ptr<pci::PciDevBase> dev, uint16_t x, uint16_t *y);
    PointDesc
    AddRootBus(const pci::PCIBus &bus, uint16_t *x, uint16_t *y);
    void AddBusDevices(const pci::PCIBus &current_bus,
                       const std::map<uint16_t, pci::PCIBus> &bus_map,
                       PointDesc parent_conn_pos,
                       uint16_t x_off, uint16_t *y_off);
    void SwitchDrawingMode(ElemReprMode);
};

inline auto MakeTopologyComp(int width, int height, const pci::PCITopologyCtx &ctx)
{
    return ftxui::Make<PCITopoUIComp>(width, height, ctx);
}

std::pair<uint16_t, uint16_t>
GetCanvasSizeEstimate(const pci::PCITopologyCtx &ctx, ElemReprMode mode) noexcept;

// TODO: It seems not to be easy to add scrolling to the component made of a bunch
// of static DOM elements, e.g:
//
// auto vc = ftxui::Container::Vertical({});
// auto comp = ftxui::Renderer([] { return ftxui::text("test"); });
// vc->Add(comp)
// . . .
// auto vc_renderer = ftxui::Renderer(vc, [&] {
//    return vc->Render() | ftxui::vscroll_indicator |
//           ftxui::hscroll_indicator | ftxui::frame;
// });
//
// This component seems to solve this problem, but at the cost of losing interactivity.
// See: https://github.com/ArthurSonzogni/FTXUI/discussions/657
// https://github.com/ArthurSonzogni/git-tui/blob/master/src/scroller.cpp
class ScrollableComp : public ftxui::ComponentBase
{
public:
    ScrollableComp(ftxui::Component child) { Add(child); }

private:
    ftxui::Element Render() override final;
    bool OnEvent(ftxui::Event event) final;
    bool Focusable() const final { return true;  }

    int selected_ {0};
    int     size_ {0};
    ftxui::Box box_;
};


// Holds resizable split component representing currently selected device.
// ┌───────────────────────────┐
// │ device registers overview │
// ├───────────────────────────┤
// │  registers detailed info  │
// └───────────────────────────┘
struct PCIRegsComponent : ftxui::ComponentBase
{
    std::shared_ptr<PCITopoUIComp>   topology_component_;
    std::shared_ptr<pci::PciDevBase> cur_dev_;
    ftxui::Component                 upper_split_comp_;
    ftxui::Component                 lower_split_comp_;
    ftxui::Component                 split_comp_;
    std::vector<uint8_t>             vis_state_;
    int                              split_off_ {40};
    uint32_t                         interactive_elem_max_ {1024};

    ftxui::Element Render() override;
    bool Focusable() const final { return true; }

    PCIRegsComponent(std::shared_ptr<PCITopoUIComp> topo_comp)
        : ftxui::ComponentBase(), topology_component_(topo_comp) {}

    // XXX DEBUG
    std::string PrintCurDevInfo() noexcept;

    void CreateComponent();
    void FinalizeComponent();
    void AddCompatHeaderRegs();
    void AddCapabilities();

};

// Wrapper to draw border around component on hover/select.
// Looks fairly similar to @Hoverable class
class BorderedHoverComp : public ftxui::ComponentBase
{
public:
    BorderedHoverComp(ftxui::Component child) { Add(child);  }
private:
    ftxui::Element Render() override final;
    bool           OnEvent(ftxui::Event event) override;

    ftxui::Box box_;
    bool       hovered_;
};

inline ftxui::Component MakeBorderedHoverComp(ftxui::Component child)
{
    return ftxui::Make<BorderedHoverComp>(child);
}

ftxui::Component GetHelpScreenComp();

void SeparatorShift(UiElemShiftDir direction, int *cur_sep_pos);

class ScreenCompCtx
{
public:
  ScreenCompCtx(const pci::PCITopologyCtx &topo_ctx);

  // Create main screen components
  ftxui::Component
  Create();

private:
  const pci::PCITopologyCtx         &topo_ctx_;
  std::shared_ptr<PCITopoUIComp>    topo_canvas_;
  ftxui::Component                  topo_canvas_comp_;
  ftxui::Component                  main_comp_split_;
  ftxui::Component                  pci_regs_comp_;

  int                               vert_split_off_;
  bool                              show_help_;
};

} // namespace ui
