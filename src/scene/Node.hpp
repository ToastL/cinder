#pragma once

#include "reflect/Reflect.hpp"
#include "scene/PropValue.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cinder::scene {

class DrawList;
class Scene;
class Transform;

class Node {
public:
    Node() = default;
    virtual ~Node() = default;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    virtual const cinder::reflect::PropList& propList() const = 0;
    virtual void* propTarget() = 0;
    virtual const void* propTarget() const = 0;

    virtual Transform* transform() { return nullptr; }
    virtual const Transform* transform() const { return nullptr; }

    int id() const { return id_; }
    Scene* scene() const { return scene_; }

    const std::string& name() const { return name_; }
    void setName(std::string name) { name_ = std::move(name); }

    Node* parent() const { return parent_; }
    const std::vector<Node*>& children() const { return children_; }
    Node* findFirstChild(std::string_view name) const;
    bool isAncestorOf(const Node* other) const;
    void setParent(Node* next);

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool value) { enabled_ = value; }
    bool enabledInHierarchy() const;

    bool started() const { return started_; }
    bool destroyed() const { return destroyed_; }
    void destroy();

    const PropRec& attributes() const { return attributes_; }
    const PropValue* attribute(const std::string& name) const;
    void setAttribute(const std::string& name, PropValue value);
    void removeAttribute(const std::string& name);
    void loadAttributes(PropRec values) { attributes_ = std::move(values); }

    virtual void onStart() {}
    virtual void onUpdate(float dt) {}
    virtual void onRender(float alpha, DrawList& draws) {}
    virtual void onDestroy() {}

    CINDER_PROPS(Node, void) { CINDER_PROP(enabled_); }

private:
    friend class Scene;

    Scene* scene_ = nullptr;
    int id_ = 0;
    std::string name_;
    Node* parent_ = nullptr;
    std::vector<Node*> children_;
    PropRec attributes_;
    bool enabled_ = true;
    bool started_ = false;
    bool destroyed_ = false;
};

}

#define CINDER_NODE(Type, Base)                                              \
public:                                                                  \
    const cinder::reflect::PropList& propList() const override {             \
        return cinder::reflect::props<Type>();                               \
    }                                                                    \
    void* propTarget() override { return this; }                         \
    const void* propTarget() const override { return this; }             \
    CINDER_PROPS(Type, Base)
