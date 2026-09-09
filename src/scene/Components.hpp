#pragma once

#include "reflect/Reflect.hpp"
#include "scene/Component.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace cinder::scene {

class Components {
public:
    using Factory = std::function<std::unique_ptr<Component>()>;

    struct Entry {
        std::string name;
        std::type_index type;
        const cinder::reflect::PropList* props;
        Factory factory;
        mutable std::unique_ptr<Component> fallback;
    };

    Components() = default;
    ~Components();

    Components(const Components&) = delete;
    Components& operator=(const Components&) = delete;

    template <class T>
    void add(std::string name) {
        add<T>(std::move(name), [] { return std::make_unique<T>(); });
    }

    template <class T, class F>
    void add(std::string name, F make) {
        bind(std::move(name), std::type_index(typeid(T)), &cinder::reflect::props<T>(),
             [factory = std::move(make)]() -> std::unique_ptr<Component> { return factory(); });
    }

    std::unique_ptr<Component> create(std::string_view name) const;
    std::string_view nameOf(std::type_index type) const;
    std::string_view nameOf(const Component& component) const;
    const cinder::reflect::PropList* propsOf(std::string_view name) const;
    const Component* fallback(std::string_view name) const;
    const Entry* entry(std::string_view name) const { return find(name); }

    const std::vector<Entry>& registered() const { return entries_; }

private:
    void bind(std::string name, std::type_index type,
              const cinder::reflect::PropList* props, Factory factory);
    const Entry* find(std::string_view name) const;

    std::vector<Entry> entries_;
    std::unordered_map<std::string, std::size_t> byName_;
    std::unordered_map<std::type_index, std::size_t> byType_;
};

}
