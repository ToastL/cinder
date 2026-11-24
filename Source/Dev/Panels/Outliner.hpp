#pragma once

#include "Dev/PrefabActions.hpp"
#include "UI/Core/Widget.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cinder::Scene {
class Actor;
class Scene;
}

namespace cinder::UI {
class Application;
class MenuBuilder;
}

namespace cinder::Dev {
class History;
class Selection;
}

namespace cinder::Dev::Panels {

class Outliner {
public:
    Outliner(Selection& selection, History& history, cinder::Scene::Scene& scene, cinder::UI::Application& app);

    Outliner(const Outliner&) = delete;
    Outliner& operator=(const Outliner&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::UI::TreeView>& tree() const { return tree_; }

    void update();
    void sceneChanged() {
        tree_->collapseAll();
        cancelRename();
    }
    void setPrefabActions(PrefabActions actions) { prefabs_ = std::move(actions); }
    std::optional<int> selectedActor() const;

    void beginRename(int id);
    void cancelRename();
    std::optional<int> renaming() const { return renaming_; }

private:
    std::vector<cinder::UI::ItemId> roots() const;
    std::vector<cinder::UI::ItemId> childrenOf(cinder::UI::ItemId item) const;
    std::optional<cinder::UI::ItemId> parentOf(cinder::UI::ItemId item) const;
    std::shared_ptr<cinder::UI::Widget> rowFor(cinder::UI::ItemId item);
    std::shared_ptr<cinder::UI::Widget> contextMenu(std::optional<cinder::UI::ItemId> item);
    void addInsertEntries(cinder::UI::MenuBuilder& menu, std::optional<int> parent);
    bool matches(const cinder::Scene::Actor& actor) const;
    void gather(const cinder::Scene::Actor& actor, std::vector<cinder::UI::ItemId>& out) const;

    void insert(const std::string& className, std::optional<int> parent);
    void place(const std::string& prefab, std::optional<int> parent);
    void duplicate(const std::vector<int>& ids);
    void remove(const std::vector<int>& ids);
    void copy(const std::vector<int>& ids);
    void cut(const std::vector<int>& ids);
    void paste();
    bool clipboardHasActors() const;
    void attach(int id, std::optional<int> parent);
    bool handleRowClick(cinder::UI::ItemId item, const cinder::UI::PointerEvent& event);
    std::vector<int> selectedActors() const;
    std::vector<int> removableSelection() const;

    Selection& selection_;
    History& history_;
    cinder::Scene::Scene& scene_;
    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::TreeView> tree_;
    std::string filter_;
    std::optional<int> renaming_;
    std::weak_ptr<cinder::UI::Widget> renameField_;
    bool focusRename_ = false;
    bool refreshPending_ = false;
    PrefabActions prefabs_;
};

}
