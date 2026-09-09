#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cinder::scene {

class Actor;
class Component;
class Components;
class DrawList;

class Scene {
public:
    explicit Scene(Components& types);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Components& types() const { return types_; }

    Actor* spawn(const std::string& name) { return spawn(name, nullptr); }
    Actor* spawn(const std::string& name, Actor* parent) { return spawn(nextId_, name, parent); }
    Actor* spawn(int id, const std::string& name, Actor* parent);

    Actor* byId(int id) const;
    Actor* find(const std::string& name) const;

    const std::vector<Actor*>& roots() const { return roots_; }
    int nextId() const { return nextId_; }

    void update(float dt);
    void render(float alpha, DrawList& draws);

    void destroy(Actor* actor);
    void clear();

    void queueStart(Component* component);
    void unqueueStart(Component* component);
    void attachRoot(Actor* actor);
    void detachRoot(Actor* actor);

private:
    void update(Actor& actor, float dt);
    void render(Actor& actor, float alpha, DrawList& draws);
    void startPending();
    void flushDestroy();
    void teardown(Actor& actor);
    Actor* findIn(Actor& actor, const std::string& name) const;

    Components& types_;
    std::vector<Actor*> roots_;
    std::unordered_map<int, std::unique_ptr<Actor>> actors_;
    std::vector<Component*> pendingStart_;
    std::vector<int> pendingDestroy_;
    int nextId_ = 1;
};

}
