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

namespace cinder::Scene {

class Actor;
class ClassRegistry;
class Component;
class DrawList;
class Object;
class SceneComponent;

enum class Signal { AttributeChanged, ChildAdded, ChildRemoved, ComponentAdded, Touched, TouchEnded, Destroying };

struct SignalArgs {
    Actor* other = nullptr;
    Component* component = nullptr;
    std::string_view name;
};

using SignalHandler = std::function<void(const SignalArgs&)>;
using TimerHandler = std::function<void()>;

struct IdHint {
    const Component* component = nullptr;
    int id = 0;
};

class Scene {
public:
    static constexpr int MAX_ID = std::numeric_limits<int>::max() - 1;

    explicit Scene(ClassRegistry& types);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    ClassRegistry& types() const { return types_; }

    Actor* insert(std::unique_ptr<Actor> actor, Actor* parent = nullptr, std::optional<int> id = std::nullopt,
                  const std::vector<IdHint>& componentIds = {});
    Actor* spawn(std::string_view className, Actor* parent = nullptr);
    Actor* clone(const Actor& source, Actor* parent);
    Actor* instantiate(const Actor& source, Actor* parent, std::string prefab, bool remapRefs = true);
    std::unique_ptr<Actor> replicate(const Actor& source,
                                     std::vector<std::pair<const Component*, Component*>>& pairs) const;
    void unlink(Actor& root);

    template <class T, class... Args>
    T* spawn(Actor* parent = nullptr, Args&&... args) {
        return static_cast<T*>(insert(std::make_unique<T>(std::forward<Args>(args)...), parent));
    }

    Component* addComponent(Actor& actor, std::unique_ptr<Component> component, std::string name = {},
                            SceneComponent* attachTo = nullptr);
    Component* addComponent(Actor& actor, std::string_view className, SceneComponent* attachTo = nullptr);

    Object* byId(int id) const;
    Actor* actor(int id) const;
    Component* component(int id) const;
    Actor* find(std::string_view name) const;
    const std::vector<Actor*>& roots() const { return roots_; }

    bool isDefault(const Component& component) const;
    const Component* archetype(const Component& component) const;

    void update(float dt);
    void render(float alpha, DrawList& draws);

    void destroy(Actor* actor);
    void destroy(Component* component);
    void destroyNow(Actor* actor);
    void destroyNow(Component* component);
    void clear();

    void reattach(Actor& actor, Actor* parent);
    void attributeChanged(Actor& actor, const std::string& name);

    void emit(Object& source, Signal signal, const SignalArgs& args = {});
    int connect(Object& source, Signal signal, Object& owner, SignalHandler handler);
    void disconnect(int connection);

    int after(Object& owner, double seconds, TimerHandler handler);
    int every(Object& owner, double seconds, TimerHandler handler);
    void cancel(int timer);
    double time() const { return time_; }

private:
    enum class Link { Keep, Unlinked, Stamp };

    Actor* cloneTree(const Actor& source, Actor* parent, std::unordered_map<int, Object*>& copies, Link link);
    static void remapRefs(const std::unordered_map<int, Object*>& copies);
    void enroll(Object& object, std::optional<int> id);
    void attach(Actor& actor, Actor* parent);
    void detach(Actor& actor);
    void update(Actor& actor, float dt);
    void snapshot(Actor& actor);
    void render(Actor& actor, float alpha, DrawList& draws);
    void startPending();
    void start(Object& object);
    void flushDestroy();
    void markDestroyed(Actor& actor);
    void markDestroyed(Component& component);
    void teardown(Actor& actor, bool clearing);
    void remove(Component& component);
    void unqueueStart(const Object* object);
    void hook(Object& object, Signal signal, const SignalArgs& args);
    int schedule(Object& owner, double seconds, bool repeat, TimerHandler handler);
    void runTimers(double dt);
    void dropHandlers(int object);

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

    ClassRegistry& types_;
    std::vector<Actor*> roots_;
    std::unordered_map<int, std::unique_ptr<Actor>> actors_;
    std::unordered_map<int, Object*> objects_;
    std::vector<Object*> pendingStart_;
    std::vector<int> pendingDestroy_;
    int nextId_ = 1;
    std::vector<Connection> connections_;
    std::vector<Timer> timers_;
    int nextHandle_ = 1;
    double time_ = 0.0;
};

}
