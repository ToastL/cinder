#include "scene/Node.hpp"

#include "scene/Attributes.hpp"
#include "scene/Scene.hpp"

#include <stdexcept>

namespace cinder::scene {

Node* Node::findFirstChild(std::string_view name) const {
    for (Node* child : children_) {
        if (!child->destroyed_ && child->name_ == name) return child;
    }
    return nullptr;
}

bool Node::isAncestorOf(const Node* other) const {
    for (const Node* entry = other; entry != nullptr; entry = entry->parent_) {
        if (entry == this) return true;
    }
    return false;
}

void Node::setParent(Node* next) {
    if (next == this || isAncestorOf(next)) {
        throw std::runtime_error("reparenting would cycle the node tree");
    }
    if (scene_ != nullptr) scene_->reparent(*this, next);
}

bool Node::enabledInHierarchy() const {
    for (const Node* entry = this; entry != nullptr; entry = entry->parent_) {
        if (!entry->enabled_) return false;
    }
    return true;
}

void Node::destroy() {
    if (scene_ != nullptr) scene_->destroy(this);
}

const PropValue* Node::attribute(const std::string& name) const {
    auto found = attributes_.find(name);
    return found == attributes_.end() ? nullptr : &found->second;
}

void Node::setAttribute(const std::string& name, PropValue value) {
    auto found = attributes_.find(name);
    if (found != attributes_.end() && sameAttribute(found->second, value)) return;

    attributes_.insert_or_assign(name, std::move(value));
    if (scene_ != nullptr) scene_->attributeChanged(*this, name);
}

void Node::removeAttribute(const std::string& name) {
    if (attributes_.erase(name) == 0) return;
    if (scene_ != nullptr) scene_->attributeChanged(*this, name);
}

}
