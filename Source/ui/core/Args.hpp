#pragma once

#include "platform/Cursor.hpp"
#include "ui/core/Attribute.hpp"
#include "ui/core/Widget.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace cinder::ui {

template <typename Derived, typename Built>
struct Args {
    Attribute<Visibility> visibility_{Visibility::Visible};
    Attribute<bool> isEnabled_{true};
    Attribute<std::string> toolTipText_;
    std::optional<cinder::platform::CursorShape> cursor_;
    std::function<void(const std::shared_ptr<Built>&)> assign_;

    Derived& visibility(Attribute<Visibility> value) {
        visibility_ = std::move(value);
        return self();
    }
    Derived& isEnabled(Attribute<bool> value) {
        isEnabled_ = std::move(value);
        return self();
    }
    Derived& toolTipText(Attribute<std::string> value) {
        toolTipText_ = std::move(value);
        return self();
    }
    Derived& cursor(cinder::platform::CursorShape value) {
        cursor_ = value;
        return self();
    }

    template <typename Target>
    Derived& assign(std::shared_ptr<Target>& target) {
        assign_ = [&target](const std::shared_ptr<Built>& widget) { target = widget; };
        return self();
    }

    std::shared_ptr<Built> build() const {
        std::shared_ptr<Built> widget = std::make_shared<Built>();
        widget->setVisibility(visibility_);
        widget->setEnabled(isEnabled_);
        widget->setToolTipText(toolTipText_);
        if (cursor_) widget->setCursor(cursor_);
        widget->construct(static_cast<const Derived&>(*this));
        if (assign_) assign_(widget);
        return widget;
    }

    template <typename Target>
        requires std::is_base_of_v<Target, Built>
    operator std::shared_ptr<Target>() const {
        return build();
    }

protected:
    Derived& self() { return static_cast<Derived&>(*this); }
};

template <typename T>
typename T::Args make() {
    return typename T::Args{};
}

}

#define UI_ATTR(Type, Name, ...)                                       \
    ::cinder::ui::Attribute<Type> Name##_{__VA_ARGS__};                \
    auto& Name(::cinder::ui::Attribute<Type> value) {                  \
        Name##_ = std::move(value);                                    \
        return this->self();                                           \
    }

#define UI_ARG(Type, Name, ...)                \
    Type Name##_{__VA_ARGS__};                 \
    auto& Name(Type value) {                   \
        Name##_ = std::move(value);            \
        return this->self();                   \
    }

#define UI_EVENT(Type, Name) UI_ARG(Type, Name)

#define UI_CONTENT(Name)                                                        \
    std::shared_ptr<::cinder::ui::Widget> Name##_;                              \
    auto& operator[](std::shared_ptr<::cinder::ui::Widget> widget) {           \
        Name##_ = std::move(widget);                                            \
        return this->self();                                                    \
    }

#define UI_SLOTS(SlotType, Name)                  \
    std::vector<SlotType> Name##_;                \
    auto& operator+(SlotType slot) {              \
        Name##_.push_back(std::move(slot));       \
        return this->self();                      \
    }
