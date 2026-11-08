#pragma once

#include "UI/Core/Args.hpp"
#include "UI/Core/CompoundWidget.hpp"
#include "UI/Core/Delegates.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::Dev::Panels {

struct DialogChoice {
    std::string label;
    std::function<void()> action;
    bool primary = false;
    std::function<bool()> enabled;
};

class Dialog : public cinder::UI::CompoundWidget {
public:
    struct Args : ::cinder::UI::Args<Args, Dialog> {
        UI_ATTR(std::string, title)
        UI_ATTR(std::string, message)
        UI_ARG(std::shared_ptr<cinder::UI::Widget>, body)
        UI_ARG(std::vector<DialogChoice>, choices)
        UI_EVENT(cinder::UI::OnVoid, onCancel)
    };

    void construct(const Args& args);

    bool supportsKeyboardFocus() const override { return true; }
    cinder::UI::Reply onKeyDown(const cinder::UI::Geometry& geometry, const cinder::UI::KeyEvent& event) override;
    cinder::UI::Reply onMouseDown(const cinder::UI::Geometry& geometry, const cinder::UI::PointerEvent& event) override;

private:
    cinder::UI::OnVoid onCancel_;
    std::function<void()> primary_;
    std::function<bool()> primaryEnabled_;
};

}
