#include "scene/NodeTypes.hpp"

#include "scene/Placeholder.hpp"

#include "scene/Node.hpp"

namespace cinder::scene {

NodeTypes::~NodeTypes() = default;

void NodeTypes::bind(std::string name, std::string type,
                      const cinder::reflect::PropList* props, Factory factory) {
    auto existing = byName_.find(name);
    if (existing != byName_.end()) {
        Entry& slot = entries_[existing->second];
        if (slot.factory) byType_.erase(slot.type);
        slot.type = type;
        slot.props = props;
        slot.factory = std::move(factory);
        slot.fallback.reset();
        byType_.emplace(std::move(type), existing->second);
        return;
    }

    const std::size_t index = entries_.size();
    entries_.push_back(Entry{name, type, props, std::move(factory), nullptr});
    byName_.emplace(std::move(name), index);
    byType_.emplace(std::move(type), index);
}

void NodeTypes::remove(std::string_view name) {
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

const NodeTypes::Entry* NodeTypes::find(std::string_view name) const {
    auto found = byName_.find(std::string(name));
    if (found == byName_.end() || !entries_[found->second].factory) return nullptr;
    return &entries_[found->second];
}

std::unique_ptr<Node> NodeTypes::create(std::string_view name) const {
    const Entry* entry = find(name);
    return entry == nullptr ? nullptr : entry->factory();
}

std::string_view NodeTypes::nameOf(const std::type_info& type) const {
    auto found = byType_.find(type.name());
    return found == byType_.end() ? std::string_view{} : entries_[found->second].name;
}

std::string_view NodeTypes::nameOf(const Node& node) const {
    if (const auto* placeholder = dynamic_cast<const Placeholder*>(&node)) return placeholder->className();
    return nameOf(typeid(node));
}

const cinder::reflect::PropList* NodeTypes::propsOf(std::string_view name) const {
    const Entry* entry = find(name);
    return entry == nullptr ? nullptr : entry->props;
}

const Node* NodeTypes::fallback(std::string_view name) const {
    const Entry* entry = find(name);
    if (entry == nullptr) return nullptr;
    if (!entry->fallback) entry->fallback = entry->factory();
    return entry->fallback.get();
}

}
