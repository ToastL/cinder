#include "Serial/SceneCodec.hpp"

#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Platform/Log.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Attributes.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/PrefabCache.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Ref.hpp"
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
using cinder::Scene::Object;
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

bool linkFree(const Actor& actor) {
    if (dynamic_cast<const cinder::Scene::ActorPlaceholder*>(&actor) != nullptr) return false;
    for (const Component* component : actor.components()) {
        if (dynamic_cast<const cinder::Scene::ComponentPlaceholder*>(component) != nullptr) return false;
    }
    for (const Actor* child : actor.attachedActors()) {
        if (!linkFree(*child)) return false;
    }
    return true;
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
    codec.finish();
}

void SceneCodec::saveToFile(const std::filesystem::path& path, Scene& scene) {
    cinder::Platform::writeTextFile(path, save(scene));
}

void SceneCodec::loadFromFile(const std::filesystem::path& path, Scene& scene) {
    load(cinder::Platform::readTextFile(path), scene);
}

const Actor* SceneCodec::prefab(cinder::Scene::ClassRegistry& types, std::string_view path) {
    cinder::Scene::PrefabCache& cache = types.prefabs();
    const cinder::Scene::PrefabCache::Entry* entry = cache.find(path);
    if (cache.opening(path)) {
        cinder::Platform::logError("[prefab] %.*s contains itself\n", static_cast<int>(path.size()), path.data());
        return nullptr;
    }

    std::string text;
    try {
        text = cinder::Platform::readTextFile(cinder::Platform::contentPath(path));
    } catch (const std::exception& error) {
        if (entry != nullptr) return entry->root;
        cinder::Platform::logError("[prefab] %.*s: %s\n", static_cast<int>(path.size()), path.data(), error.what());
        cache.store(std::string(path), {}, nullptr, nullptr);
        return nullptr;
    }
    if (entry != nullptr && entry->text == text) return entry->root;

    auto scene = std::make_unique<Scene>(types);
    const Actor* root = nullptr;
    cache.begin(std::string(path));
    try {
        load(text, *scene);
        if (scene->roots().empty()) throw std::runtime_error("it holds no actor");
        if (scene->roots().size() > 1) {
            cinder::Platform::logError("[prefab] %.*s holds %zu actors; only the first is the prefab\n",
                                       static_cast<int>(path.size()), path.data(), scene->roots().size());
        }
        root = scene->roots().front();
        if (!linkFree(*root)) throw std::runtime_error("it holds an unknown class");
    } catch (const std::exception& error) {
        cinder::Platform::logError("[prefab] %.*s: %s\n", static_cast<int>(path.size()), path.data(), error.what());
        root = nullptr;
    }
    cache.end(path);
    if (root == nullptr) scene.reset();
    return cache.store(std::string(path), std::move(text), std::move(scene), root).root;
}

std::string SceneCodec::savePrefab(Scene& scene, const Actor& root, const std::map<int, int>& ids) {
    TextSave archive;
    SceneCodec codec(scene);
    codec.only_ = &root;
    codec.remap_ = &ids;
    codec.walk(archive);
    return archive.text();
}

void SceneCodec::revert(Scene& scene, const Actor& root) {
    TextSave archive;
    SceneCodec codec(scene);
    codec.revert_ = &root;
    codec.walk(archive);
    load(archive.text(), scene);
}

void SceneCodec::walk(Archive& ar) {
    ar.integer("version", VERSION, 0);
    if (only_ != nullptr) {
        actors(ar, {const_cast<Actor*>(only_)}, nullptr, nullptr);
    } else {
        actors(ar, scene_.roots(), nullptr, nullptr);
    }
}

void SceneCodec::finish() {
    const std::vector<Deferred> deferred = std::move(deferred_);
    for (const Deferred& entry : deferred) {
        Actor* copy = scene_.instantiate(*entry.source, entry.parent, {}, false);
        if (copy != nullptr) enlist(*entry.scope, *copy, true);
    }
    resolveRefs();
}

const Scene* SceneCodec::templateOf(std::string_view path) {
    auto found = opened_.find(path);
    if (found != opened_.end()) return found->second;
    const Actor* root = prefab(types_, path);
    const Scene* tmpl = root != nullptr ? root->scene() : nullptr;
    opened_.emplace(std::string(path), tmpl);
    return tmpl;
}

const Object* SceneCodec::counterpart(const Instance* scope, const Object& object) {
    if (scope == nullptr || scope->tmpl == nullptr || object.prefabId() == 0) return nullptr;
    return scope->tmpl->byId(object.prefabId());
}

void SceneCodec::enlist(Instance& scope, Actor& actor, bool refs) {
    const auto add = [&](Object& object) {
        if (object.prefabId() == 0) return;
        scope.locals.emplace(object.prefabId(), &object);
        if (!refs) return;
        cinder::Scene::forEachRef(object, [&](const cinder::Reflect::PropDef& def, void* target) {
            loadedRefs_.push_back({&def, target, def.readInt(target), &scope});
        });
    };
    add(actor);
    for (Component* component : actor.components()) add(*component);
    for (Actor* child : actor.attachedActors()) {
        if (child->prefab().empty() && child->prefabId() != 0) enlist(scope, *child, refs);
    }
}

int SceneCodec::exported(int id) const {
    if (remap_ == nullptr) return id;
    auto found = remap_->find(id);
    return found == remap_->end() ? 0 : found->second;
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

void SceneCodec::actors(Archive& ar, const std::vector<Actor*>& list, Actor* parent, Instance* within) {
    std::vector<Actor*> saved;
    if (!ar.loading()) {
        for (Actor* entry : list) {
            if (entry->destroyed() || types_.nameOf(*entry).empty()) continue;
            if (within != nullptr && within->reverting && counterpart(within, *entry) == nullptr) continue;
            saved.push_back(entry);
        }
    }

    const int count = ar.enterArray(parent == nullptr ? "actors" : "children", static_cast<int>(saved.size()));
    for (int i = 0; i < count; ++i) {
        Actor* existing = ar.loading() ? nullptr : saved[static_cast<std::size_t>(i)];
        const std::string type = ar.loading() ? ar.itemType(i) : std::string(types_.nameOf(*existing));

        ar.enterItem(i, type);
        actor(ar, existing, type, parent, within);
        ar.leaveItem();
    }
    ar.leaveArray();
}

void SceneCodec::actor(Archive& ar, Actor* existing, std::string_view type, Actor* parent, Instance* within) {
    using cinder::Scene::ActorPlaceholder;

    const bool loading = ar.loading();
    const bool linking = remap_ == nullptr;
    const auto* classBase = dynamic_cast<const Actor*>(types_.fallback(type));
    auto* held = dynamic_cast<ActorPlaceholder*>(existing);

    std::string link;
    int from = 0;
    if (loading) {
        link = ar.text("prefab", {}, {});
        if (link.empty() && within != nullptr) from = ar.integer("from", 0, 0);
    } else if (linking && held == nullptr) {
        link = existing->prefab();
        if (link.empty() && within != nullptr) from = existing->prefabId();
    }

    Instance* scope = nullptr;
    const Actor* origin = nullptr;
    bool root = false;
    if (!link.empty()) {
        root = true;
        scope = &instances_.emplace_back();
        scope->tmpl = templateOf(link);
        if (scope->tmpl != nullptr && classBase != nullptr) {
            origin = scope->tmpl->roots().front();
        } else {
            scope->tmpl = nullptr;
        }
        if (!loading) {
            scope->reverting = existing == revert_;
            enlist(*scope, *existing, false);
        }
    } else if (from != 0) {
        if (within->tmpl == nullptr) {
            scope = within;
        } else {
            origin = dynamic_cast<const Actor*>(within->tmpl->byId(from));
            if (origin != nullptr && loading && !within->claimed.insert(from).second) origin = nullptr;
            if (origin != nullptr) {
                scope = within;
            } else if (loading) {
                cinder::Platform::logError("[prefab] actor %d is no longer in its prefab, dropped\n", from);
                return;
            } else {
                from = 0;
            }
        }
    }

    const bool kept = loading ? classBase == nullptr || (scope != nullptr && scope->tmpl == nullptr) : held != nullptr;
    if (kept && loading && classBase == nullptr) report("actor", type);
    if (kept) origin = nullptr;
    const bool reverting = scope != nullptr && scope->reverting;

    const int id = ar.integer("id", existing == nullptr ? 0 : exported(existing->id()), NO_ID);
    const std::string_view nameBase = origin != nullptr ? std::string_view(origin->name()) : type;
    const std::string name = ar.text(
            "name", existing == nullptr ? nameBase : std::string_view(existing->name()),
            reverting && !root ? std::string_view(existing->name()) : nameBase);
    if (!kept && !loading) {
        if (root) ar.text("prefab", link, {});
        else if (from != 0) ar.integer("from", from, 0);
    }

    std::unique_ptr<Actor> created;
    std::vector<std::pair<const Component*, Component*>> pairs;
    Actor* target = existing;
    if (kept) {
        cinder::Scene::RawFields fields = ar.raw(held == nullptr ? cinder::Scene::RawFields{} : held->fields());
        if (loading) {
            created = std::make_unique<ActorPlaceholder>(std::string(type), withoutIdentity(std::move(fields)));
        }
    } else if (loading && origin != nullptr) {
        created = scene_.replicate(*origin, pairs);
        if (created != nullptr) {
            created->setPrefabId(origin->id());
            for (const auto& [source, copy] : pairs) copy->setPrefabId(source->id());
            if (root) created->setPrefab(link);
        }
    } else if (loading) {
        created = types_.createActor(type);
    }
    if (created != nullptr) target = created.get();
    if (target == nullptr) return;
    if (loading) target->setName(name);

    Instance* refScope = origin != nullptr ? scope : nullptr;
    if (!kept && !reverting) {
        const Actor* base = origin != nullptr ? origin : classBase;
        emit(ar, target->propList(), target->propTarget(), base->propTarget(), refScope);
    }

    Pending pending;
    pending.base = kept || origin != nullptr ? nullptr : classBase;
    pending.scope = scope;
    pending.templateless = scope != nullptr && (scope->tmpl == nullptr || kept);
    pending.keepPlacement = root;
    std::optional<int> wanted;
    if (loading) {
        pending.unclaimed = target->components();
        if (usable(id, pending)) {
            wanted = id;
            pending.claimed.insert(id);
        }
    }

    components(ar, *target, nullptr, pending);

    PropRec attributes;
    if (!loading && !reverting) {
        for (const auto& [key, value] : target->attributes()) {
            const cinder::Scene::PropValue* inherited = origin != nullptr ? origin->attribute(key) : nullptr;
            if (inherited == nullptr || !cinder::Scene::sameAttribute(*inherited, value)) attributes.emplace(key, value);
        }
    }
    attributes = ar.bag("attributes", attributes);

    if (loading) {
        target->arrange(pending.order);
        PropRec merged = origin != nullptr ? origin->attributes() : PropRec{};
        for (auto& [key, value] : accepted(attributes, id == NO_ID ? 0 : id)) merged.insert_or_assign(key, value);
        target->loadAttributes(std::move(merged));
        target = scene_.insert(std::move(created), parent, wanted, pending.hints);
        if (id != NO_ID) fileIds_.emplace(id, target);
        for (const auto& [component, fileId] : pending.fileIds) fileIds_.emplace(fileId, component);
        if (refScope != nullptr) {
            scope->locals.emplace(target->prefabId(), target);
            for (Component* component : target->components()) {
                if (component->prefabId() != 0) scope->locals.emplace(component->prefabId(), component);
            }
            for (Component* component : pending.unclaimed) {
                cinder::Scene::forEachRef(*component, [&](const cinder::Reflect::PropDef& def, void* field) {
                    loadedRefs_.push_back({&def, field, def.readInt(field), scope});
                });
            }
        }
    }

    Instance* inner = scope;
    if (kept) {
        inner = nullptr;
        if (loading && scope != nullptr) inner = scope->tmpl == nullptr ? scope : &instances_.emplace_back();
    }
    actors(ar, target->attachedActors(), target, inner);

    if (loading && origin != nullptr) {
        for (const Actor* child : origin->attachedActors()) {
            if (child->destroyed() || !scope->claimed.insert(child->id()).second) continue;
            deferred_.push_back({target, child, scope});
        }
    }
}

void SceneCodec::components(Archive& ar, Actor& owner, SceneComponent* parent, Pending& pending) {
    std::vector<Component*> saved;
    if (!ar.loading()) {
        const bool reverting = pending.scope != nullptr && pending.scope->reverting;
        const auto keep = [&](Component* component) {
            if (component->destroyed() || types_.nameOf(*component).empty()) return;
            if (reverting && counterpart(pending.scope, *component) == nullptr) return;
            saved.push_back(component);
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

    const bool loading = ar.loading();
    const auto* classBase = dynamic_cast<const Component*>(types_.fallback(type));
    auto* held = dynamic_cast<ComponentPlaceholder*>(existing);
    Instance* scope = pending.scope;

    int from = 0;
    const Component* archetype = nullptr;
    if (loading) {
        if (scope != nullptr) from = ar.integer("from", 0, 0);
    } else if (scope != nullptr && held == nullptr && existing->prefabId() != 0) {
        archetype = dynamic_cast<const Component*>(counterpart(scope, *existing));
        if (archetype != nullptr || scope->tmpl == nullptr) from = existing->prefabId();
    }

    const bool kept = loading ? classBase == nullptr || (from != 0 && pending.templateless) : held != nullptr;
    if (kept && loading && classBase == nullptr) report("component", type);
    const bool reverting = scope != nullptr && scope->reverting;

    const int id = ar.integer("id", existing == nullptr ? 0 : exported(existing->id()), NO_ID);
    const std::string_view nameBase = archetype != nullptr ? std::string_view(archetype->name()) : type;
    std::string name = ar.text("name", existing == nullptr ? type : std::string_view(existing->name()),
                               reverting ? std::string_view(existing->name()) : nameBase);
    if (!kept && !loading && from != 0) ar.integer("from", from, 0);

    Component* target = existing;
    bool inherited = false;
    if (kept) {
        cinder::Scene::RawFields fields = ar.raw(held == nullptr ? cinder::Scene::RawFields{} : held->fields());
        if (loading) {
            target = owner.add(std::make_unique<ComponentPlaceholder>(std::string(type),
                                                                      withoutIdentity(std::move(fields))), name);
        }
    } else if (loading && scope != nullptr && !pending.templateless) {
        if (from != 0) {
            auto found = std::find_if(pending.unclaimed.begin(), pending.unclaimed.end(),
                                      [&](Component* candidate) { return candidate->prefabId() == from; });
            if (found == pending.unclaimed.end() || types_.nameOf(**found) != type) {
                cinder::Platform::logError("[prefab] component %d is no longer in its prefab, dropped\n", from);
                return;
            }
            target = *found;
            pending.unclaimed.erase(found);
            archetype = dynamic_cast<const Component*>(scope->tmpl->byId(from));
            inherited = true;
            if (ar.text("name", {}, {}).empty()) name = archetype->name();
        } else {
            target = owner.add(types_.createComponent(type), name);
        }
    } else if (loading) {
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
    } else if (scope == nullptr) {
        archetype = scene_.archetype(*existing);
    }
    if (target == nullptr) return;

    if (loading) {
        target->setName(name);
        pending.order.push_back(target);
        if (id != NO_ID) pending.fileIds.emplace_back(target, id);
        if (usable(id, pending)) {
            pending.hints.push_back({target, id});
            pending.claimed.insert(id);
        }
        if (SceneComponent* placed = target->asSceneComponent(); placed != nullptr && !inherited) {
            placed->attachTo(parent);
        }
    }

    SceneComponent* placed = target->asSceneComponent();
    if (!kept) {
        const bool linked = scope != nullptr && archetype != nullptr && from != 0;
        const bool placement = pending.keepPlacement && placed != nullptr && placed == owner.rootComponent();
        if (placed != nullptr && (!reverting || placement)) {
            const SceneComponent* source = archetype != nullptr ? archetype->asSceneComponent() : nullptr;
            emit(ar, cinder::Reflect::props<cinder::Scene::Transform>(), &placed->transform(),
                 source != nullptr ? static_cast<const void*>(&source->transform()) : &blank_);
        }
        if (!reverting) {
            const Component* propBase = archetype != nullptr ? archetype : classBase;
            emit(ar, target->propList(), target->propTarget(), propBase->propTarget(), linked ? scope : nullptr);
        }
    }

    if (placed != nullptr) components(ar, owner, placed, pending);
}

int SceneCodec::savedRef(const cinder::Reflect::PropDef& def, int id) const {
    const cinder::Scene::Object* object = scene_.byId(id);
    if (object == nullptr || object->destroyed() || !def.accepts(object)) return 0;
    return exported(id);
}

void SceneCodec::resolveRefs() {
    for (const LoadedRef& ref : loadedRefs_) {
        const std::map<int, Object*>& ids = ref.local != nullptr ? ref.local->locals : fileIds_;
        auto found = ids.find(ref.fileId);
        const cinder::Scene::Object* object = found == ids.end() ? nullptr : found->second;
        ref.def->writeInt(ref.target, object != nullptr && ref.def->accepts(object) ? object->id() : 0);
    }
    loadedRefs_.clear();
}

void SceneCodec::emit(Archive& ar, const std::vector<cinder::Reflect::PropDef>& defs,
                      void* target, const void* base, Instance* scope) {
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
                if (scope != nullptr) {
                    const int local = def.readInt(base);
                    if (ar.loading()) {
                        const int value = ar.integer(key, 0, NO_ID);
                        if (value == NO_ID) loadedRefs_.push_back({&def, target, local, scope});
                        else loadedRefs_.push_back({&def, target, value, nullptr});
                    } else {
                        auto found = scope->locals.find(local);
                        const int inherited = found == scope->locals.end() ? 0 : savedRef(def, found->second->id());
                        ar.integer(key, savedRef(def, def.readInt(target)), inherited);
                    }
                    break;
                }
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
