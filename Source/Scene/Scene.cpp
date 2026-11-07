#include "Scene/Scene.hpp"

#include "Scene/DrawList.hpp"
#include "Scene/Node.hpp"
#include "Scene/NodeTypes.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/Transform.hpp"

#include <algorithm>
#include <stdexcept>

namespace cinder::Scene {
namespace {

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

    if (id && (*id < 1 || *id > MAX_ID)) throw std::runtime_error("node id " + std::to_string(*id) + " is out of range");
    if (!id && nextId_ > MAX_ID) throw std::runtime_error("the scene has run out of node ids");
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
    std::unique_ptr<Node> copy;
    if (const auto* placeholder = dynamic_cast<const Placeholder*>(&source)) {
        copy = std::make_unique<Placeholder>(placeholder->className(), placeholder->fields());
    } else {
        copy = types_.create(types_.nameOf(source));
    }
    if (copy == nullptr) return nullptr;

    cinder::Reflect::copyProps(source.propList(), source.propTarget(), copy->propTarget());
    if (source.transform() != nullptr && copy->transform() != nullptr) {
        cinder::Reflect::copyProps(cinder::Reflect::props<Transform>(), source.transform(), copy->transform());
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
    runTimers(dt);
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
    teardown(*node, false);
}

void Scene::flushDestroy() {
    for (std::size_t i = 0; i < pendingDestroy_.size(); ++i) {
        Node* node = byId(pendingDestroy_[i]);
        if (node == nullptr) continue;
        detach(*node);
        teardown(*node, false);
    }
    pendingDestroy_.clear();
}

void Scene::markDestroyed(Node& node) {
    node.destroyed_ = true;
    for (Node* child : node.children_) markDestroyed(*child);
}

void Scene::teardown(Node& node, bool clearing) {
    if (!clearing) emit(node, Signal::Destroying);

    const std::vector<Node*> children = node.children_;
    for (std::size_t i = children.size(); i-- > 0;) teardown(*children[i], clearing);
    node.children_.clear();

    if (!clearing) unqueueStart(&node);
    if (node.started_) node.onDestroy();
    if (!clearing) dropHandlers(node.id_);
    nodes_.erase(node.id_);
}

void Scene::clear() {
    const std::vector<Node*> roots = roots_;
    for (std::size_t i = roots.size(); i-- > 0;) teardown(*roots[i], true);
    roots_.clear();
    nodes_.clear();
    pendingStart_.clear();
    pendingDestroy_.clear();
    connections_.clear();
    timers_.clear();
    time_ = 0.0;
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
    if (parent != nullptr) emit(*parent, Signal::ChildAdded, {&node, {}});
}

void Scene::detach(Node& node) {
    Node* parent = node.parent_;
    std::vector<Node*>& siblings = parent == nullptr ? roots_ : parent->children_;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), &node), siblings.end());
    node.parent_ = nullptr;

    if (parent != nullptr) emit(*parent, Signal::ChildRemoved, {&node, {}});
}

void Scene::unqueueStart(Node* node) {
    for (Node*& pending : pendingStart_) {
        if (pending == node) pending = nullptr;
    }
}

void Scene::attributeChanged(Node& node, const std::string& name) {
    emit(node, Signal::AttributeChanged, {nullptr, name});
}

void Scene::hook(Node& node, Signal signal, const SignalArgs& args) {
    switch (signal) {
        case Signal::AttributeChanged: node.onAttributeChanged(args.name); break;
        case Signal::ChildAdded: node.onChildAdded(*args.other); break;
        case Signal::ChildRemoved: node.onChildRemoved(*args.other); break;
        case Signal::Touched: node.onTouched(*args.other); break;
        case Signal::TouchEnded: node.onTouchEnded(*args.other); break;
        case Signal::Destroying: break;
    }
}

void Scene::emit(Node& source, Signal signal, const SignalArgs& args) {
    if (source.started_ && !source.destroyed_) hook(source, signal, args);

    std::vector<int> handles;
    for (const Connection& connection : connections_) {
        if (connection.source == source.id_ && connection.signal == signal) handles.push_back(connection.id);
    }
    for (const int handle : handles) {
        auto found = std::find_if(connections_.begin(), connections_.end(),
                                  [handle](const Connection& connection) { return connection.id == handle; });
        if (found == connections_.end()) continue;
        const Node* owner = byId(found->owner);
        if (owner == nullptr || !owner->started_ || owner->destroyed_) continue;
        const SignalHandler handler = found->handler;
        handler(args);
    }
}

int Scene::connect(Node& source, Signal signal, Node& owner, SignalHandler handler) {
    const int handle = nextHandle_++;
    connections_.push_back(Connection{handle, source.id_, signal, owner.id_, std::move(handler)});
    return handle;
}

void Scene::disconnect(int connection) {
    std::erase_if(connections_, [connection](const Connection& entry) { return entry.id == connection; });
}

int Scene::after(Node& owner, double seconds, TimerHandler handler) {
    return schedule(owner, seconds, false, std::move(handler));
}

int Scene::every(Node& owner, double seconds, TimerHandler handler) {
    return schedule(owner, seconds, true, std::move(handler));
}

int Scene::schedule(Node& owner, double seconds, bool repeat, TimerHandler handler) {
    const double interval = std::max(seconds, 0.0);
    const int handle = nextHandle_++;
    timers_.push_back(Timer{handle, owner.id_, time_ + interval, interval, repeat, std::move(handler)});
    return handle;
}

void Scene::cancel(int timer) {
    std::erase_if(timers_, [timer](const Timer& entry) { return entry.id == timer; });
}

void Scene::runTimers(double dt) {
    time_ += dt;

    std::vector<int> due;
    for (const Timer& timer : timers_) {
        if (timer.due <= time_) due.push_back(timer.id);
    }
    for (const int handle : due) {
        auto found = std::find_if(timers_.begin(), timers_.end(),
                                  [handle](const Timer& timer) { return timer.id == handle; });
        if (found == timers_.end()) continue;
        const Node* owner = byId(found->owner);
        if (owner == nullptr || owner->destroyed_) {
            timers_.erase(found);
            continue;
        }

        const TimerHandler handler = found->handler;
        if (found->repeat) {
            found->due = std::max(found->due + found->interval, time_);
        } else {
            timers_.erase(found);
        }
        handler();
    }
}

void Scene::dropHandlers(int node) {
    std::erase_if(connections_, [node](const Connection& entry) { return entry.source == node || entry.owner == node; });
    std::erase_if(timers_, [node](const Timer& entry) { return entry.owner == node; });
}

}
