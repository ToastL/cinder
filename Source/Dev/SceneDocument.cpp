#include "Dev/SceneDocument.hpp"

#include "Components/Actors.hpp"
#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Platform/Log.hpp"
#include "Platform/Strings.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/PrefabCache.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Serial/Prefab.hpp"
#include "Serial/SceneCodec.hpp"

#include <algorithm>
#include <exception>
#include <initializer_list>
#include <system_error>
#include <utility>

namespace cinder::Dev {
namespace {

std::filesystem::path contentRoot() { return cinder::Platform::projectRoot() / "Content"; }

std::vector<std::string> listFiles(std::initializer_list<std::string_view> extensions) {
    std::vector<std::string> files;
    const std::filesystem::path root = contentRoot();
    std::error_code error;
    if (!std::filesystem::is_directory(root, error)) return files;

    for (std::filesystem::recursive_directory_iterator it(
                 root, std::filesystem::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        const std::string extension = it->path().extension().string();
        if (std::find(extensions.begin(), extensions.end(), extension) == extensions.end()) continue;
        if (!it->is_regular_file(error)) continue;
        files.push_back(it->path().lexically_relative(root).generic_string());
    }
    std::sort(files.begin(), files.end());
    return files;
}

}

SceneDocument::SceneDocument(cinder::Scene::Scene& scene, History& history, Selection& selection,
                             std::filesystem::path path)
    : scene_(scene), history_(history), selection_(selection), path_(std::move(path)) {}

std::string SceneDocument::displayName() const { return untitled() ? std::string(UNTITLED) : nameOf(path_); }

bool SceneDocument::open(const std::filesystem::path& path) {
    try {
        cinder::Serial::SceneCodec::load(cinder::Platform::readTextFile(path), scene_);
    } catch (const std::exception& e) {
        cinder::Platform::logError("[editor] open failed: %s: %s\n", path.string().c_str(), e.what());
        return false;
    }
    path_ = path;
    switched();
    cinder::Platform::logInfo("[editor] opened %s\n", path_.string().c_str());
    return true;
}

void SceneDocument::create() {
    scene_.clear();
    auto* camera = scene_.spawn<cinder::Components::CameraActor>();
    camera->transform()->setPosition(0.0f, 2.0f, 10.0f);
    camera->transform()->setRotation(-0.17f, 0.0f, 0.0f);
    path_.clear();
    switched();
    cinder::Platform::logInfo("[editor] new scene\n");
}

bool SceneDocument::save() {
    if (untitled()) return false;
    return saveAs(path_);
}

bool SceneDocument::saveAs(const std::filesystem::path& path) {
    try {
        history_.save(path);
    } catch (const std::exception& e) {
        cinder::Platform::logError("[editor] save failed: %s\n", e.what());
        return false;
    }
    path_ = path;
    if (path_.extension() == PREFAB_EXTENSION) scene_.types().prefabs().forget(nameOf(path_));
    cinder::Platform::logInfo("[editor] saved %s\n", path_.string().c_str());
    return true;
}

cinder::Scene::Actor* SceneDocument::prefabRoot(int actor, std::string_view verb) const {
    if (!history_.enabled()) return nullptr;
    cinder::Scene::Actor* root = scene_.actor(actor);
    if (root == nullptr || root->prefab().empty()) {
        cinder::Platform::logError("[prefab] cannot %.*s: not a prefab instance\n", static_cast<int>(verb.size()),
                                   verb.data());
        return nullptr;
    }
    return root;
}

bool SceneDocument::applyPrefab(int actor) {
    cinder::Scene::Actor* root = prefabRoot(actor, "apply");
    if (root == nullptr) return false;
    const std::string link = root->prefab();
    const std::string refused = cinder::Serial::Prefab::refusal(scene_, *root, link);
    if (!refused.empty()) {
        cinder::Platform::logError("[prefab] cannot apply to %s: %s\n", link.c_str(), refused.c_str());
        return false;
    }
    if (!cinder::Serial::Prefab::write(scene_, *root, link)) return false;
    history_.rebase();
    cinder::Platform::logInfo("[prefab] applied to %s\n", link.c_str());
    if (!untitled()) save();
    return true;
}

bool SceneDocument::revertPrefab(int actor) {
    cinder::Scene::Actor* root = prefabRoot(actor, "revert");
    if (root == nullptr) return false;
    history_.touch("Revert " + root->name(), actor);
    return cinder::Serial::Prefab::revert(scene_, *root);
}

bool SceneDocument::unpackPrefab(int actor) {
    cinder::Scene::Actor* root = prefabRoot(actor, "unpack");
    if (root == nullptr) return false;
    history_.touch("Unpack " + root->name(), actor);
    scene_.unlink(*root);
    return true;
}

bool SceneDocument::savePrefab(int actor, const std::filesystem::path& path) {
    if (!history_.enabled()) return false;
    cinder::Scene::Actor* source = scene_.actor(actor);
    if (source == nullptr) return false;
    const std::string name = nameOf(path);
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        cinder::Platform::logError("[prefab] %s already exists\n", name.c_str());
        return false;
    }
    const std::string refused = cinder::Serial::Prefab::refusal(scene_, *source, name);
    if (!refused.empty()) {
        cinder::Platform::logError("[prefab] cannot save %s: %s\n", name.c_str(), refused.c_str());
        return false;
    }
    history_.touch("Save " + source->name() + " as Prefab", actor);
    if (!cinder::Serial::Prefab::write(scene_, *source, name)) return false;
    cinder::Platform::logInfo("[prefab] saved %s\n", name.c_str());
    return true;
}

void SceneDocument::requestOpen(std::filesystem::path path) {
    request_ = Request::Open;
    requested_ = std::move(path);
}

void SceneDocument::requestNew() {
    request_ = Request::New;
    requested_.clear();
}

bool SceneDocument::applyPending() {
    const Request request = std::exchange(request_, Request::None);
    switch (request) {
        case Request::None:
            return false;
        case Request::Open:
            return open(std::exchange(requested_, {}));
        case Request::New:
            create();
            return true;
    }
    return false;
}

void SceneDocument::switched() {
    history_.reset();
    selection_.clear();
    if (onSwitched_) onSwitched_();
}

std::string SceneDocument::nameOf(const std::filesystem::path& path) {
    const std::filesystem::path relative = path.lexically_normal().lexically_relative(contentRoot().lexically_normal());
    if (relative.empty() || *relative.begin() == "..") return path.filename().generic_string();
    return relative.generic_string();
}

std::vector<std::string> SceneDocument::listScenes() { return listFiles({EXTENSION, PREFAB_EXTENSION}); }

std::vector<std::string> SceneDocument::listPrefabs() { return listFiles({PREFAB_EXTENSION}); }

SceneDocument::Resolved SceneDocument::resolve(std::string_view name, std::string_view extension) {
    Resolved resolved;
    const std::string kind = extension == PREFAB_EXTENSION ? "prefab" : "scene";
    std::string text(cinder::Platform::trimmed(name));
    if (text.empty()) {
        resolved.error = "Enter a name for the " + kind;
        return resolved;
    }
    if (text.back() == '/' || text.back() == '\\') {
        resolved.error = "Enter a file name, not a folder";
        return resolved;
    }
    const std::filesystem::path typed(text);
    if (typed.extension() != EXTENSION && typed.extension() != PREFAB_EXTENSION) text += extension;

    try {
        resolved.path = cinder::Platform::contentPath(text);
    } catch (const std::exception&) {
        resolved.error = "The " + kind + " must be inside Content/";
        return resolved;
    }

    std::error_code error;
    if (std::filesystem::is_directory(resolved.path, error)) {
        resolved.error = text + " is a folder";
        return resolved;
    }
    resolved.exists = std::filesystem::exists(resolved.path, error);
    return resolved;
}

}
