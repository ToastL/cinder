#pragma once

#include "UI/Core/Args.hpp"
#include "UI/Core/CompoundWidget.hpp"
#include "UI/Core/Delegates.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Framework/PopupHandle.hpp"

#include <functional>
#include <memory>

namespace cinder::UI {

class MenuAnchor : public CompoundWidget {
public:
    using MenuFactory = std::function<std::shared_ptr<Widget>()>;

    struct Args : ::cinder::UI::Args<Args, MenuAnchor> {
        UI_EVENT(MenuFactory, onGetMenuContent)
        UI_ARG(Placement, placement, Placement::Below)
        UI_ARG(bool, matchWidth)
        UI_EVENT(OnBoolChanged, onMenuOpenChanged)
        UI_CONTENT(content)
    };

    void construct(const Args& args);

    void open();
    void close();
    void toggle();
    bool isOpen() const { return popup_.isOpen(); }
    const std::shared_ptr<Widget>& menu() const { return popup_.content(); }

private:
    MenuFactory factory_;
    Placement placement_ = Placement::Below;
    bool matchWidth_ = false;
    OnBoolChanged onChanged_;
    PopupHandle popup_;
};

}
