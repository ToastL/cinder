#include "scene/Scene.hpp"

#include "scene/Actor.hpp"
#include "scene/Component.hpp"
#include "scene/Components.hpp"
#include "scene/DrawList.hpp"

#include <algorithm>
#include <stdexcept>

namespace cinder::scene {

Scene::Scene(Components& types) : types_(types) {}

Scene::~Scene() { clear(); }

Actor* Scene::spawn(int id, const std::string& name, Actor* parent) {
    if (actors_.count(id) != 0) {
        throw std::runtime_error("actor id " + std::to_string(id) + " is taken");
    }

    auto owned = std::make_unique<Actor>(*this, id, name);
    Actor* actor = owned.get();
    actors_.emplace(id, std::move(owned));
    roots_.push_back(actor);

    if (parent != nullptr) actor->setParent(parent);
    if (id >= nextId_) nextId_ = id + 1;
    return actor;
}

Actor* Scene::byId(int id) const {
    auto found = actors_.find(id);
    return found == actors_.end() ? nullptr : found->second.get();
}

Actor* Scene::find(const std::string& name) const {
    for (Actor* root : roots_) {
        if (Actor* found = findIn(*root, name)) return found;
    }
    return nullptr;
}

Actor* Scene::findIn(Actor& actor, const std::string& name) const {
    if (actor.name() == name) return &actor;
    for (Actor* child : actor.children()) {
        if (Actor* found = findIn(*child, name)) return found;
    }
    return nullptr;
}

void Scene::update(float dt) {
    startPending();
    for (std::size_t i = 0; i < roots_.size(); ++i) update(*roots_[i], dt);
    flushDestroy();
}

void Scene::update(Actor& actor, float dt) {
    if (actor.destroyed() || !actor.activeSelf()) return;

    std::vector<std::unique_ptr<Component>>& components = actor.components();
    for (std::size_t i = 0; i < components.size(); ++i) {
        Component& component = *components[i];
        if (component.started() && component.isEnabled()) component.onUpdate(dt);
    }

    std::vector<Actor*>& children = actor.children();
    for (std::size_t i = 0; i < children.size(); ++i) update(*children[i], dt);
}

void Scene::render(float alpha, DrawList& draws) {
    for (std::size_t i = 0; i < roots_.size(); ++i) render(*roots_[i], alpha, draws);
}

void Scene::render(Actor& actor, float alpha, DrawList& draws) {
    if (actor.destroyed() || !actor.activeSelf()) return;

    std::vector<std::unique_ptr<Component>>& components = actor.components();
    for (std::size_t i = 0; i < components.size(); ++i) {
        Component& component = *components[i];
        if (component.isEnabled()) component.onRender(alpha, draws);
    }

    std::vector<Actor*>& children = actor.children();
    for (std::size_t i = 0; i < children.size(); ++i) render(*children[i], alpha, draws);
}

void Scene::startPending() {
    for (std::size_t i = 0; i < pendingStart_.size(); ++i) {
        Component* component = pendingStart_[i];
        if (component == nullptr) continue;
        if (component->actor() == nullptr || component->actor()->destroyed()) continue;
        component->started_ = true;
        component->onStart();
    }
    pendingStart_.clear();
}

void Scene::destroy(Actor* actor) {
    if (actor == nullptr || actor->destroyed()) return;
    actor->markDestroyed();
    pendingDestroy_.push_back(actor->id());
}

void Scene::flushDestroy() {
    for (std::size_t i = 0; i < pendingDestroy_.size(); ++i) {
        Actor* actor = byId(pendingDestroy_[i]);
        if (actor == nullptr) continue;

        Actor* parent = actor->parent();
        if (parent != nullptr) {
            std::vector<Actor*>& siblings = parent->children();
            siblings.erase(std::remove(siblings.begin(), siblings.end(), actor), siblings.end());
        } else {
            detachRoot(actor);
        }
        teardown(*actor);
    }
    pendingDestroy_.clear();
}

void Scene::teardown(Actor& actor) {
    std::vector<Actor*>& children = actor.children();
    for (std::size_t i = children.size(); i-- > 0;) teardown(*children[i]);
    children.clear();

    std::vector<std::unique_ptr<Component>>& components = actor.components();
    for (std::size_t i = components.size(); i-- > 0;) {
        Component& component = *components[i];
        unqueueStart(&component);
        if (component.started()) component.onDestroy();
        component.actor_ = nullptr;
    }
    components.clear();

    actors_.erase(actor.id());
}

void Scene::clear() {
    for (std::size_t i = roots_.size(); i-- > 0;) teardown(*roots_[i]);
    roots_.clear();
    actors_.clear();
    pendingStart_.clear();
    pendingDestroy_.clear();
}

void Scene::queueStart(Component* component) { pendingStart_.push_back(component); }

void Scene::unqueueStart(Component* component) {
    for (Component*& pending : pendingStart_) {
        if (pending == component) pending = nullptr;
    }
}

void Scene::attachRoot(Actor* actor) { roots_.push_back(actor); }

void Scene::detachRoot(Actor* actor) {
    roots_.erase(std::remove(roots_.begin(), roots_.end(), actor), roots_.end());
}

}
