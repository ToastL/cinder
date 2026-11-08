#include "Dev/SceneDocument.hpp"

#include "Components/Actors.hpp"
#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Platform/Log.hpp"
#include "Platform/Strings.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Serial/SceneCodec.hpp"

#include <algorithm>
#include <exception>
#include <system_error>
#include <utility>

namespace cinder::Dev {
namespace {

std::filesystem::path contentRoot() { return cinder::Platform::projectRoot() / "Content"; }

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
    cinder::Platform::logInfo("[editor] saved %s\n", path_.string().c_str());
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

std::vector<std::string> SceneDocument::listScenes() {
    std::vector<std::string> scenes;
    const std::filesystem::path root = contentRoot();
    std::error_code error;
    if (!std::filesystem::is_directory(root, error)) return scenes;

    for (std::filesystem::recursive_directory_iterator it(
                 root, std::filesystem::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        if (it->path().extension() != EXTENSION || !it->is_regular_file(error)) continue;
        scenes.push_back(it->path().lexically_relative(root).generic_string());
    }
    std::sort(scenes.begin(), scenes.end());
    return scenes;
}

SceneDocument::Resolved SceneDocument::resolve(std::string_view name) {
    Resolved resolved;
    std::string text(cinder::Platform::trimmed(name));
    if (text.empty()) {
        resolved.error = "Enter a name for the scene";
        return resolved;
    }
    if (text.back() == '/' || text.back() == '\\') {
        resolved.error = "Enter a file name, not a folder";
        return resolved;
    }
    if (std::filesystem::path(text).extension() != EXTENSION) text += EXTENSION;

    try {
        resolved.path = cinder::Platform::contentPath(text);
    } catch (const std::exception&) {
        resolved.error = "The scene must be inside Content/";
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
