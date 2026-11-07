#pragma once

#include "UI/Core/Widget.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <memory>
#include <optional>
#include <string>
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
    std::optional<int> selectedActor() const;

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
    void duplicate(int id);
    void remove(int id);
    void attach(int id, std::optional<int> parent);

    Selection& selection_;
    History& history_;
    cinder::Scene::Scene& scene_;
    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::TreeView> tree_;
    std::string filter_;
};

}
