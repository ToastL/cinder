#include "ui/widgets/Menu.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/ExpandableArea.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/ScrollBox.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace cinder::ui {

namespace keys = cinder::platform::keys;

namespace {

class MenuSeparator : public LeafWidget {
public:
    explicit MenuSeparator(std::string style) : style_(std::move(style)) {}

protected:
    glm::vec2 computeDesiredSize(float) const override { return {0.0f, 9.0f}; }

    int onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer, const PaintStyle& style,
                bool) const override {
        const MenuStyle& look = Application::get().theme().get<MenuStyle>(style_);
        const float y = std::round(geometry.size.y * 0.5f);
        list.box(layer, geometry.localRect({4.0f, y}, {std::max(0.0f, geometry.size.x - 8.0f), 1.0f}),
                 BoxStyle{style.apply(look.separator)});
        return layer;
    }

private:
    std::string style_;
};

}

MenuBuilder& MenuBuilder::add(MenuItem item) {
    items_.push_back(std::move(item));
    return *this;
}

MenuBuilder& MenuBuilder::entry(Attribute<std::string> label, std::function<void()> action, std::string shortcut) {
    MenuItem item;
    item.label = std::move(label);
    item.action = std::move(action);
    item.shortcut = std::move(shortcut);
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::check(Attribute<std::string> label, std::function<void()> action,
                                std::function<bool()> checked, std::string shortcut) {
    MenuItem item;
    item.label = std::move(label);
    item.action = std::move(action);
    item.isChecked = std::move(checked);
    item.checkable = true;
    item.shortcut = std::move(shortcut);
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::command(std::shared_ptr<const CommandList> commands, const Command& command) {
    MenuItem item;
    item.label = command.label;
    item.shortcut = command.shortcut.label();
    item.toolTip = command.description;
    const Command* bound = &command;
    item.action = [commands, bound] { commands->execute(*bound); };
    item.canExecute = [commands, bound] {
        const CommandAction* action = commands->action(*bound);
        return action != nullptr && action->can();
    };
    const CommandAction* action = commands->action(command);
    if (action != nullptr && action->isChecked) {
        item.checkable = true;
        item.isChecked = [commands, bound] {
            const CommandAction* found = commands->action(*bound);
            return found != nullptr && found->checked();
        };
    }
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::subMenu(Attribute<std::string> label, std::function<void(MenuBuilder&)> fill) {
    MenuItem item;
    item.kind = MenuItem::Kind::SubMenu;
    item.label = std::move(label);
    item.subMenu = std::move(fill);
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::separator() {
    MenuItem item;
    item.kind = MenuItem::Kind::Separator;
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::heading(std::string text) {
    MenuItem item;
    item.kind = MenuItem::Kind::Heading;
    item.label = std::move(text);
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::widget(std::shared_ptr<Widget> content) {
    MenuItem item;
    item.kind = MenuItem::Kind::Custom;
    item.widget = std::move(content);
    return add(std::move(item));
}

MenuBuilder& MenuBuilder::enabledIf(std::function<bool()> canExecute) {
    if (!items_.empty()) items_.back().canExecute = std::move(canExecute);
    return *this;
}

MenuBuilder& MenuBuilder::toolTip(std::string text) {
    if (!items_.empty()) items_.back().toolTip = std::move(text);
    return *this;
}

std::shared_ptr<Menu> MenuBuilder::build(std::string style) const {
    return make<Menu>().items(items_).style(std::move(style));
}

void MenuRow::onMouseEnter(const Geometry&, const PointerEvent&) {
    if (menu_->rowEnabled(index_)) menu_->highlight(index_, true);
}

Reply MenuRow::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (menu_->item(index_).kind == MenuItem::Kind::SubMenu && menu_->subMenuRow() != index_) {
        menu_->openSubMenu(index_, false);
    }
    return Reply::handled();
}

Reply MenuRow::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (menu_->item(index_).kind != MenuItem::Kind::SubMenu) menu_->activate(index_);
    return Reply::handled();
}

glm::vec2 MenuRow::computeDesiredSize(float) const {
    Application& app = Application::get();
    const MenuStyle& look = app.theme().get<MenuStyle>(menu_->style());
    const MenuItem& item = menu_->item(index_);
    label_.shape(app.fonts(), item.label.get(), look.font);
    glm::vec2 size(look.checkWidth + label_.size().x, label_.lineHeight());
    if (!item.shortcut.empty()) {
        shortcut_.shape(app.fonts(), item.shortcut, look.font);
        size.x += look.shortcutGap + shortcut_.size().x;
    }
    if (item.kind == MenuItem::Kind::SubMenu) size.x += look.shortcutGap;
    return size + look.entryPadding.total();
}

int MenuRow::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                     const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    const MenuStyle& look = app.theme().get<MenuStyle>(menu_->style());
    const MenuItem& item = menu_->item(index_);
    painted_ = geometry.rect();
    const bool lit = enabled && (menu_->highlighted() == index_ || menu_->subMenuRow() == index_);
    if (lit) look.highlight.paint(list, layer, geometry.rect(), style, geometry.scale);

    Color color = style.apply(enabled ? look.color : look.dim);
    if (!enabled) color.a *= 0.6f;
    const Color dim = style.apply(look.dim);
    const float height = geometry.size.y;

    if (item.checkable && item.isChecked && item.isChecked()) {
        const float size = std::min(look.checkWidth, height) * 0.6f;
        const glm::vec2 corner(look.entryPadding.left + (look.checkWidth - size) * 0.5f - 2.0f, (height - size) * 0.5f);
        const std::array<glm::vec2, 3> mark = {geometry.absolute(corner + glm::vec2(0.1f, 0.55f) * size),
                                               geometry.absolute(corner + glm::vec2(0.4f, 0.85f) * size),
                                               geometry.absolute(corner + glm::vec2(0.95f, 0.2f) * size)};
        list.lines(layer + 1, mark, color, 1.6f * geometry.scale);
    }

    label_.shape(app.fonts(), item.label.get(), look.font);
    const float top = std::round((height - label_.lineHeight()) * 0.5f);
    label_.paint(list, layer + 1, geometry.absolute({look.entryPadding.left + look.checkWidth, top}), geometry.scale,
                 color, app.atlas());

    float right = geometry.size.x - look.entryPadding.right;
    if (item.kind == MenuItem::Kind::SubMenu) {
        ExpanderArrow::paintArrow(list, layer + 1, geometry.absolute({right - 5.0f, height * 0.5f}), 9.0f * geometry.scale,
                                  false, color);
        right -= look.shortcutGap;
    }
    if (!item.shortcut.empty()) {
        shortcut_.shape(app.fonts(), item.shortcut, look.font);
        shortcut_.paint(list, layer + 1, geometry.absolute({right - shortcut_.size().x, top}), geometry.scale, dim,
                        app.atlas());
    }
    return layer + 1;
}

void Menu::construct(const Args& args) {
    items_ = args.items_;
    style_ = args.style_;
    auto rows = make<ScrollBox>();
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const MenuItem& item = items_[i];
        switch (item.kind) {
            case MenuItem::Kind::Entry:
            case MenuItem::Kind::SubMenu: {
                const int index = static_cast<int>(rows_.size());
                auto row = std::make_shared<MenuRow>(*this, index);
                row->setEnabled([this, index] { return rowEnabled(index); });
                if (!item.toolTip.empty()) row->setToolTipText(item.toolTip);
                rows_.push_back(row);
                rowItems_.push_back(static_cast<int>(i));
                rows + ScrollBox::slot()[row];
                break;
            }
            case MenuItem::Kind::Separator:
                rows + ScrollBox::slot()[std::make_shared<MenuSeparator>(style_)];
                break;
            case MenuItem::Kind::Heading:
                rows + ScrollBox::slot().padding(Margin(8.0f, 6.0f, 8.0f, 2.0f))
                               [make<Label>().text(item.label).textStyle("Menu.Heading")];
                break;
            case MenuItem::Kind::Custom:
                if (item.widget) rows + ScrollBox::slot().padding(Margin(4.0f, 2.0f))[item.widget];
                break;
        }
    }
    childSlot_.widget = rows;
}

bool Menu::rowEnabled(int row) const {
    if (row < 0 || row >= rowCount()) return false;
    const MenuItem& entry = item(row);
    return !entry.canExecute || entry.canExecute();
}

void Menu::highlight(int row, bool fromMouse) {
    fromMouse_ = fromMouse;
    if (row == highlighted_) return;
    highlighted_ = row;
    if (Application* app = Application::current()) highlightedAt_ = app->time();
}

int Menu::step(int from, int direction) const {
    const int count = rowCount();
    if (count == 0) return -1;
    int at = from < 0 ? (direction > 0 ? -1 : count) : from;
    for (int i = 0; i < count; ++i) {
        at = (at + direction + count) % count;
        if (rowEnabled(at)) return at;
    }
    return -1;
}

void Menu::activate(int row) {
    if (!rowEnabled(row)) return;
    const MenuItem& entry = item(row);
    if (entry.kind == MenuItem::Kind::SubMenu) {
        openSubMenu(row, true);
        return;
    }
    const std::function<void()> action = entry.action;
    if (Application* app = Application::current()) app->dismissAllPopups();
    if (action) action();
}

void Menu::openSubMenu(int row, bool focus) {
    Application* app = Application::current();
    if (app == nullptr || row < 0 || row >= rowCount() || !rowEnabled(row)) return;
    const MenuItem& entry = item(row);
    if (entry.kind != MenuItem::Kind::SubMenu || !entry.subMenu) return;
    app->dismissPopupsAbove(this);

    MenuBuilder builder;
    entry.subMenu(builder);
    std::shared_ptr<Menu> sub = builder.build(style_);
    sub->nested_ = true;

    const MenuStyle& look = app->theme().get<MenuStyle>(style_);
    Rect anchor = rows_[static_cast<std::size_t>(row)]->paintedRect();
    anchor.min.y -= look.padding.top;
    anchor.max.x += look.padding.right;

    PopupOptions options;
    options.placement = Placement::Right;
    options.focus = focus;
    const std::weak_ptr<Widget> self = weak_from_this();
    const Widget* raw = sub.get();
    options.onDismissed = [self, raw] {
        std::shared_ptr<Widget> alive = self.lock();
        if (!alive) return;
        auto& menu = static_cast<Menu&>(*alive);
        if (menu.sub_.get() != raw) return;
        menu.sub_.reset();
        menu.subRow_ = -1;
    };
    sub_ = sub;
    subRow_ = row;
    app->pushPopup(sub, anchor, std::move(options));
    if (focus) sub->highlight(sub->step(-1, 1), false);
}

void Menu::closeSubMenu() {
    if (!sub_) return;
    if (Application* app = Application::current()) app->dismissPopup(sub_.get());
}

Reply Menu::onKeyDown(const Geometry&, const KeyEvent& event) {
    switch (event.key) {
        case keys::UP:
            highlight(step(highlighted_, -1), false);
            return Reply::handled();
        case keys::DOWN:
            highlight(step(highlighted_, 1), false);
            return Reply::handled();
        case keys::ENTER:
        case keys::KEYPAD_ENTER:
        case keys::SPACE:
            if (highlighted_ >= 0) activate(highlighted_);
            return Reply::handled();
        case keys::RIGHT:
            if (highlighted_ >= 0 && item(highlighted_).kind == MenuItem::Kind::SubMenu) {
                openSubMenu(highlighted_, true);
                return Reply::handled();
            }
            return Reply::unhandled();
        case keys::LEFT:
            if (!nested_) return Reply::unhandled();
            if (Application* app = Application::current()) app->dismissPopup(this);
            return Reply::handled();
        default:
            return Reply::unhandled();
    }
}

Reply Menu::onMouseDown(const Geometry&, const PointerEvent&) { return Reply::handled(); }

void Menu::tick(const Geometry&, double time, float) {
    if (!fromMouse_ || highlighted_ == subRow_ || time - highlightedAt_ < SUBMENU_DELAY) return;
    if (highlighted_ >= 0 && item(highlighted_).kind == MenuItem::Kind::SubMenu && rowEnabled(highlighted_)) {
        openSubMenu(highlighted_, false);
    } else {
        closeSubMenu();
    }
}

glm::vec2 Menu::computeDesiredSize(float) const {
    const MenuStyle& look = Application::get().theme().get<MenuStyle>(style_);
    glm::vec2 size = (childSlot_.widget ? childSlot_.widget->desiredSize() : glm::vec2(0.0f)) + look.padding.total();
    size.x = std::max(size.x, look.minWidth);
    return size;
}

void Menu::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!childSlot_.widget) return;
    const MenuStyle& look = Application::get().theme().get<MenuStyle>(style_);
    out.push_back({childSlot_.widget,
                   geometry.child(look.padding.topLeft(), glm::max(glm::vec2(0.0f), geometry.size - look.padding.total()))});
}

int Menu::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                  const PaintStyle& style, bool enabled) const {
    const MenuStyle& look = Application::get().theme().get<MenuStyle>(style_);
    look.background.paint(list, layer, geometry.rect(), style, geometry.scale);
    return paintChildren(args, geometry, list, layer + 1, style, enabled);
}

void MenuAnchor::construct(const Args& args) {
    factory_ = args.onGetMenuContent_;
    placement_ = args.placement_;
    matchWidth_ = args.matchWidth_;
    onChanged_ = args.onMenuOpenChanged_;
    childSlot_.widget = args.content_;
}

bool MenuAnchor::isOpen() const { return menu_ && app_ != nullptr && app_->isPopupOpen(menu_.get()); }

void MenuAnchor::open() {
    Application* app = Application::current();
    if (app == nullptr || !factory_ || isOpen()) return;
    std::shared_ptr<Widget> content = factory_();
    if (!content) return;
    const std::optional<Geometry> geometry = app->grid().geometryOf(this);
    const Rect anchor = geometry ? geometry->rect() : Rect::fromSize(app->cursorPosition(), glm::vec2(0.0f));

    PopupOptions options;
    options.placement = placement_;
    options.minWidth = matchWidth_ ? anchor.width() : 0.0f;
    options.owner = weak_from_this();
    const std::weak_ptr<Widget> self = weak_from_this();
    const Widget* raw = content.get();
    options.onDismissed = [self, raw] {
        if (std::shared_ptr<Widget> alive = self.lock()) static_cast<MenuAnchor&>(*alive).dismissed(raw);
    };
    menu_ = content;
    app_ = app;
    app->pushPopup(std::move(content), anchor, std::move(options));
    if (onChanged_) onChanged_(true);
}

void MenuAnchor::dismissed(const Widget* menu) {
    if (menu_.get() != menu) return;
    menu_.reset();
    if (onChanged_) onChanged_(false);
}

void MenuAnchor::close() {
    if (menu_ && app_ != nullptr) app_->dismissPopup(menu_.get());
}

void MenuAnchor::toggle() {
    if (isOpen()) close();
    else open();
}

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
    if (open_ < 0 || !menu_ || app_ == nullptr || !app_->isPopupOpen(menu_.get())) return -1;
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
    const Widget* raw = menu.get();
    options.onDismissed = [self, raw] {
        std::shared_ptr<Widget> alive = self.lock();
        if (!alive) return;
        auto& bar = static_cast<MenuBar&>(*alive);
        if (bar.menu_.get() != raw) return;
        bar.menu_.reset();
        bar.open_ = -1;
    };
    const Rect anchor = app->grid().geometryOf(items_[static_cast<std::size_t>(index)].get()).value_or(Geometry{}).rect();
    menu_ = menu;
    open_ = index;
    app_ = app;
    app->pushPopup(menu, anchor, std::move(options));
}

void MenuBar::close() {
    if (menu_ && app_ != nullptr) app_->dismissPopup(menu_.get());
}

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

void showContextMenu(std::shared_ptr<Widget> menu, glm::vec2 at) {
    Application* app = Application::current();
    if (app == nullptr || !menu) return;
    PopupOptions options;
    options.placement = Placement::AtPoint;
    app->pushPopup(std::move(menu), Rect::fromSize(at, glm::vec2(0.0f)), std::move(options));
}

}
