#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Delegates.hpp"

#include <string>
#include <vector>

namespace cinder::dev::panels {

struct DialogChoice {
    std::string label;
    std::function<void()> action;
    bool primary = false;
};

class Dialog : public cinder::ui::CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Dialog> {
        UI_ATTR(std::string, title)
        UI_ATTR(std::string, message)
        UI_ARG(std::vector<DialogChoice>, choices)
        UI_EVENT(cinder::ui::OnVoid, onCancel)
    };

    void construct(const Args& args);

    bool supportsKeyboardFocus() const override { return true; }
    cinder::ui::Reply onKeyDown(const cinder::ui::Geometry& geometry, const cinder::ui::KeyEvent& event) override;
    cinder::ui::Reply onMouseDown(const cinder::ui::Geometry& geometry, const cinder::ui::PointerEvent& event) override;

private:
    cinder::ui::OnVoid onCancel_;
    std::function<void()> primary_;
};

}
