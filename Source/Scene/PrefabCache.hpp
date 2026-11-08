#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>

namespace cinder::Scene {

class Actor;
class Scene;

class PrefabCache {
public:
    struct Entry {
        std::string text;
        std::unique_ptr<Scene> scene;
        const Actor* root = nullptr;
    };

    PrefabCache();
    ~PrefabCache();

    PrefabCache(const PrefabCache&) = delete;
    PrefabCache& operator=(const PrefabCache&) = delete;

    const Entry* find(std::string_view path) const;
    const Entry& store(std::string path, std::string text, std::unique_ptr<Scene> scene, const Actor* root);
    void forget(std::string_view path);
    void clear();

    bool opening(std::string_view path) const { return opening_.count(path) != 0; }
    void begin(std::string path) { opening_.insert(std::move(path)); }
    void end(std::string_view path);

private:
    std::map<std::string, Entry, std::less<>> entries_;
    std::set<std::string, std::less<>> opening_;
};

}
