#pragma once

#include "ui/core/Layout.hpp"
#include "ui/core/Widget.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cinder::ui {

class Border;
class MenuBuilder;
class Splitter;
class TabStack;

enum class DockSide : std::uint8_t { Centre, Left, Right, Top, Bottom };

struct TabSpawner {
    std::string id;
    std::string label;
    std::function<std::shared_ptr<Widget>()> build;
    bool closable = true;
};

struct LayoutNode {
    enum class Kind : std::uint8_t { Split, Stack };

    Kind kind = Kind::Stack;
    Orientation orientation = Orientation::Horizontal;
    float weight = 1.0f;
    int id = 0;
    std::vector<std::string> tabs;
    std::string active;
    std::vector<LayoutNode> children;
};

class TabManager {
public:
    using Factory = std::function<std::shared_ptr<Widget>()>;

    static LayoutNode stack(std::vector<std::string> tabs, float weight = 1.0f);
    static LayoutNode split(Orientation orientation, std::vector<LayoutNode> children, float weight = 1.0f);

    void registerTab(std::string id, std::string label, Factory build, bool closable = true);
    std::shared_ptr<Widget> restore(LayoutNode layout);
    std::shared_ptr<Widget> widget() const;

    bool isOpen(const std::string& tab) const;
    bool isActive(const std::string& tab) const;
    void openTab(const std::string& tab);
    void closeTab(const std::string& tab);
    void toggleTab(const std::string& tab);
    void activateTab(const std::string& tab);
    void dock(const std::string& tab, int stackId, DockSide side);
    void setActive(int stackId, const std::string& tab);

    const std::vector<TabSpawner>& spawners() const { return spawners_; }
    const TabSpawner* spawner(const std::string& tab) const;
    std::string labelOf(const std::string& tab) const;
    bool isClosable(const std::string& tab) const;
    std::shared_ptr<Widget> contentFor(const std::string& tab);
    void fillWindowMenu(MenuBuilder& menu);

    const LayoutNode& layout() const { return root_; }
    int stackCount() const;
    std::vector<std::string> tabsIn(int stackId) const;
    const std::shared_ptr<TabStack>& stackWidget(int stackId) const;

private:
    void rebuild();
    void syncWeights();
    std::shared_ptr<Widget> buildNode(LayoutNode& node);
    void number(LayoutNode& node);
    LayoutNode* findNode(LayoutNode& node, int id);
    LayoutNode* stackWith(LayoutNode& node, const std::string& tab);
    const LayoutNode* stackWith(const LayoutNode& node, const std::string& tab) const;
    LayoutNode* primaryStack(LayoutNode& node);
    bool detach(LayoutNode& node, const std::string& tab);
    void collapse(LayoutNode& node);

    std::vector<TabSpawner> spawners_;
    std::map<std::string, std::shared_ptr<Widget>> content_;
    std::map<std::string, int> home_;
    std::map<int, std::shared_ptr<TabStack>> stacks_;
    std::vector<std::pair<int, std::weak_ptr<Splitter>>> splitters_;
    LayoutNode root_;
    std::shared_ptr<Border> area_;
    int nextId_ = 1;
};

}
