#pragma once

#include <functional>
#include <limits>
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

enum class Signal { AttributeChanged, ChildAdded, ChildRemoved, Touched, TouchEnded, Destroying };

struct SignalArgs {
    Node* other = nullptr;
    std::string_view name;
};

using SignalHandler = std::function<void(const SignalArgs&)>;
using TimerHandler = std::function<void()>;

class Scene {
public:
    static constexpr int MAX_ID = std::numeric_limits<int>::max() - 1;

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

    void reparent(Node& node, Node* next);
    void attributeChanged(Node& node, const std::string& name);

    void emit(Node& source, Signal signal, const SignalArgs& args = {});
    int connect(Node& source, Signal signal, Node& owner, SignalHandler handler);
    void disconnect(int connection);

    int after(Node& owner, double seconds, TimerHandler handler);
    int every(Node& owner, double seconds, TimerHandler handler);
    void cancel(int timer);
    double time() const { return time_; }

private:
    void attach(Node& node, Node* parent);
    void detach(Node& node);
    void update(Node& node, float dt);
    void render(Node& node, float alpha, DrawList& draws);
    void startPending();
    void flushDestroy();
    void markDestroyed(Node& node);
    void teardown(Node& node, bool clearing);
    void unqueueStart(Node* node);
    void hook(Node& node, Signal signal, const SignalArgs& args);
    int schedule(Node& owner, double seconds, bool repeat, TimerHandler handler);
    void runTimers(double dt);
    void dropHandlers(int node);

    struct Connection {
        int id;
        int source;
        Signal signal;
        int owner;
        SignalHandler handler;
    };

    struct Timer {
        int id;
        int owner;
        double due;
        double interval;
        bool repeat;
        TimerHandler handler;
    };

    NodeTypes& types_;
    std::vector<Node*> roots_;
    std::unordered_map<int, std::unique_ptr<Node>> nodes_;
    std::vector<Node*> pendingStart_;
    std::vector<int> pendingDestroy_;
    int nextId_ = 1;
    std::vector<Connection> connections_;
    std::vector<Timer> timers_;
    int nextHandle_ = 1;
    double time_ = 0.0;
};

}
