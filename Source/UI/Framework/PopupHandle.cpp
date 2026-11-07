#include "UI/Framework/PopupHandle.hpp"

#include <utility>

namespace cinder::UI {

PopupHandle::~PopupHandle() { close(); }

Application* PopupHandle::live() const {
    if (state_->app == nullptr || state_->alive.expired()) return nullptr;
    return state_->app;
}

void PopupHandle::open(Application& app, std::shared_ptr<Widget> content, const Rect& anchor, PopupOptions options) {
    if (!content) return;
    close();
    state_->content = content;
    state_->app = &app;
    state_->alive = app.lifetime();
    const std::weak_ptr<State> state = state_;
    const Widget* raw = content.get();
    options.onDismissed = [state, raw, then = std::move(options.onDismissed)] {
        const std::shared_ptr<State> alive = state.lock();
        if (!alive || alive->content.get() != raw) return;
        alive->content.reset();
        if (then) then();
    };
    app.pushPopup(std::move(content), anchor, std::move(options));
}

void PopupHandle::close() {
    if (!state_->content) return;
    if (Application* app = live()) {
        app->dismissPopup(state_->content.get());
    }
    state_->content.reset();
}

bool PopupHandle::isOpen() const {
    const Application* app = live();
    return state_->content && app != nullptr && app->isPopupOpen(state_->content.get());
}

}
