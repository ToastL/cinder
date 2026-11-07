#include "Scene/ClassRegistry.hpp"

#include "Scene/Placeholder.hpp"

namespace cinder::Scene {

ClassRegistry::~ClassRegistry() = default;

void ClassRegistry::bind(std::string name, std::string type, ClassKind kind,
                         const cinder::Reflect::PropList* props, Factory factory) {
    auto existing = byName_.find(name);
    if (existing != byName_.end()) {
        Entry& slot = entries_[existing->second];
        if (slot.factory) byType_.erase(slot.type);
        slot.type = type;
        slot.kind = kind;
        slot.props = props;
        slot.factory = std::move(factory);
        slot.fallback.reset();
        byType_.emplace(std::move(type), existing->second);
        return;
    }

    const std::size_t index = entries_.size();
    entries_.push_back(Entry{name, type, kind, props, std::move(factory), nullptr});
    byName_.emplace(std::move(name), index);
    byType_.emplace(std::move(type), index);
}

void ClassRegistry::remove(std::string_view name) {
    auto found = byName_.find(std::string(name));
    if (found == byName_.end()) return;
    Entry& slot = entries_[found->second];
    if (!slot.factory) return;
    byType_.erase(slot.type);
    slot.type.clear();
    slot.props = nullptr;
    slot.factory = nullptr;
    slot.fallback.reset();
}

const ClassRegistry::Entry* ClassRegistry::find(std::string_view name) const {
    auto found = byName_.find(std::string(name));
    if (found == byName_.end() || !entries_[found->second].factory) return nullptr;
    return &entries_[found->second];
}

std::unique_ptr<Actor> ClassRegistry::createActor(std::string_view name) const {
    const Entry* entry = find(name);
    if (entry == nullptr || entry->kind != ClassKind::Actor) return nullptr;
    return std::unique_ptr<Actor>(static_cast<Actor*>(entry->factory().release()));
}

std::unique_ptr<Component> ClassRegistry::createComponent(std::string_view name) const {
    const Entry* entry = find(name);
    if (entry == nullptr || entry->kind != ClassKind::Component) return nullptr;
    return std::unique_ptr<Component>(static_cast<Component*>(entry->factory().release()));
}

std::string_view ClassRegistry::nameOf(const std::type_info& type) const {
    auto found = byType_.find(type.name());
    return found == byType_.end() ? std::string_view{} : entries_[found->second].name;
}

std::string_view ClassRegistry::nameOf(const Object& object) const {
    if (const auto* actor = dynamic_cast<const ActorPlaceholder*>(&object)) return actor->className();
    if (const auto* component = dynamic_cast<const ComponentPlaceholder*>(&object)) return component->className();
    return nameOf(typeid(object));
}

std::string_view ClassRegistry::displayName(const Object& object) const {
    return object.name().empty() ? nameOf(object) : std::string_view(object.name());
}

const Object* ClassRegistry::fallback(std::string_view name) const {
    const Entry* entry = find(name);
    if (entry == nullptr) return nullptr;
    if (!entry->fallback) entry->fallback = entry->factory();
    return entry->fallback.get();
}

}
