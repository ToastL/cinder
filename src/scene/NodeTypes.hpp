#pragma once

#include "reflect/Reflect.hpp"
#include "scene/Node.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace cinder::scene {

class NodeTypes {
public:
    using Factory = std::function<std::unique_ptr<Node>()>;

    struct Entry {
        std::string name;
        std::type_index type;
        const cinder::reflect::PropList* props;
        Factory factory;
        mutable std::unique_ptr<Node> fallback;
    };

    NodeTypes() = default;
    ~NodeTypes();

    NodeTypes(const NodeTypes&) = delete;
    NodeTypes& operator=(const NodeTypes&) = delete;

    template <class T>
    void add(std::string name) {
        add<T>(std::move(name), [] { return std::make_unique<T>(); });
    }

    template <class T, class F>
    void add(std::string name, F make) {
        bind(std::move(name), std::type_index(typeid(T)), &cinder::reflect::props<T>(),
             [factory = std::move(make)]() -> std::unique_ptr<Node> { return factory(); });
    }

    std::unique_ptr<Node> create(std::string_view name) const;
    std::string_view nameOf(std::type_index type) const;
    std::string_view nameOf(const Node& node) const;
    const cinder::reflect::PropList* propsOf(std::string_view name) const;
    const Node* fallback(std::string_view name) const;
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
