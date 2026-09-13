#pragma once

#include "scene/Component.hpp"
#include "scene/PropValue.hpp"
#include "scene/Transform.hpp"

#include <memory>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace cinder::scene {

class Scene;

class Actor {
public:
    Actor(Scene& scene, int id, std::string name);

    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;

    int id() const { return id_; }
    Scene& scene() const { return scene_; }
    Transform& transform() { return transform_; }
    const Transform& transform() const { return transform_; }

    const std::string& name() const { return name_; }
    void setName(std::string name) { name_ = std::move(name); }

    Actor* parent() const { return parent_; }
    std::vector<Actor*>& children() { return children_; }
    const std::vector<Actor*>& children() const { return children_; }
    std::vector<std::unique_ptr<Component>>& components() { return components_; }
    const std::vector<std::unique_ptr<Component>>& components() const { return components_; }

    bool destroyed() const { return destroyed_; }
    bool activeSelf() const { return active_; }
    void setActive(bool active) { active_ = active; }
    bool active() const;

    const PropRec& attributes() const { return attributes_; }
    const PropValue* attribute(const std::string& name) const;
    void setAttribute(const std::string& name, PropValue value);
    void removeAttribute(const std::string& name);
    void loadAttributes(PropRec values) { attributes_ = std::move(values); }

    Component* add(std::unique_ptr<Component> component);

    template <class T, class... Args>
    T* add(Args&&... args) {
        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = owned.get();
        add(std::move(owned));
        return raw;
    }

    template <class T>
    T* get() const {
        for (const std::unique_ptr<Component>& component : components_) {
            if (T* found = dynamic_cast<T*>(component.get())) return found;
        }
        return nullptr;
    }

    template <class T>
    std::vector<T*> getAll() const {
        std::vector<T*> out;
        for (const std::unique_ptr<Component>& component : components_) {
            if (T* found = dynamic_cast<T*>(component.get())) out.push_back(found);
        }
        return out;
    }

    Component* get(std::type_index type) const;

    void remove(Component* component);
    void setParent(Actor* next);
    Actor* spawnChild(const std::string& name);
    void destroy();

    void markDestroyed();

private:
    bool isAncestorOf(const Actor* other) const;

    Scene& scene_;
    int id_;
    Transform transform_;
    std::vector<std::unique_ptr<Component>> components_;
    std::vector<Actor*> children_;
    PropRec attributes_;

    std::string name_;
    Actor* parent_ = nullptr;
    bool active_ = true;
    bool destroyed_ = false;
};

}
