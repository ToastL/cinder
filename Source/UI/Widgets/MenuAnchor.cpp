#include "UI/Widgets/MenuAnchor.hpp"

#include <utility>

namespace cinder::UI {

void MenuAnchor::construct(const Args& args) {
    factory_ = args.onGetMenuContent_;
    placement_ = args.placement_;
    matchWidth_ = args.matchWidth_;
    onChanged_ = args.onMenuOpenChanged_;
    childSlot_.widget = args.content_;
}

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
    options.onDismissed = [self] {
        std::shared_ptr<Widget> alive = self.lock();
        if (!alive) return;
        auto& anchor = static_cast<MenuAnchor&>(*alive);
        if (anchor.onChanged_) anchor.onChanged_(false);
    };
    popup_.open(*app, std::move(content), anchor, std::move(options));
    if (onChanged_) onChanged_(true);
}

void MenuAnchor::close() { popup_.close(); }

void MenuAnchor::toggle() {
    if (isOpen()) close();
    else open();
}

}
