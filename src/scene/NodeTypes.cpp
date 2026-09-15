#include "scene/NodeTypes.hpp"

#include "scene/Node.hpp"

namespace cinder::scene {

NodeTypes::~NodeTypes() = default;

void NodeTypes::bind(std::string name, std::type_index type,
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

const NodeTypes::Entry* NodeTypes::find(std::string_view name) const {
    auto found = byName_.find(std::string(name));
    return found == byName_.end() ? nullptr : &entries_[found->second];
}

std::unique_ptr<Node> NodeTypes::create(std::string_view name) const {
    const Entry* entry = find(name);
    return entry == nullptr ? nullptr : entry->factory();
}

std::string_view NodeTypes::nameOf(std::type_index type) const {
    auto found = byType_.find(type);
    return found == byType_.end() ? std::string_view{} : entries_[found->second].name;
}

std::string_view NodeTypes::nameOf(const Node& node) const {
    return nameOf(std::type_index(typeid(node)));
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
