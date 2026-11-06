#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/widgets/Menu.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::ui {

class Button;

class ComboButton : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, ComboButton> {
        UI_EVENT(MenuAnchor::MenuFactory, onGetMenuContent)
        UI_ARG(std::string, buttonStyle, "ComboButton")
        UI_ARG(bool, hasDownArrow, true)
        UI_ARG(Placement, placement, Placement::Below)
        UI_EVENT(OnBoolChanged, onMenuOpenChanged)
        UI_CONTENT(buttonContent)
    };

    void construct(const Args& args);

    void open() { anchor_->open(); }
    void close() { anchor_->close(); }
    bool isOpen() const { return anchor_->isOpen(); }
    const std::shared_ptr<MenuAnchor>& anchor() const { return anchor_; }
    const std::shared_ptr<Button>& button() const { return button_; }

private:
    std::shared_ptr<MenuAnchor> anchor_;
    std::shared_ptr<Button> button_;
};

class ComboBox : public CompoundWidget {
public:
    using OnSelectionChanged = std::function<void(const std::string&)>;
    using LabelOf = std::function<std::string(const std::string&)>;

    struct Args : ::cinder::ui::Args<Args, ComboBox> {
        UI_ATTR(std::vector<std::string>, options)
        UI_ATTR(std::string, selectedOption)
        UI_EVENT(OnSelectionChanged, onSelectionChanged)
        UI_EVENT(LabelOf, labelOf)
        UI_ARG(std::string, buttonStyle, "ComboButton")
    };

    void construct(const Args& args);

    std::string selected() const { return selected_.get(); }
    void select(const std::string& option);
    std::string labelFor(const std::string& option) const { return labelOf_ ? labelOf_(option) : option; }
    std::vector<std::string> options() const { return options_.get(); }
    const std::shared_ptr<ComboButton>& button() const { return button_; }

private:
    std::shared_ptr<Widget> buildMenu();

    Attribute<std::vector<std::string>> options_;
    Attribute<std::string> selected_;
    OnSelectionChanged onChanged_;
    LabelOf labelOf_;
    std::shared_ptr<ComboButton> button_;
};

}
