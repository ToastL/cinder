#pragma once

#include "ui/framework/Application.hpp"

#include <algorithm>

namespace cinder::ui {

class Application::Scope {
public:
    explicit Scope(Application& application);
    ~Scope();

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    Application* previous_;
};

namespace detail {

template <typename Handler>
Reply bubble(const WidgetPath& path, Handler handler) {
    for (auto entry = path.rbegin(); entry != path.rend(); ++entry) {
        if (!entry->enabled) continue;
        Reply reply = handler(*entry);
        if (reply.isHandled()) return reply;
    }
    return Reply::unhandled();
}

inline bool holds(const WidgetPath& path, const Widget* widget) {
    return std::any_of(path.begin(), path.end(), [widget](const PathEntry& entry) { return entry.widget.get() == widget; });
}

inline bool within(const Widget* root, const Widget* target) {
    if (root == nullptr || target == nullptr) return false;
    if (root == target) return true;
    for (int i = 0; i < root->childCount(); ++i) {
        if (within(root->childAt(i), target)) return true;
    }
    return false;
}

}
}
