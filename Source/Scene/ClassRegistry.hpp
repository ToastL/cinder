#pragma once

#include "Reflect/Reflect.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Component.hpp"
#include "Scene/PrefabCache.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace cinder::Scene {

enum class ClassKind { Actor, Component };

class ClassRegistry {
public:
    using Factory = std::function<std::unique_ptr<Object>()>;

    struct Entry {
        std::string name;
        std::string type;
        ClassKind kind;
        const cinder::Reflect::PropList* props;
        Factory factory;
        mutable std::unique_ptr<Object> fallback;
    };

    ClassRegistry() = default;
    ~ClassRegistry();

    ClassRegistry(const ClassRegistry&) = delete;
    ClassRegistry& operator=(const ClassRegistry&) = delete;

    template <class T>
    void add(std::string name) {
        static_assert(std::is_base_of_v<Actor, T> || std::is_base_of_v<Component, T>,
                      "a registered class derives from Actor or Component");
        bind(std::move(name), typeid(T).name(),
             std::is_base_of_v<Actor, T> ? ClassKind::Actor : ClassKind::Component,
             &cinder::Reflect::props<T>(), []() -> std::unique_ptr<Object> { return std::make_unique<T>(); });
    }

    void remove(std::string_view name);

    std::unique_ptr<Actor> createActor(std::string_view name) const;
    std::unique_ptr<Component> createComponent(std::string_view name) const;

    std::string_view nameOf(const std::type_info& type) const;
    std::string_view nameOf(const Object& object) const;
    std::string_view displayName(const Object& object) const;
    const Object* fallback(std::string_view name) const;
    const Entry* entry(std::string_view name) const { return find(name); }

    const std::vector<Entry>& registered() const { return entries_; }

    PrefabCache& prefabs() const { return prefabs_; }

private:
    void bind(std::string name, std::string type, ClassKind kind,
              const cinder::Reflect::PropList* props, Factory factory);
    const Entry* find(std::string_view name) const;

    std::vector<Entry> entries_;
    std::unordered_map<std::string, std::size_t> byName_;
    std::unordered_map<std::string, std::size_t> byType_;
    mutable PrefabCache prefabs_;
};

}
