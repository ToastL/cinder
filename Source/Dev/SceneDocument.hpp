#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::Scene { class Scene; }

namespace cinder::Dev {

class History;
class Selection;

class SceneDocument {
public:
    static constexpr std::string_view EXTENSION = ".scene";
    static constexpr std::string_view UNTITLED = "Untitled";

    struct Resolved {
        std::filesystem::path path;
        std::string error;
        bool exists = false;
    };

    SceneDocument(cinder::Scene::Scene& scene, History& history, Selection& selection, std::filesystem::path path);

    SceneDocument(const SceneDocument&) = delete;
    SceneDocument& operator=(const SceneDocument&) = delete;

    const std::filesystem::path& path() const { return path_; }
    bool untitled() const { return path_.empty(); }
    std::string displayName() const;

    void setOnSwitched(std::function<void()> callback) { onSwitched_ = std::move(callback); }

    bool open(const std::filesystem::path& path);
    void create();
    bool save();
    bool saveAs(const std::filesystem::path& path);

    void requestOpen(std::filesystem::path path);
    void requestNew();
    bool pending() const { return request_ != Request::None; }
    bool applyPending();

    static std::string nameOf(const std::filesystem::path& path);
    static std::vector<std::string> listScenes();
    static Resolved resolve(std::string_view name);

private:
    enum class Request { None, Open, New };

    void switched();

    cinder::Scene::Scene& scene_;
    History& history_;
    Selection& selection_;
    std::filesystem::path path_;
    std::function<void()> onSwitched_;
    Request request_ = Request::None;
    std::filesystem::path requested_;
};

}
