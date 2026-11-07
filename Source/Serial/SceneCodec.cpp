#include "Serial/SceneCodec.hpp"

#include "Platform/Files.hpp"
#include "Platform/Log.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Attributes.hpp"
#include "Scene/Node.hpp"
#include "Scene/NodeTypes.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Serial/TextLoad.hpp"
#include "Serial/TextSave.hpp"

#include <climits>
#include <optional>
#include <stdexcept>

namespace cinder::Serial {

using cinder::Scene::Node;
using cinder::Scene::PropRec;
using cinder::Scene::Scene;

namespace {

constexpr int NO_ID = INT_MIN;

PropRec accepted(const PropRec& values, int node) {
    PropRec out;
    for (const auto& [name, value] : values) {
        if (cinder::Scene::isAttributeName(name) && cinder::Scene::isAttributeValue(value)) {
            out.emplace(name, value);
        } else {
            cinder::Platform::logError(
                    "[serial] node %d: attribute %s dropped, not a number, string, bool or vector\n",
                    node, name.c_str());
        }
    }
    return out;
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
    SceneCodec(scene).walk(archive);
}

void SceneCodec::saveToFile(const std::filesystem::path& path, Scene& scene) {
    cinder::Platform::writeTextFile(path, save(scene));
}

void SceneCodec::loadFromFile(const std::filesystem::path& path, Scene& scene) {
    load(cinder::Platform::readTextFile(path), scene);
}

void SceneCodec::walk(Archive& ar) {
    ar.integer("version", VERSION, 0);
    nodes(ar, scene_.roots(), nullptr);
}

void SceneCodec::nodes(Archive& ar, const std::vector<Node*>& list, Node* parent) {
    std::vector<Node*> saved;
    if (!ar.loading()) {
        for (Node* entry : list) {
            if (!entry->destroyed() && !types_.nameOf(*entry).empty()) saved.push_back(entry);
        }
    }

    const int count = ar.enterArray(parent == nullptr ? "nodes" : "children",
                                    static_cast<int>(saved.size()));
    for (int i = 0; i < count; ++i) {
        Node* existing = ar.loading() ? nullptr : saved[static_cast<std::size_t>(i)];
        const std::string type = ar.loading() ? ar.itemType(i) : std::string(types_.nameOf(*existing));

        ar.enterItem(i, type);
        node(ar, existing, type, parent);
        ar.leaveItem();
    }
    ar.leaveArray();
}

void SceneCodec::node(Archive& ar, Node* existing, std::string_view type, Node* parent) {
    using cinder::Scene::Placeholder;

    const Node* base = types_.fallback(type);
    auto* held = dynamic_cast<Placeholder*>(existing);
    const bool kept = ar.loading() ? base == nullptr : held != nullptr;
    if (kept && ar.loading() && reported_.insert(std::string(type)).second) {
        cinder::Platform::logError("[serial] unknown node class %.*s, kept as a placeholder\n",
                                   static_cast<int>(type.size()), type.data());
    }

    const int id = ar.integer("id", existing == nullptr ? 0 : existing->id(), NO_ID);
    const std::string name = ar.text(
            "name", existing == nullptr ? type : std::string_view(existing->name()), type);

    Node* target = existing;
    if (kept) {
        cinder::Scene::RawFields fields = ar.raw(held == nullptr ? cinder::Scene::RawFields{} : held->fields());
        if (ar.loading()) {
            std::erase_if(fields, [](const auto& field) { return field.first == "id" || field.first == "name"; });
            target = insert(std::make_unique<Placeholder>(std::string(type), std::move(fields)), id, parent);
        }
    } else if (ar.loading()) {
        target = insert(types_.create(type), id, parent);
    }
    if (target == nullptr) return;
    if (ar.loading()) target->setName(name);

    if (!kept) {
        if (cinder::Scene::Transform* transform = target->transform()) {
            emit(ar, cinder::Reflect::props<cinder::Scene::Transform>(), transform, &blank_);
        }
        emit(ar, target->propList(), target->propTarget(), base->propTarget());
    }

    const PropRec attributes = ar.bag("attributes", ar.loading() ? PropRec{} : target->attributes());
    if (ar.loading()) target->loadAttributes(accepted(attributes, target->id()));

    nodes(ar, target->children(), target);
}

Node* SceneCodec::insert(std::unique_ptr<Node> node, int id, Node* parent) {
    std::optional<int> wanted;
    if (id != NO_ID && (id < 1 || id > Scene::MAX_ID)) cinder::Platform::logError("[serial] node id %d is out of range\n", id);
    else if (id != NO_ID && scene_.byId(id) == nullptr) wanted = id;
    else if (id != NO_ID) cinder::Platform::logError("[serial] duplicate node id %d\n", id);
    return scene_.insert(std::move(node), parent, wanted);
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
