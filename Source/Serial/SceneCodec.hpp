#pragma once

#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"

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
    static constexpr int VERSION = 5;
    static constexpr int OLDEST = 5;

    static std::string save(cinder::Scene::Scene& scene);
    static void load(const std::string& source, cinder::Scene::Scene& scene);
    static void saveToFile(const std::filesystem::path& path, cinder::Scene::Scene& scene);
    static void loadFromFile(const std::filesystem::path& path, cinder::Scene::Scene& scene);

private:
    struct Pending {
        const cinder::Scene::Actor* base = nullptr;
        std::vector<cinder::Scene::Component*> unclaimed;
        std::vector<cinder::Scene::Component*> order;
        std::vector<cinder::Scene::IdHint> hints;
        std::set<int> claimed;
        std::vector<std::pair<cinder::Scene::Component*, int>> fileIds;
    };

    explicit SceneCodec(cinder::Scene::Scene& scene);

    void walk(Archive& ar);
    void actors(Archive& ar, const std::vector<cinder::Scene::Actor*>& list, cinder::Scene::Actor* parent);
    void actor(Archive& ar, cinder::Scene::Actor* existing, std::string_view type, cinder::Scene::Actor* parent);
    void components(Archive& ar, cinder::Scene::Actor& owner, cinder::Scene::SceneComponent* parent, Pending& pending);
    void component(Archive& ar, cinder::Scene::Actor& owner, cinder::Scene::Component* existing,
                   std::string_view type, cinder::Scene::SceneComponent* parent, Pending& pending);
    bool usable(int id, const Pending& pending) const;
    void emit(Archive& ar, const std::vector<cinder::Reflect::PropDef>& defs,
              void* target, const void* base);
    void report(std::string_view kind, std::string_view type);
    int savedRef(const cinder::Reflect::PropDef& def, int id) const;
    void resolveRefs();

    struct LoadedRef {
        const cinder::Reflect::PropDef* def = nullptr;
        void* target = nullptr;
        int fileId = 0;
    };

    cinder::Scene::Scene& scene_;
    cinder::Scene::ClassRegistry& types_;
    cinder::Scene::Transform blank_{nullptr};
    float values_[4]{};
    float fallback_[4]{};
    std::set<std::string, std::less<>> reported_;
    std::vector<LoadedRef> loadedRefs_;
    std::map<int, cinder::Scene::Object*> fileIds_;
};

}
