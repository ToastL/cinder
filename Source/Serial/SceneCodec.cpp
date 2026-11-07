#include "Serial/SceneCodec.hpp"

#include "Platform/Files.hpp"
#include "Platform/Log.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Attributes.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Serial/TextLoad.hpp"
#include "Serial/TextSave.hpp"

#include <algorithm>
#include <climits>
#include <optional>
#include <stdexcept>

namespace cinder::Serial {

using cinder::Scene::Actor;
using cinder::Scene::Component;
using cinder::Scene::PropRec;
using cinder::Scene::Scene;
using cinder::Scene::SceneComponent;

namespace {

constexpr int NO_ID = INT_MIN;

PropRec accepted(const PropRec& values, int actor) {
    PropRec out;
    for (const auto& [name, value] : values) {
        if (cinder::Scene::isAttributeName(name) && cinder::Scene::isAttributeValue(value)) {
            out.emplace(name, value);
        } else {
            cinder::Platform::logError(
                    "[serial] actor %d: attribute %s dropped, not a number, string, bool or vector\n",
                    actor, name.c_str());
        }
    }
    return out;
}

cinder::Scene::RawFields withoutIdentity(cinder::Scene::RawFields fields) {
    std::erase_if(fields, [](const auto& field) { return field.first == "id" || field.first == "name"; });
    return fields;
}

}

SceneCodec::SceneCodec(Scene& scene) : scene_(scene), types_(scene.types()) {}

std::string SceneCodec::save(Scene& scene) {
    TextSave archive;
    SceneCodec(scene).walk(archive);
    return archive.text();
}

void SceneCodec::load(const std::string& source, Scene& scene) {
    TextLoad archive = TextLoad::parse(source);

    const int version = archive.integer("version", 0, 0);
    if (version < OLDEST || version > VERSION) {
        throw std::runtime_error("scene version " + std::to_string(version)
                                 + " is not supported (this build reads "
                                 + std::to_string(OLDEST) + " to " + std::to_string(VERSION) + ")");
    }

    scene.clear();
    SceneCodec codec(scene);
    codec.walk(archive);
    codec.resolveRefs();
}

void SceneCodec::saveToFile(const std::filesystem::path& path, Scene& scene) {
    cinder::Platform::writeTextFile(path, save(scene));
}

void SceneCodec::loadFromFile(const std::filesystem::path& path, Scene& scene) {
    load(cinder::Platform::readTextFile(path), scene);
}

void SceneCodec::walk(Archive& ar) {
    ar.integer("version", VERSION, 0);
    actors(ar, scene_.roots(), nullptr);
}

void SceneCodec::report(std::string_view kind, std::string_view type) {
    if (!reported_.insert(std::string(type)).second) return;
    cinder::Platform::logError("[serial] unknown %.*s class %.*s, kept as a placeholder\n",
                               static_cast<int>(kind.size()), kind.data(),
                               static_cast<int>(type.size()), type.data());
}

bool SceneCodec::usable(int id, const Pending& pending) const {
    if (id == NO_ID) return false;
    if (id < 1 || id > Scene::MAX_ID) {
        cinder::Platform::logError("[serial] id %d is out of range\n", id);
        return false;
    }
    if (scene_.byId(id) != nullptr || pending.claimed.count(id) != 0) {
        cinder::Platform::logError("[serial] duplicate id %d\n", id);
        return false;
    }
    return true;
}

void SceneCodec::actors(Archive& ar, const std::vector<Actor*>& list, Actor* parent) {
    std::vector<Actor*> saved;
    if (!ar.loading()) {
        for (Actor* entry : list) {
            if (!entry->destroyed() && !types_.nameOf(*entry).empty()) saved.push_back(entry);
        }
    }

    const int count = ar.enterArray(parent == nullptr ? "actors" : "children", static_cast<int>(saved.size()));
    for (int i = 0; i < count; ++i) {
        Actor* existing = ar.loading() ? nullptr : saved[static_cast<std::size_t>(i)];
        const std::string type = ar.loading() ? ar.itemType(i) : std::string(types_.nameOf(*existing));

        ar.enterItem(i, type);
        actor(ar, existing, type, parent);
        ar.leaveItem();
    }
    ar.leaveArray();
}

void SceneCodec::actor(Archive& ar, Actor* existing, std::string_view type, Actor* parent) {
    using cinder::Scene::ActorPlaceholder;

    const auto* base = dynamic_cast<const Actor*>(types_.fallback(type));
    auto* held = dynamic_cast<ActorPlaceholder*>(existing);
    const bool kept = ar.loading() ? base == nullptr : held != nullptr;
    if (kept && ar.loading()) report("actor", type);

    const int id = ar.integer("id", existing == nullptr ? 0 : existing->id(), NO_ID);
    const std::string name = ar.text(
            "name", existing == nullptr ? type : std::string_view(existing->name()), type);

    std::unique_ptr<Actor> created;
    Actor* target = existing;
    if (kept) {
        cinder::Scene::RawFields fields = ar.raw(held == nullptr ? cinder::Scene::RawFields{} : held->fields());
        if (ar.loading()) {
            created = std::make_unique<ActorPlaceholder>(std::string(type), withoutIdentity(std::move(fields)));
        }
    } else if (ar.loading()) {
        created = types_.createActor(type);
    }
    if (created != nullptr) target = created.get();
    if (target == nullptr) return;
    if (ar.loading()) target->setName(name);

    if (!kept) emit(ar, target->propList(), target->propTarget(), base->propTarget());

    Pending pending;
    pending.base = kept ? nullptr : base;
    std::optional<int> wanted;
    if (ar.loading()) {
        pending.unclaimed = target->components();
        if (usable(id, pending)) {
            wanted = id;
            pending.claimed.insert(id);
        }
    }

    components(ar, *target, nullptr, pending);

    const PropRec attributes = ar.bag("attributes", ar.loading() ? PropRec{} : target->attributes());

    if (ar.loading()) {
        target->arrange(pending.order);
        target->loadAttributes(accepted(attributes, id == NO_ID ? 0 : id));
        target = scene_.insert(std::move(created), parent, wanted, pending.hints);
        if (id != NO_ID) fileIds_.emplace(id, target);
        for (const auto& [component, fileId] : pending.fileIds) fileIds_.emplace(fileId, component);
    }

    actors(ar, target->attachedActors(), target);
}

void SceneCodec::components(Archive& ar, Actor& owner, SceneComponent* parent, Pending& pending) {
    std::vector<Component*> saved;
    if (!ar.loading()) {
        const auto keep = [&](Component* component) {
            if (!component->destroyed() && !types_.nameOf(*component).empty()) saved.push_back(component);
        };
        if (parent == nullptr) {
            for (Component* component : owner.components()) {
                const SceneComponent* placed = component->asSceneComponent();
                if (placed == nullptr || placed->attachParent() == nullptr) keep(component);
            }
        } else {
            for (SceneComponent* child : parent->attachChildren()) keep(child);
        }
    }

    const int count = ar.enterArray(parent == nullptr ? "components" : "attached", static_cast<int>(saved.size()));
    for (int i = 0; i < count; ++i) {
        Component* existing = ar.loading() ? nullptr : saved[static_cast<std::size_t>(i)];
        const std::string type = ar.loading() ? ar.itemType(i) : std::string(types_.nameOf(*existing));

        ar.enterItem(i, type);
        component(ar, owner, existing, type, parent, pending);
        ar.leaveItem();
    }
    ar.leaveArray();
}

void SceneCodec::component(Archive& ar, Actor& owner, Component* existing, std::string_view type,
                           SceneComponent* parent, Pending& pending) {
    using cinder::Scene::ComponentPlaceholder;

    const auto* classBase = dynamic_cast<const Component*>(types_.fallback(type));
    auto* held = dynamic_cast<ComponentPlaceholder*>(existing);
    const bool kept = ar.loading() ? classBase == nullptr : held != nullptr;
    if (kept && ar.loading()) report("component", type);

    const int id = ar.integer("id", existing == nullptr ? 0 : existing->id(), NO_ID);
    const std::string name = ar.text(
            "name", existing == nullptr ? type : std::string_view(existing->name()), type);

    Component* target = existing;
    const Component* archetype = nullptr;
    if (kept) {
        cinder::Scene::RawFields fields = ar.raw(held == nullptr ? cinder::Scene::RawFields{} : held->fields());
        if (ar.loading()) {
            target = owner.add(std::make_unique<ComponentPlaceholder>(std::string(type),
                                                                      withoutIdentity(std::move(fields))), name);
        }
    } else if (ar.loading()) {
        auto found = std::find_if(pending.unclaimed.begin(), pending.unclaimed.end(), [&](Component* candidate) {
            return types_.displayName(*candidate) == name && types_.nameOf(*candidate) == type;
        });
        if (found != pending.unclaimed.end()) {
            target = *found;
            pending.unclaimed.erase(found);
            if (pending.base != nullptr) {
                for (const Component* candidate : pending.base->components()) {
                    if (types_.displayName(*candidate) == name && types_.nameOf(*candidate) == type) {
                        archetype = candidate;
                        break;
                    }
                }
            }
        } else {
            target = owner.add(types_.createComponent(type), name);
        }
    } else {
        archetype = scene_.archetype(*existing);
    }
    if (target == nullptr) return;

    if (ar.loading()) {
        target->setName(name);
        pending.order.push_back(target);
        if (id != NO_ID) pending.fileIds.emplace_back(target, id);
        if (usable(id, pending)) {
            pending.hints.push_back({target, id});
            pending.claimed.insert(id);
        }
        if (SceneComponent* placed = target->asSceneComponent()) placed->attachTo(parent);
    }

    SceneComponent* placed = target->asSceneComponent();
    if (!kept) {
        if (placed != nullptr) {
            const SceneComponent* from = archetype != nullptr ? archetype->asSceneComponent() : nullptr;
            emit(ar, cinder::Reflect::props<cinder::Scene::Transform>(), &placed->transform(),
                 from != nullptr ? static_cast<const void*>(&from->transform()) : &blank_);
        }
        const Component* propBase = archetype != nullptr ? archetype : classBase;
        emit(ar, target->propList(), target->propTarget(), propBase->propTarget());
    }

    if (placed != nullptr) components(ar, owner, placed, pending);
}

int SceneCodec::savedRef(const cinder::Reflect::PropDef& def, int id) const {
    const cinder::Scene::Object* object = scene_.byId(id);
    if (object == nullptr || object->destroyed() || !def.accepts(object)) return 0;
    return id;
}

void SceneCodec::resolveRefs() {
    for (const LoadedRef& ref : loadedRefs_) {
        auto found = fileIds_.find(ref.fileId);
        const cinder::Scene::Object* object = found == fileIds_.end() ? nullptr : found->second;
        ref.def->writeInt(ref.target, object != nullptr && ref.def->accepts(object) ? object->id() : 0);
    }
    loadedRefs_.clear();
}

void SceneCodec::emit(Archive& ar, const std::vector<cinder::Reflect::PropDef>& defs,
                      void* target, const void* base) {
    for (const cinder::Reflect::PropDef& def : defs) {
        const std::string_view key = def.name();

        switch (def.type()) {
            case cinder::Reflect::PropType::Bool: {
                const bool value = ar.flag(key, def.readBool(target), def.readBool(base));
                if (ar.loading()) def.writeBool(target, value);
                break;
            }
            case cinder::Reflect::PropType::Int: {
                const int value = ar.integer(key, def.readInt(target), def.readInt(base));
                if (ar.loading()) def.writeInt(target, value);
                break;
            }
            case cinder::Reflect::PropType::Ref: {
                const int saved = ar.loading() ? 0 : savedRef(def, def.readInt(target));
                const int value = ar.integer(key, saved, 0);
                if (ar.loading()) loadedRefs_.push_back({&def, target, value});
                break;
            }
            case cinder::Reflect::PropType::String:
            case cinder::Reflect::PropType::Enum: {
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
