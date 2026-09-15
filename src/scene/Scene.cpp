#include "scene/Scene.hpp"

#include "scene/DrawList.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Transform.hpp"

#include <algorithm>
#include <stdexcept>

namespace cinder::scene {
namespace {

void copyProps(const cinder::reflect::PropList& defs, const void* from, void* to) {
    float values[4]{};
    for (const cinder::reflect::PropDef& def : defs) {
        switch (def.type()) {
            case cinder::reflect::PropType::Bool:
                def.writeBool(to, def.readBool(from));
                break;
            case cinder::reflect::PropType::String:
            case cinder::reflect::PropType::Enum:
                def.writeText(to, def.readText(from));
                break;
            default:
                def.read(from, values);
                def.write(to, values);
                break;
        }
    }
}

Node* findIn(Node& node, std::string_view name) {
    if (!node.destroyed() && node.name() == name) return &node;
    for (Node* child : node.children()) {
        if (Node* found = findIn(*child, name)) return found;
    }
    return nullptr;
}

}

Scene::Scene(NodeTypes& types) : types_(types) {}

Scene::~Scene() { clear(); }

Node* Scene::insert(std::unique_ptr<Node> owned, Node* parent, std::optional<int> id) {
    if (owned == nullptr) return nullptr;
    if (parent != nullptr && parent->scene_ != this) {
        throw std::runtime_error("a parent must belong to the same scene");
    }

    const int assigned = id.value_or(nextId_);
    if (nodes_.count(assigned) != 0) {
        throw std::runtime_error("node id " + std::to_string(assigned) + " is taken");
    }

    Node* node = owned.get();
    node->scene_ = this;
    node->id_ = assigned;
    if (node->name_.empty()) {
        const std::string_view type = types_.nameOf(*node);
        node->name_ = type.empty() ? std::string("Node") : std::string(type);
    }

    nodes_.emplace(assigned, std::move(owned));
    if (assigned >= nextId_) nextId_ = assigned + 1;

    attach(*node, parent);
    pendingStart_.push_back(node);
    return node;
}

Node* Scene::create(std::string_view className, Node* parent) {
    std::unique_ptr<Node> node = types_.create(className);
    return node == nullptr ? nullptr : insert(std::move(node), parent);
}

Node* Scene::clone(const Node& source, Node* parent) {
    std::unique_ptr<Node> copy = types_.create(types_.nameOf(source));
    if (copy == nullptr) return nullptr;

    copyProps(source.propList(), source.propTarget(), copy->propTarget());
    if (source.transform() != nullptr && copy->transform() != nullptr) {
        copyProps(cinder::reflect::props<Transform>(), source.transform(), copy->transform());
    }
    copy->name_ = source.name_;
    copy->attributes_ = source.attributes_;

    const std::vector<Node*> children = source.children_;
    Node* node = insert(std::move(copy), parent);
    for (Node* child : children) {
        if (!child->destroyed_) clone(*child, node);
    }
    return node;
}

Node* Scene::byId(int id) const {
    auto found = nodes_.find(id);
    return found == nodes_.end() ? nullptr : found->second.get();
}

Node* Scene::find(std::string_view name) const {
    for (Node* root : roots_) {
        if (Node* found = findIn(*root, name)) return found;
    }
    return nullptr;
}

void Scene::update(float dt) {
    startPending();
    for (std::size_t i = 0; i < roots_.size(); ++i) update(*roots_[i], dt);
    flushDestroy();
}

void Scene::update(Node& node, float dt) {
    if (node.destroyed_ || !node.enabled_) return;
    if (node.started_) node.onUpdate(dt);
    for (std::size_t i = 0; i < node.children_.size(); ++i) update(*node.children_[i], dt);
}

void Scene::render(float alpha, DrawList& draws) {
    for (std::size_t i = 0; i < roots_.size(); ++i) render(*roots_[i], alpha, draws);
}

void Scene::render(Node& node, float alpha, DrawList& draws) {
    if (node.destroyed_ || !node.enabled_) return;
    node.onRender(alpha, draws);
    for (std::size_t i = 0; i < node.children_.size(); ++i) render(*node.children_[i], alpha, draws);
}

void Scene::startPending() {
    for (std::size_t i = 0; i < pendingStart_.size(); ++i) {
        Node* node = pendingStart_[i];
        if (node == nullptr || node->destroyed_) continue;
        node->started_ = true;
        node->onStart();
    }
    pendingStart_.clear();
}

void Scene::destroy(Node* node) {
    if (node == nullptr || node->destroyed_) return;
    markDestroyed(*node);
    pendingDestroy_.push_back(node->id_);
}

void Scene::destroyNow(Node* node) {
    if (node == nullptr || byId(node->id_) != node) return;
    markDestroyed(*node);
    detach(*node);
    teardown(*node, true);
}

void Scene::flushDestroy() {
    for (std::size_t i = 0; i < pendingDestroy_.size(); ++i) {
        Node* node = byId(pendingDestroy_[i]);
        if (node == nullptr) continue;
        detach(*node);
        teardown(*node, true);
    }
    pendingDestroy_.clear();
}

void Scene::markDestroyed(Node& node) {
    node.destroyed_ = true;
    for (Node* child : node.children_) markDestroyed(*child);
}

void Scene::teardown(Node& node, bool notify) {
    if (notify && observer_ != nullptr) observer_->destroying(node);

    const std::vector<Node*> children = node.children_;
    for (std::size_t i = children.size(); i-- > 0;) teardown(*children[i], notify);
    node.children_.clear();

    unqueueStart(&node);
    if (node.started_) node.onDestroy();
    nodes_.erase(node.id_);
}

void Scene::clear() {
    const std::vector<Node*> roots = roots_;
    for (std::size_t i = roots.size(); i-- > 0;) teardown(*roots[i], false);
    roots_.clear();
    nodes_.clear();
    pendingStart_.clear();
    pendingDestroy_.clear();
}

void Scene::reparent(Node& node, Node* next) {
    if (next != nullptr && next->scene_ != this) {
        throw std::runtime_error("a parent must belong to the same scene");
    }
    detach(node);
    attach(node, next);
}

void Scene::attach(Node& node, Node* parent) {
    node.parent_ = parent;
    if (parent == nullptr) roots_.push_back(&node);
    else parent->children_.push_back(&node);

    if (Transform* transform = node.transform()) transform->dirty();
    if (parent != nullptr && observer_ != nullptr) observer_->childAdded(*parent, node);
}

void Scene::detach(Node& node) {
    Node* parent = node.parent_;
    std::vector<Node*>& siblings = parent == nullptr ? roots_ : parent->children_;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), &node), siblings.end());
    node.parent_ = nullptr;

    if (parent != nullptr && observer_ != nullptr) observer_->childRemoved(*parent, node);
}

void Scene::unqueueStart(Node* node) {
    for (Node*& pending : pendingStart_) {
        if (pending == node) pending = nullptr;
    }
}

void Scene::attributeChanged(Node& node, const std::string& name) {
    if (observer_ != nullptr) observer_->attributeChanged(node, name);
}

}
