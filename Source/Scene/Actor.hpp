#pragma once

#include "Scene/Component.hpp"
#include "Scene/Object.hpp"
#include "Scene/PropValue.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cinder::Scene {

class SceneComponent;
class Transform;

class Actor : public Object {
public:
    Actor();
    ~Actor() override;

    CINDER_ACTOR(Actor, Object) {}

    const std::vector<Component*>& components() const { return components_; }
    SceneComponent* rootComponent() const;
    void setRootComponent(SceneComponent* component);
    void arrange(const std::vector<Component*>& order);
    Transform* transform() const;

    Component* findComponent(std::string_view name) const;

    template <class T>
    T* findComponent() const {
        for (Component* component : components_) {
            if (component->destroyed()) continue;
            if (auto* found = dynamic_cast<T*>(component)) return found;
        }
        return nullptr;
    }

    template <class T>
    T* addComponent(std::string name = {}) {
        return static_cast<T*>(add(std::make_unique<T>(), std::move(name)));
    }

    Component* add(std::unique_ptr<Component> component, std::string name = {});

    Actor* attachParent() const { return parent_; }
    const std::vector<Actor*>& attachedActors() const { return children_; }
    Actor* findAttached(std::string_view name) const;
    bool isAncestorOf(const Actor* other) const;
    void attachTo(Actor* parent);
    bool enabledInHierarchy() const;

    void destroy();

    const std::string& prefab() const { return prefab_; }
    void setPrefab(std::string path) { prefab_ = std::move(path); }
    bool prefabOwned() const { return prefabId() != 0 && prefab_.empty(); }

    const PropRec& attributes() const { return attributes_; }
    const PropValue* attribute(const std::string& name) const;
    void setAttribute(const std::string& name, PropValue value);
    void removeAttribute(const std::string& name);
    void loadAttributes(PropRec values) { attributes_ = std::move(values); }

    virtual void beginPlay() {}
    virtual void tick(float dt) {}
    virtual void endPlay() {}
    virtual void onAttributeChanged(std::string_view name) {}
    virtual void onChildAdded(Actor& child) {}
    virtual void onChildRemoved(Actor& child) {}
    virtual void onComponentAdded(Component& component) {}
    virtual void onTouched(Actor& other) {}
    virtual void onTouchEnded(Actor& other) {}

private:
    friend class Scene;

    std::vector<std::unique_ptr<Component>> owned_;
    std::vector<Component*> components_;
    Actor* parent_ = nullptr;
    std::vector<Actor*> children_;
    PropRec attributes_;
    std::string prefab_;
};

}
