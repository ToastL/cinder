#pragma once

#include "UI/Framework/Application.hpp"

#include <functional>
#include <memory>

namespace cinder::UI {

class PopupHandle {
public:
    PopupHandle() = default;
    ~PopupHandle();

    PopupHandle(const PopupHandle&) = delete;
    PopupHandle& operator=(const PopupHandle&) = delete;

    void open(Application& app, std::shared_ptr<Widget> content, const Rect& anchor, PopupOptions options);
    void close();
    bool isOpen() const;
    const std::shared_ptr<Widget>& content() const { return state_->content; }

private:
    struct State {
        std::shared_ptr<Widget> content;
        Application* app = nullptr;
        std::weak_ptr<const void> alive;
    };

    Application* live() const;

    std::shared_ptr<State> state_ = std::make_shared<State>();
};

}
