#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace cinder::ui {

template <typename T>
class Attribute {
public:
    Attribute() : value_(T{}) {}

    template <typename U>
        requires(!std::invocable<U> && std::convertible_to<U, T> && !std::same_as<std::decay_t<U>, Attribute>)
    Attribute(U&& value) : value_(T(std::forward<U>(value))) {}

    template <typename F>
        requires(std::invocable<F> && std::convertible_to<std::invoke_result_t<F>, T>
                 && !std::same_as<std::decay_t<F>, Attribute>)
    Attribute(F&& getter) : value_(std::function<T()>(std::forward<F>(getter))) {}

    T get() const {
        if (const auto* getter = std::get_if<std::function<T()>>(&value_)) return (*getter)();
        return std::get<T>(value_);
    }

    bool bound() const { return std::holds_alternative<std::function<T()>>(value_); }

    void set(T value) { value_ = std::move(value); }

private:
    std::variant<T, std::function<T()>> value_;
};

}
