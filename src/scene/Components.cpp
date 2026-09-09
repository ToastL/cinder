#include "scene/Components.hpp"

#include "scene/Component.hpp"

namespace cinder::scene {

Components::~Components() = default;

void Components::bind(std::string name, std::type_index type,
                      const cinder::reflect::PropList* props, Factory factory) {
    auto existing = byName_.find(name);
    if (existing != byName_.end()) {
        Entry& slot = entries_[existing->second];
        byType_.erase(slot.type);
        slot.type = type;
        slot.props = props;
        slot.factory = std::move(factory);
        slot.fallback.reset();
        byType_.emplace(type, existing->second);
        return;
    }

    const std::size_t index = entries_.size();
    entries_.push_back(Entry{name, type, props, std::move(factory), nullptr});
    byName_.emplace(std::move(name), index);
    byType_.emplace(type, index);
}

const Components::Entry* Components::find(std::string_view name) const {
    auto found = byName_.find(std::string(name));
    return found == byName_.end() ? nullptr : &entries_[found->second];
}

std::unique_ptr<Component> Components::create(std::string_view name) const {
    const Entry* entry = find(name);
    return entry == nullptr ? nullptr : entry->factory();
}

std::string_view Components::nameOf(std::type_index type) const {
    auto found = byType_.find(type);
    return found == byType_.end() ? std::string_view{} : entries_[found->second].name;
}

std::string_view Components::nameOf(const Component& component) const {
    return nameOf(std::type_index(typeid(component)));
}

const cinder::reflect::PropList* Components::propsOf(std::string_view name) const {
    const Entry* entry = find(name);
    return entry == nullptr ? nullptr : entry->props;
}

const Component* Components::fallback(std::string_view name) const {
    const Entry* entry = find(name);
    if (entry == nullptr) return nullptr;
    if (!entry->fallback) entry->fallback = entry->factory();
    return entry->fallback.get();
}

}
