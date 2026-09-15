#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cinder::scene {

class DrawList;
class Node;
class NodeTypes;

class SceneObserver {
public:
    virtual ~SceneObserver() = default;
    virtual void attributeChanged(Node& node, const std::string& name) = 0;
    virtual void childAdded(Node& parent, Node& child) = 0;
    virtual void childRemoved(Node& parent, Node& child) = 0;
    virtual void destroying(Node& node) = 0;
};

class Scene {
public:
    explicit Scene(NodeTypes& types);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    NodeTypes& types() const { return types_; }

    Node* insert(std::unique_ptr<Node> node, Node* parent, std::optional<int> id = std::nullopt);
    Node* create(std::string_view className, Node* parent);
    Node* clone(const Node& source, Node* parent);

    template <class T, class... Args>
    T* create(Node* parent, Args&&... args) {
        return static_cast<T*>(insert(std::make_unique<T>(std::forward<Args>(args)...), parent));
    }

    Node* byId(int id) const;
    Node* find(std::string_view name) const;
    const std::vector<Node*>& roots() const { return roots_; }
    int nextId() const { return nextId_; }

    void update(float dt);
    void render(float alpha, DrawList& draws);

    void destroy(Node* node);
    void destroyNow(Node* node);
    void clear();

    void setObserver(SceneObserver* observer) { observer_ = observer; }
    void reparent(Node& node, Node* next);
    void attributeChanged(Node& node, const std::string& name);

private:
    void attach(Node& node, Node* parent);
    void detach(Node& node);
    void update(Node& node, float dt);
    void render(Node& node, float alpha, DrawList& draws);
    void startPending();
    void flushDestroy();
    void markDestroyed(Node& node);
    void teardown(Node& node, bool notify);
    void unqueueStart(Node* node);

    NodeTypes& types_;
    SceneObserver* observer_ = nullptr;
    std::vector<Node*> roots_;
    std::unordered_map<int, std::unique_ptr<Node>> nodes_;
    std::vector<Node*> pendingStart_;
    std::vector<int> pendingDestroy_;
    int nextId_ = 1;
};

}
