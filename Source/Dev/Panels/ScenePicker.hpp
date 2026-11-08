#pragma once

#include "Dev/SceneDocument.hpp"
#include "UI/Core/Args.hpp"
#include "UI/Core/CompoundWidget.hpp"
#include "UI/Core/Delegates.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::UI {
class TextBox;
class TreeView;
}

namespace cinder::Dev::Panels {

class OpenSceneDialog : public cinder::UI::CompoundWidget {
public:
    using OnOpen = std::function<void(const std::string&)>;

    static constexpr float LIST_HEIGHT = 240.0f;

    struct Args : ::cinder::UI::Args<Args, OpenSceneDialog> {
        UI_ATTR(std::string, current)
        UI_EVENT(OnOpen, onOpen)
        UI_EVENT(cinder::UI::OnVoid, onCancel)
    };

    void construct(const Args& args);

    void show(std::vector<std::string> scenes);
    std::shared_ptr<cinder::UI::Widget> focusTarget() const;
    const std::vector<std::string>& matches() const { return matches_; }
    std::optional<std::string> chosen() const;
    const std::shared_ptr<cinder::UI::TreeView>& list() const { return list_; }

private:
    void filter(const std::string& text);
    void move(int delta);
    void submit();

    cinder::UI::Attribute<std::string> current_;
    OnOpen onOpen_;
    cinder::UI::OnVoid onCancel_;
    std::shared_ptr<cinder::UI::TextBox> field_;
    std::shared_ptr<cinder::UI::TreeView> list_;
    std::vector<std::string> scenes_;
    std::vector<std::string> matches_;
    std::optional<std::size_t> selected_;
};

class SaveSceneDialog : public cinder::UI::CompoundWidget {
public:
    using OnSave = std::function<void(const std::filesystem::path&)>;

    struct Args : ::cinder::UI::Args<Args, SaveSceneDialog> {
        UI_ARG(std::string, title, "Save Scene As")
        UI_ARG(std::string, hint, "Scenes/name.scene")
        UI_ARG(std::string, extension, std::string(SceneDocument::EXTENSION))
        UI_ARG(bool, refuseExisting, false)
        UI_EVENT(OnSave, onSave)
        UI_EVENT(cinder::UI::OnVoid, onCancel)
    };

    void construct(const Args& args);

    void show(const std::string& name);
    std::shared_ptr<cinder::UI::Widget> focusTarget() const;
    const SceneDocument::Resolved& resolved() const { return resolved_; }
    std::string text() const;

private:
    void typed(const std::string& text);
    void submit();

    OnSave onSave_;
    cinder::UI::OnVoid onCancel_;
    std::shared_ptr<cinder::UI::TextBox> field_;
    SceneDocument::Resolved resolved_;
    std::string extension_;
    bool refuseExisting_ = false;
};

}
