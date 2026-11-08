#pragma once

#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"

#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cinder::Reflect { class PropDef; }

namespace cinder::Scene {
class Actor;
class ClassRegistry;
class Component;
class Object;
class SceneComponent;
}

namespace cinder::Serial {

class Archive;

class SceneCodec {
public:
    static constexpr int VERSION = 6;
    static constexpr int OLDEST = 5;

    static std::string save(cinder::Scene::Scene& scene);
    static void load(const std::string& source, cinder::Scene::Scene& scene);
    static void saveToFile(const std::filesystem::path& path, cinder::Scene::Scene& scene);
    static void loadFromFile(const std::filesystem::path& path, cinder::Scene::Scene& scene);

    static const cinder::Scene::Actor* prefab(cinder::Scene::ClassRegistry& types, std::string_view path);
    static std::string savePrefab(cinder::Scene::Scene& scene, const cinder::Scene::Actor& root,
                                  const std::map<int, int>& ids);
    static void revert(cinder::Scene::Scene& scene, const cinder::Scene::Actor& root);

private:
    struct Instance {
        const cinder::Scene::Scene* tmpl = nullptr;
        std::set<int> claimed;
        std::map<int, cinder::Scene::Object*> locals;
        bool reverting = false;
    };

    struct Pending {
        const cinder::Scene::Actor* base = nullptr;
        Instance* scope = nullptr;
        bool templateless = false;
        bool keepPlacement = false;
        std::vector<cinder::Scene::Component*> unclaimed;
        std::vector<cinder::Scene::Component*> order;
        std::vector<cinder::Scene::IdHint> hints;
        std::set<int> claimed;
        std::vector<std::pair<cinder::Scene::Component*, int>> fileIds;
    };

    explicit SceneCodec(cinder::Scene::Scene& scene);

    void walk(Archive& ar);
    void finish();
    void actors(Archive& ar, const std::vector<cinder::Scene::Actor*>& list, cinder::Scene::Actor* parent,
                Instance* within);
    void actor(Archive& ar, cinder::Scene::Actor* existing, std::string_view type, cinder::Scene::Actor* parent,
               Instance* within);
    const cinder::Scene::Scene* templateOf(std::string_view path);
    static const cinder::Scene::Object* counterpart(const Instance* scope, const cinder::Scene::Object& object);
    void enlist(Instance& scope, cinder::Scene::Actor& actor, bool refs);
    int exported(int id) const;
    void components(Archive& ar, cinder::Scene::Actor& owner, cinder::Scene::SceneComponent* parent, Pending& pending);
    void component(Archive& ar, cinder::Scene::Actor& owner, cinder::Scene::Component* existing,
                   std::string_view type, cinder::Scene::SceneComponent* parent, Pending& pending);
    bool usable(int id, const Pending& pending) const;
    void emit(Archive& ar, const std::vector<cinder::Reflect::PropDef>& defs,
              void* target, const void* base, Instance* scope = nullptr);
    void report(std::string_view kind, std::string_view type);
    int savedRef(const cinder::Reflect::PropDef& def, int id) const;
    void resolveRefs();

    struct LoadedRef {
        const cinder::Reflect::PropDef* def = nullptr;
        void* target = nullptr;
        int fileId = 0;
        const Instance* local = nullptr;
    };

    struct Deferred {
        cinder::Scene::Actor* parent = nullptr;
        const cinder::Scene::Actor* source = nullptr;
        Instance* scope = nullptr;
    };

    cinder::Scene::Scene& scene_;
    cinder::Scene::ClassRegistry& types_;
    cinder::Scene::Transform blank_{nullptr};
    float values_[4]{};
    float fallback_[4]{};
    std::set<std::string, std::less<>> reported_;
    std::vector<LoadedRef> loadedRefs_;
    std::map<int, cinder::Scene::Object*> fileIds_;
    std::deque<Instance> instances_;
    std::vector<Deferred> deferred_;
    std::map<std::string, const cinder::Scene::Scene*, std::less<>> opened_;
    const cinder::Scene::Actor* only_ = nullptr;
    const cinder::Scene::Actor* revert_ = nullptr;
    const std::map<int, int>* remap_ = nullptr;
};

}
