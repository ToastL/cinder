#pragma once

#include "ui/core/Widget.hpp"
#include "ui/widgets/TreeView.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::scene {
class Node;
class Scene;
}

namespace cinder::ui {
class Application;
class MenuBuilder;
}

namespace cinder::dev {
class History;
class Selection;
}

namespace cinder::dev::panels {

class Explorer {
public:
    Explorer(Selection& selection, History& history, cinder::scene::Scene& scene, cinder::ui::Application& app);

    Explorer(const Explorer&) = delete;
    Explorer& operator=(const Explorer&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::ui::TreeView>& tree() const { return tree_; }

    void update();

private:
    std::vector<cinder::ui::ItemId> roots() const;
    std::vector<cinder::ui::ItemId> childrenOf(cinder::ui::ItemId item) const;
    std::optional<cinder::ui::ItemId> parentOf(cinder::ui::ItemId item) const;
    std::shared_ptr<cinder::ui::Widget> rowFor(cinder::ui::ItemId item);
    std::shared_ptr<cinder::ui::Widget> contextMenu(std::optional<cinder::ui::ItemId> item);
    void addInsertEntries(cinder::ui::MenuBuilder& menu, std::optional<int> parent);
    bool matches(const cinder::scene::Node& node) const;
    void gather(const cinder::scene::Node& node, std::vector<cinder::ui::ItemId>& out) const;

    void insert(const std::string& className, std::optional<int> parent);
    void duplicate(int id);
    void remove(int id);
    void reparent(int id, std::optional<int> parent);

    Selection& selection_;
    History& history_;
    cinder::scene::Scene& scene_;
    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::TreeView> tree_;
    std::string filter_;
};

}
