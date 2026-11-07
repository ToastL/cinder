#include "ui/widgets/MenuBar.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Menu.hpp"

#include <cmath>
#include <utility>

namespace cinder::ui {

void MenuBarItem::onMouseEnter(const Geometry&, const PointerEvent&) {
    const int open = bar_->openIndex();
    if (open >= 0 && open != index_) bar_->open(index_);
}

Reply MenuBarItem::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (bar_->openIndex() == index_) bar_->close();
    else bar_->open(index_);
    return Reply::handled();
}

glm::vec2 MenuBarItem::computeDesiredSize(float) const {
    Application& app = Application::get();
    const MenuBarStyle& look = app.theme().get<MenuBarStyle>(bar_->style());
    title_.shape(app.fonts(), bar_->title(index_), look.font);
    return glm::vec2(title_.size().x, title_.lineHeight()) + look.itemPadding.total();
}

int MenuBarItem::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                         const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    const MenuBarStyle& look = app.theme().get<MenuBarStyle>(bar_->style());
    const bool open = bar_->openIndex() == index_;
    const Brush& brush = open ? look.itemOpen : (isHovered() ? look.itemHovered : look.item);
    brush.paint(list, layer, geometry.rect(), style, geometry.scale);
    title_.shape(app.fonts(), bar_->title(index_), look.font);
    Color color = style.apply(look.color);
    if (!enabled) color.a *= 0.4f;
    const float top = std::round((geometry.size.y - title_.lineHeight()) * 0.5f);
    title_.paint(list, layer + 1, geometry.absolute({look.itemPadding.left, top}), geometry.scale, color, app.atlas());
    return layer + 1;
}

void MenuBar::construct(const Args& args) {
    menus_ = args.menus_;
    style_ = args.style_;
    auto row = make<HorizontalBox>();
    for (std::size_t i = 0; i < menus_.size(); ++i) {
        auto item = std::make_shared<MenuBarItem>(*this, static_cast<int>(i));
        items_.push_back(item);
        row + HorizontalBox::slot().autoWidth()[item];
    }
    childSlot_.widget = row;
}

int MenuBar::openIndex() const {
    if (open_ < 0 || !popup_.isOpen()) return -1;
    return open_;
}

void MenuBar::open(int index) {
    Application* app = Application::current();
    if (app == nullptr || index < 0 || index >= menuCount() || openIndex() == index) return;
    app->dismissAllPopups();
    MenuBuilder builder;
    if (menus_[static_cast<std::size_t>(index)].fill) menus_[static_cast<std::size_t>(index)].fill(builder);
    std::shared_ptr<Menu> menu = builder.build();

    PopupOptions options;
    options.placement = Placement::Below;
    options.owner = weak_from_this();
    const std::weak_ptr<Widget> self = weak_from_this();
    options.onDismissed = [self] {
        if (std::shared_ptr<Widget> alive = self.lock()) static_cast<MenuBar&>(*alive).open_ = -1;
    };
    const Rect anchor = app->grid().geometryOf(items_[static_cast<std::size_t>(index)].get()).value_or(Geometry{}).rect();
    popup_.open(*app, std::move(menu), anchor, std::move(options));
    open_ = index;
}

void MenuBar::close() { popup_.close(); }

Reply MenuBar::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    close();
    return Reply::handled();
}

int MenuBar::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                     const PaintStyle& style, bool enabled) const {
    const MenuBarStyle& look = Application::get().theme().get<MenuBarStyle>(style_);
    look.background.paint(list, layer, geometry.rect(), style, geometry.scale);
    return paintChildren(args, geometry, list, layer + 1, style, enabled);
}

}
