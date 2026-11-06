#include "ui/docking/DockTab.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/SizeBox.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace cinder::ui {

namespace {

constexpr float EDGE = 0.25f;

const DockStyle& look() { return Application::get().theme().get<DockStyle>("Dock"); }

}

TabDragDrop::TabDragDrop(std::string tab, std::string label) : tab_(std::move(tab)) {
    decorator_ = make<Border>()
            .brush([] { return look().tabActive; })
            .padding(Margin(12.0f, 5.0f))
            [make<Label>().text(std::move(label))];
}

DockTab::DockTab(TabManager& manager, TabStack& stack, std::string tab)
    : manager_(&manager), stack_(&stack), tab_(std::move(tab)) {
    setToolTipText(manager_->labelOf(tab_));
}

bool DockTab::closable() const { return manager_->isClosable(tab_); }

Rect DockTab::closeRect(const Geometry& geometry) const {
    const DockStyle& style = look();
    const float size = style.closeSize + 6.0f;
    return geometry.localRect({geometry.size.x - style.tabPadding.right - size, (geometry.size.y - size) * 0.5f},
                              {size, size});
}

Reply DockTab::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (closable() && closeRect(geometry).contains(event.position)) return Reply::handled();
    stack_->activate(tab_);
    return Reply::handled().detectDrag(shared_from_this(), cinder::platform::buttons::LEFT);
}

Reply DockTab::onMouseUp(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (closable() && closeRect(geometry).contains(event.position)) {
        const std::string tab = tab_;
        TabManager& manager = *manager_;
        manager.closeTab(tab);
        return Reply::handled();
    }
    return Reply::handled();
}

Reply DockTab::onDragDetected(const Geometry&, const PointerEvent&) {
    return Reply::handled().beginDragDrop(std::make_shared<TabDragDrop>(tab_, manager_->labelOf(tab_)));
}

glm::vec2 DockTab::computeDesiredSize(float) const {
    Application& app = Application::get();
    const DockStyle& style = look();
    label_.shape(app.fonts(), manager_->labelOf(tab_), app.theme().get<LabelStyle>("Label").font);
    float width = label_.size().x + style.tabPadding.total().x;
    if (closable()) width += style.closeSize + 10.0f;
    return {width, style.barHeight};
}

int DockTab::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                     const PaintStyle& style, bool) const {
    Application& app = Application::get();
    const DockStyle& dock = look();
    const bool active = stack_->active() == tab_;
    const Brush& brush = active ? dock.tabActive : (isHovered() ? dock.tabHovered : dock.tab);
    brush.paint(list, layer, geometry.rect(), style, geometry.scale);

    label_.shape(app.fonts(), manager_->labelOf(tab_), app.theme().get<LabelStyle>("Label").font);
    const Color color = style.apply(active ? dock.labelActive : dock.label);
    const float top = std::round((geometry.size.y - label_.lineHeight()) * 0.5f);
    label_.paint(list, layer + 1, geometry.absolute({dock.tabPadding.left, top}), geometry.scale, color, app.atlas());

    if (active) {
        list.box(layer + 1, geometry.localRect({0.0f, geometry.size.y - 2.0f}, {geometry.size.x, 2.0f}),
                 BoxStyle{style.apply(app.theme().color("Color.Primary"))});
    }
    if (closable()) {
        const Rect close = closeRect(geometry);
        const glm::vec2 centre = close.center();
        const float arm = dock.closeSize * 0.5f * geometry.scale;
        const std::array<glm::vec2, 2> first = {centre - glm::vec2(arm), centre + glm::vec2(arm)};
        const std::array<glm::vec2, 2> second = {centre + glm::vec2(arm, -arm), centre + glm::vec2(-arm, arm)};
        const Color mark = isHovered() ? style.apply(dock.labelActive) : color;
        list.lines(layer + 2, first, mark, 1.4f * geometry.scale);
        list.lines(layer + 2, second, mark, 1.4f * geometry.scale);
    }
    return layer + 2;
}

TabStack::TabStack(TabManager& manager, int id, std::vector<std::string> tabs, std::string active)
    : manager_(&manager), id_(id), tabs_(std::move(tabs)), active_(std::move(active)) {
    if (active_.empty() && !tabs_.empty()) active_ = tabs_.front();
}

void TabStack::activate(const std::string& tab) {
    if (active_ == tab) return;
    active_ = tab;
    manager_->setActive(id_, tab);
    if (content_) content_->setContent(manager_->contentFor(active_));
}

std::shared_ptr<DockTab> TabStack::tabWidget(const std::string& tab) const {
    for (const std::shared_ptr<DockTab>& widget : widgets_) {
        if (widget->tab() == tab) return widget;
    }
    return nullptr;
}

void TabStack::build() {
    auto bar = make<HorizontalBox>();
    widgets_.clear();
    for (const std::string& tab : tabs_) {
        auto widget = std::make_shared<DockTab>(*manager_, *this, tab);
        widgets_.push_back(widget);
        bar + HorizontalBox::slot().autoWidth()[widget];
    }
    bar + HorizontalBox::slot().fill(1.0f)[make<Spacer>()];

    content_ = make<Border>()
            .brush([] { return look().background; })
            .padding(Margin(0.0f))
            [active_.empty() ? nullptr : manager_->contentFor(active_)];
    childSlot_.widget = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()
                  [make<Border>().brush([] { return look().bar; }).padding(Margin(0.0f))[bar]]
            + VerticalBox::slot().fill(1.0f)[content_];
}

std::optional<DockSide> TabStack::sideAt(const Geometry& geometry, glm::vec2 point, const DragDropEvent& event) const {
    if (dynamic_cast<const TabDragDrop*>(event.operation.get()) == nullptr) return std::nullopt;
    const glm::vec2 local = geometry.local(point);
    if (local.y <= look().barHeight) return DockSide::Centre;
    const glm::vec2 unit = local / glm::max(geometry.size, glm::vec2(1.0f));
    const float left = unit.x;
    const float right = 1.0f - unit.x;
    const float top = unit.y;
    const float bottom = 1.0f - unit.y;
    const float nearest = std::min({left, right, top, bottom});
    if (nearest > EDGE) return DockSide::Centre;
    if (nearest == left) return DockSide::Left;
    if (nearest == right) return DockSide::Right;
    if (nearest == top) return DockSide::Top;
    return DockSide::Bottom;
}

void TabStack::onDragEnter(const Geometry&, const DragDropEvent&) {}

void TabStack::onDragLeave(const DragDropEvent&) { side_.reset(); }

Reply TabStack::onDragOver(const Geometry& geometry, const DragDropEvent& event) {
    side_ = sideAt(geometry, event.position, event);
    return side_ ? Reply::handled() : Reply::unhandled();
}

Reply TabStack::onDrop(const Geometry& geometry, const DragDropEvent& event) {
    const std::optional<DockSide> side = sideAt(geometry, event.position, event);
    side_.reset();
    const auto* dragged = dynamic_cast<const TabDragDrop*>(event.operation.get());
    if (!side || dragged == nullptr) return Reply::unhandled();
    const std::string tab = dragged->tab();
    manager_->dock(tab, id_, *side);
    return Reply::handled();
}

int TabStack::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                      const PaintStyle& style, bool enabled) const {
    const DockStyle& dock = look();
    dock.background.paint(list, layer, geometry.rect(), style, geometry.scale);
    int highest = paintChildren(args, geometry, list, layer + 1, style, enabled);
    if (!side_) return highest;

    const Rect rect = geometry.rect();
    Rect target = rect;
    switch (*side_) {
        case DockSide::Left: target.max.x = rect.min.x + rect.width() * 0.5f; break;
        case DockSide::Right: target.min.x = rect.min.x + rect.width() * 0.5f; break;
        case DockSide::Top: target.max.y = rect.min.y + rect.height() * 0.5f; break;
        case DockSide::Bottom: target.min.y = rect.min.y + rect.height() * 0.5f; break;
        case DockSide::Centre: break;
    }
    list.box(highest + 1, target,
             BoxStyle{style.apply(dock.dropFill), style.apply(dock.dropOutline), 2.0f * geometry.scale, glm::vec4(2.0f)});
    return highest + 1;
}

}
