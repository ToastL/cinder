#include "Serial/Prefab.hpp"

#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Platform/Log.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/PrefabCache.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Transform.hpp"
#include "Serial/SceneCodec.hpp"

#include <algorithm>
#include <exception>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace cinder::Serial {

using cinder::Scene::Actor;
using cinder::Scene::Component;
using cinder::Scene::Object;
using cinder::Scene::Scene;

namespace {

template <class Fn>
void eachObject(Actor& actor, Fn&& fn) {
    if (actor.destroyed()) return;
    fn(static_cast<Object&>(actor));
    for (Component* component : actor.components()) {
        if (!component->destroyed()) fn(static_cast<Object&>(*component));
    }
    for (Actor* child : actor.attachedActors()) eachObject(*child, fn);
}

bool holdsLink(const Actor& actor, bool self) {
    if (self && !actor.prefab().empty()) return true;
    for (const Actor* child : actor.attachedActors()) {
        if (!child->destroyed() && holdsLink(*child, true)) return true;
    }
    return false;
}

bool holdsPlaceholder(Actor& actor) {
    bool found = false;
    eachObject(actor, [&](Object& object) {
        if (dynamic_cast<cinder::Scene::ActorPlaceholder*>(&object) != nullptr
            || dynamic_cast<cinder::Scene::ComponentPlaceholder*>(&object) != nullptr) {
            found = true;
        }
    });
    return found;
}

const Actor* cached(const Scene& scene, std::string_view path) {
    const cinder::Scene::PrefabCache::Entry* entry = scene.types().prefabs().find(path);
    return entry != nullptr ? entry->root : nullptr;
}

}

Actor* Prefab::instantiate(Scene& scene, std::string_view path, Actor* parent) {
    const Actor* source = cached(scene, path);
    if (source == nullptr) source = SceneCodec::prefab(scene.types(), path);
    if (source == nullptr) return nullptr;
    return scene.instantiate(*source, parent, std::string(path));
}

const Actor* Prefab::rootOf(const Object& object) {
    const Actor* actor = dynamic_cast<const Actor*>(&object);
    if (const auto* component = dynamic_cast<const Component*>(&object)) actor = component->owner();
    for (; actor != nullptr; actor = actor->attachParent()) {
        if (!actor->prefab().empty()) return actor;
        if (actor->prefabId() == 0) return nullptr;
    }
    return nullptr;
}

const Object* Prefab::baseline(const Scene& scene, const Object& object) {
    if (object.prefabId() == 0) return nullptr;
    const Actor* root = rootOf(object);
    if (root == nullptr) return nullptr;
    const Actor* source = cached(scene, root->prefab());
    return source != nullptr ? source->scene()->byId(object.prefabId()) : nullptr;
}

bool Prefab::missing(const Scene& scene, const Actor& root) { return cached(scene, root.prefab()) == nullptr; }

std::string Prefab::refusal(Scene& scene, const Actor& actor, std::string_view path) {
    const bool applying = !actor.prefab().empty() && actor.prefab() == path;
    if (!applying && actor.prefabId() != 0) {
        return "it is part of a prefab instance; unpack the instance first";
    }
    if (holdsLink(actor, !applying)) return "nested prefabs are not supported yet";
    if (holdsPlaceholder(const_cast<Actor&>(actor))) return "it holds an unknown class";
    if (applying) {
        const Actor* source = SceneCodec::prefab(scene.types(), path);
        if (source == nullptr) return "the prefab file cannot be read";
        if (holdsLink(*source, true)) return "the prefab holds another prefab";
    }
    return {};
}

bool Prefab::write(Scene& scene, Actor& actor, std::string_view path) {
    const bool applying = actor.prefab() == path;
    const Actor* source = applying ? SceneCodec::prefab(scene.types(), path) : nullptr;
    if (applying && source == nullptr) return false;

    int next = 0;
    if (source != nullptr) {
        eachObject(const_cast<Actor&>(*source), [&](Object& object) { next = std::max(next, object.id()); });
    }
    std::map<int, int> ids;
    std::vector<std::pair<int, int>> previous;
    eachObject(actor, [&](Object& object) {
        previous.emplace_back(object.id(), object.prefabId());
        const bool inherited = source != nullptr && object.prefabId() != 0
                               && source->scene()->byId(object.prefabId()) != nullptr;
        ids.emplace(object.id(), inherited ? object.prefabId() : ++next);
    });

    const std::string name = actor.name();
    cinder::Scene::SceneComponent* placed = actor.rootComponent();
    cinder::Scene::Transform held{nullptr};
    const auto& transformProps = cinder::Reflect::props<cinder::Scene::Transform>();
    if (placed != nullptr) {
        cinder::Reflect::copyProps(transformProps, &placed->transform(), &held);
        if (source != nullptr) {
            actor.setName(source->name());
            if (const cinder::Scene::SceneComponent* from = source->rootComponent()) {
                cinder::Reflect::copyProps(transformProps, &from->transform(), &placed->transform());
            }
        } else {
            placed->transform().setPosition(0.0f, 0.0f, 0.0f);
        }
    } else if (source != nullptr) {
        actor.setName(source->name());
    }
    const std::string text = SceneCodec::savePrefab(scene, actor, ids);
    actor.setName(name);
    if (placed != nullptr) cinder::Reflect::copyProps(transformProps, &held, &placed->transform());

    const int rootId = actor.id();
    const std::string link = actor.prefab();
    scene.unlink(actor);
    const std::string snapshot = SceneCodec::save(scene);

    try {
        const std::filesystem::path file = cinder::Platform::contentPath(path);
        std::filesystem::create_directories(file.parent_path());
        cinder::Platform::writeTextFile(file, text);
    } catch (const std::exception& error) {
        cinder::Platform::logError("[prefab] %.*s: %s\n", static_cast<int>(path.size()), path.data(), error.what());
        for (const auto& [id, prefabId] : previous) {
            if (Object* object = scene.byId(id)) object->setPrefabId(prefabId);
        }
        actor.setPrefab(link);
        return false;
    }

    scene.types().prefabs().forget(path);
    SceneCodec::load(snapshot, scene);
    for (const auto& [id, prefabId] : ids) {
        if (Object* object = scene.byId(id)) object->setPrefabId(prefabId);
    }
    if (Actor* root = scene.actor(rootId)) root->setPrefab(std::string(path));
    return true;
}

bool Prefab::revert(Scene& scene, Actor& root) {
    if (root.prefab().empty() || SceneCodec::prefab(scene.types(), root.prefab()) == nullptr) return false;
    SceneCodec::revert(scene, root);
    return true;
}

}
