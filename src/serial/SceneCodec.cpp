#include "serial/SceneCodec.hpp"

#include "platform/Log.hpp"
#include "reflect/Reflect.hpp"
#include "scene/Actor.hpp"
#include "scene/Component.hpp"
#include "scene/Components.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "serial/TextLoad.hpp"
#include "serial/TextSave.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cinder::serial {

using cinder::scene::Actor;
using cinder::scene::Component;
using cinder::scene::PropBag;
using cinder::scene::PropRec;
using cinder::scene::Scene;

SceneCodec::SceneCodec(Scene& scene) : scene_(scene), types_(scene.types()) {}

std::string SceneCodec::save(Scene& scene) {
    TextSave archive;
    SceneCodec(scene).walk(archive, VERSION);
    return archive.text();
}

void SceneCodec::load(const std::string& source, Scene& scene) {
    TextLoad archive = TextLoad::parse(source);

    const int version = archive.integer("version", 0, 0);
    if (version < OLDEST || version > VERSION) {
        throw std::runtime_error("scene version " + std::to_string(version)
                                 + " is not supported (this build writes "
                                 + std::to_string(VERSION) + ")");
    }

    scene.clear();
    SceneCodec(scene).walk(archive, version);
}

void SceneCodec::saveToFile(const std::filesystem::path& path, Scene& scene) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + path.string());
    out << save(scene);
}

void SceneCodec::loadFromFile(const std::filesystem::path& path, Scene& scene) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    load(buffer.str(), scene);
}

void SceneCodec::walk(Archive& ar, int version) {
    ar.integer("version", VERSION, 0);
    actors(ar, scene_.roots(), nullptr, version);
}

void SceneCodec::actors(Archive& ar, const std::vector<Actor*>& list, Actor* parent, int version) {
    const int count = ar.enterArray(parent == nullptr ? "actors" : "children",
                                    static_cast<int>(list.size()));
    for (int i = 0; i < count; ++i) {
        ar.enterItem(i, "actor");
        actor(ar, ar.loading() ? nullptr : list[static_cast<std::size_t>(i)], parent, version);
        ar.leaveItem();
    }
    ar.leaveArray();
}

void SceneCodec::actor(Archive& ar, Actor* actor, Actor* parent, int version) {
    const int id = ar.integer("id", actor == nullptr ? 0 : actor->id(), 0);
    const std::string name = ar.text("name", actor == nullptr ? "Actor" : actor->name(), "Actor");

    Actor* target = ar.loading() ? spawn(id, name, parent) : actor;

    target->setActive(ar.flag("active", target->activeSelf(), true));
    emit(ar, cinder::reflect::props<cinder::scene::Transform>(), &target->transform(), &blank_);
    components(ar, *target, version);
    actors(ar, target->children(), target, version);
}

Actor* SceneCodec::spawn(int id, const std::string& name, Actor* parent) {
    try {
        return scene_.spawn(id, name, parent);
    } catch (const std::runtime_error&) {
        cinder::platform::logError("[serial] duplicate actor id %d\n", id);
        return scene_.spawn(name, parent);
    }
}

void SceneCodec::components(Archive& ar, Actor& actor, int version) {
    auto& list = actor.components();
    const int count = ar.enterArray("components", static_cast<int>(list.size()));

    for (int i = 0; i < count; ++i) {
        Component* existing = ar.loading() ? nullptr : list[static_cast<std::size_t>(i)].get();
        const std::string type = ar.loading()
                ? ar.itemType(i)
                : std::string(types_.nameOf(*existing));

        ar.enterItem(i, type);
        component(ar, actor, existing, type, version);
        ar.leaveItem();
    }
    ar.leaveArray();
}

void SceneCodec::component(Archive& ar, Actor& actor, Component* existing,
                           std::string_view type, int version) {
    const Component* base = types_.fallback(type);
    if (base == nullptr) {
        cinder::platform::logError("[serial] unknown component type: %.*s\n",
                                   static_cast<int>(type.size()), type.data());
        return;
    }

    Component* target = ar.loading() ? actor.add(types_.create(type)) : existing;
    if (target == nullptr) return;

    emit(ar, target->propList(), target->propTarget(), base->propTarget());

    if (auto* holder = dynamic_cast<PropBag*>(target)) {
        const PropRec data = ar.bag("data", ar.loading() ? PropRec{} : holder->readBag());
        if (ar.loading()) holder->writeBag(data);
    }
}

void SceneCodec::emit(Archive& ar, const std::vector<cinder::reflect::PropDef>& defs,
                      void* target, const void* base) {
    for (const cinder::reflect::PropDef& def : defs) {
        const std::string_view key = def.name();

        switch (def.type()) {
            case cinder::reflect::PropType::Bool: {
                const bool value = ar.flag(key, def.readBool(target), def.readBool(base));
                if (ar.loading()) def.writeBool(target, value);
                break;
            }
            case cinder::reflect::PropType::String:
            case cinder::reflect::PropType::Enum: {
                const std::string value = ar.text(key, def.readText(target), def.readText(base));
                if (ar.loading()) def.writeText(target, value);
                break;
            }
            default: {
                def.read(target, values_);
                def.read(base, fallback_);
                ar.vector(key, values_, fallback_, def.arity());
                if (ar.loading()) def.write(target, values_);
                break;
            }
        }
    }
}

}
