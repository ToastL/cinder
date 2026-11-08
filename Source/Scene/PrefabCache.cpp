#include "Scene/PrefabCache.hpp"

#include "Scene/Scene.hpp"

namespace cinder::Scene {

PrefabCache::PrefabCache() = default;

PrefabCache::~PrefabCache() = default;

const PrefabCache::Entry* PrefabCache::find(std::string_view path) const {
    auto found = entries_.find(path);
    return found == entries_.end() ? nullptr : &found->second;
}

const PrefabCache::Entry& PrefabCache::store(std::string path, std::string text, std::unique_ptr<Scene> scene,
                                             const Actor* root) {
    Entry& entry = entries_[std::move(path)];
    entry.text = std::move(text);
    entry.scene = std::move(scene);
    entry.root = root;
    return entry;
}

void PrefabCache::forget(std::string_view path) {
    auto found = entries_.find(path);
    if (found != entries_.end()) entries_.erase(found);
}

void PrefabCache::clear() { entries_.clear(); }

void PrefabCache::end(std::string_view path) {
    auto found = opening_.find(path);
    if (found != opening_.end()) opening_.erase(found);
}

}
