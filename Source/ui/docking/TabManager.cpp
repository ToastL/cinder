#include "ui/docking/TabManager.hpp"

#include "ui/docking/DockTab.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/Splitter.hpp"

#include <algorithm>

namespace cinder::ui {

namespace {

const std::shared_ptr<TabStack> NO_STACK;

}

LayoutNode TabManager::stack(std::vector<std::string> tabs, float weight) {
    LayoutNode node;
    node.kind = LayoutNode::Kind::Stack;
    node.weight = weight;
    node.active = tabs.empty() ? std::string() : tabs.front();
    node.tabs = std::move(tabs);
    return node;
}

LayoutNode TabManager::split(Orientation orientation, std::vector<LayoutNode> children, float weight) {
    LayoutNode node;
    node.kind = LayoutNode::Kind::Split;
    node.orientation = orientation;
    node.weight = weight;
    node.children = std::move(children);
    return node;
}

void TabManager::registerTab(std::string id, std::string label, Factory build, bool closable) {
    for (TabSpawner& spawner : spawners_) {
        if (spawner.id != id) continue;
        spawner.label = std::move(label);
        spawner.build = std::move(build);
        spawner.closable = closable;
        return;
    }
    spawners_.push_back(TabSpawner{std::move(id), std::move(label), std::move(build), closable});
}

const TabSpawner* TabManager::spawner(const std::string& tab) const {
    for (const TabSpawner& known : spawners_) {
        if (known.id == tab) return &known;
    }
    return nullptr;
}

std::string TabManager::labelOf(const std::string& tab) const {
    const TabSpawner* known = spawner(tab);
    return known != nullptr ? known->label : tab;
}

bool TabManager::isClosable(const std::string& tab) const {
    const TabSpawner* known = spawner(tab);
    return known == nullptr || known->closable;
}

std::shared_ptr<Widget> TabManager::contentFor(const std::string& tab) {
    const auto found = content_.find(tab);
    if (found != content_.end()) return found->second;
    const TabSpawner* known = spawner(tab);
    if (known == nullptr || !known->build) return nullptr;
    std::shared_ptr<Widget> content = known->build();
    content_.emplace(tab, content);
    return content;
}

void TabManager::number(LayoutNode& node) {
    if (node.id == 0) node.id = nextId_++;
    for (LayoutNode& child : node.children) number(child);
}

LayoutNode* TabManager::findNode(LayoutNode& node, int id) {
    if (node.id == id) return &node;
    for (LayoutNode& child : node.children) {
        if (LayoutNode* found = findNode(child, id)) return found;
    }
    return nullptr;
}

LayoutNode* TabManager::stackWith(LayoutNode& node, const std::string& tab) {
    if (node.kind == LayoutNode::Kind::Stack) {
        return std::find(node.tabs.begin(), node.tabs.end(), tab) != node.tabs.end() ? &node : nullptr;
    }
    for (LayoutNode& child : node.children) {
        if (LayoutNode* found = stackWith(child, tab)) return found;
    }
    return nullptr;
}

const LayoutNode* TabManager::stackWith(const LayoutNode& node, const std::string& tab) const {
    if (node.kind == LayoutNode::Kind::Stack) {
        return std::find(node.tabs.begin(), node.tabs.end(), tab) != node.tabs.end() ? &node : nullptr;
    }
    for (const LayoutNode& child : node.children) {
        if (const LayoutNode* found = stackWith(child, tab)) return found;
    }
    return nullptr;
}

LayoutNode* TabManager::primaryStack(LayoutNode& node) {
    if (node.kind == LayoutNode::Kind::Stack) return &node;
    for (LayoutNode& child : node.children) {
        if (LayoutNode* found = primaryStack(child)) return found;
    }
    return nullptr;
}

bool TabManager::detach(LayoutNode& node, const std::string& tab) {
    if (node.kind == LayoutNode::Kind::Stack) {
        const auto found = std::find(node.tabs.begin(), node.tabs.end(), tab);
        if (found == node.tabs.end()) return false;
        home_[tab] = node.id;
        node.tabs.erase(found);
        if (node.active == tab) node.active = node.tabs.empty() ? std::string() : node.tabs.front();
        return true;
    }
    for (LayoutNode& child : node.children) {
        if (detach(child, tab)) return true;
    }
    return false;
}

void TabManager::collapse(LayoutNode& node) {
    if (node.kind != LayoutNode::Kind::Split) return;
    for (LayoutNode& child : node.children) collapse(child);
    std::erase_if(node.children, [](const LayoutNode& child) {
        return child.kind == LayoutNode::Kind::Stack && child.tabs.empty();
    });
    if (node.children.size() == 1) {
        LayoutNode only = std::move(node.children.front());
        only.weight = node.weight;
        node = std::move(only);
    }
}

void TabManager::syncWeights() {
    for (const auto& [id, weak] : splitters_) {
        const std::shared_ptr<Splitter> splitter = weak.lock();
        LayoutNode* node = splitter ? findNode(root_, id) : nullptr;
        if (node == nullptr || node->kind != LayoutNode::Kind::Split) continue;
        for (std::size_t i = 0; i < node->children.size() && i < splitter->slotCount(); ++i) {
            node->children[i].weight = splitter->slotValue(i);
        }
    }
}

std::shared_ptr<Widget> TabManager::buildNode(LayoutNode& node) {
    if (node.kind == LayoutNode::Kind::Stack) {
        auto stack = std::make_shared<TabStack>(*this, node.id, node.tabs, node.active);
        stack->build();
        stacks_.emplace(node.id, stack);
        return stack;
    }
    std::shared_ptr<Splitter> splitter;
    auto args = make<Splitter>().orientation(node.orientation).assign(splitter);
    for (LayoutNode& child : node.children) {
        args + Splitter::slot().value(child.weight)[buildNode(child)];
    }
    std::shared_ptr<Widget> widget = args;
    splitters_.emplace_back(node.id, splitter);
    return widget;
}

std::shared_ptr<Widget> TabManager::restore(LayoutNode layout) {
    root_ = std::move(layout);
    number(root_);
    if (!area_) area_ = make<Border>().padding(Margin(0.0f));
    rebuild();
    return area_;
}

std::shared_ptr<Widget> TabManager::widget() const { return area_; }

void TabManager::rebuild() {
    if (!area_) return;
    syncWeights();
    stacks_.clear();
    splitters_.clear();
    number(root_);
    area_->setContent(buildNode(root_));
}

bool TabManager::isOpen(const std::string& tab) const { return stackWith(root_, tab) != nullptr; }

bool TabManager::isActive(const std::string& tab) const {
    const LayoutNode* node = stackWith(root_, tab);
    return node != nullptr && node->active == tab;
}

void TabManager::openTab(const std::string& tab) {
    if (isOpen(tab) || spawner(tab) == nullptr) return;
    LayoutNode* target = nullptr;
    const auto home = home_.find(tab);
    if (home != home_.end()) {
        LayoutNode* node = findNode(root_, home->second);
        if (node != nullptr && node->kind == LayoutNode::Kind::Stack) target = node;
    }
    if (target == nullptr) target = primaryStack(root_);
    if (target == nullptr) return;
    target->tabs.push_back(tab);
    target->active = tab;
    rebuild();
}

void TabManager::closeTab(const std::string& tab) {
    if (!detach(root_, tab)) return;
    collapse(root_);
    rebuild();
}

void TabManager::toggleTab(const std::string& tab) {
    if (isOpen(tab)) closeTab(tab);
    else openTab(tab);
}

void TabManager::activateTab(const std::string& tab) {
    LayoutNode* node = stackWith(root_, tab);
    if (node == nullptr) {
        openTab(tab);
        return;
    }
    if (node->active == tab) return;
    node->active = tab;
    const auto found = stacks_.find(node->id);
    if (found != stacks_.end()) found->second->activate(tab);
    else rebuild();
}

void TabManager::setActive(int stackId, const std::string& tab) {
    LayoutNode* node = findNode(root_, stackId);
    if (node != nullptr && node->kind == LayoutNode::Kind::Stack) node->active = tab;
}

void TabManager::dock(const std::string& tab, int stackId, DockSide side) {
    LayoutNode* target = findNode(root_, stackId);
    if (target == nullptr || target->kind != LayoutNode::Kind::Stack || spawner(tab) == nullptr) return;
    const bool alone = target->tabs.size() == 1 && target->tabs.front() == tab;
    if (alone && side != DockSide::Centre) return;
    detach(root_, tab);
    if (side == DockSide::Centre) {
        target->tabs.push_back(tab);
        target->active = tab;
    } else {
        LayoutNode moved = stack({tab});
        moved.id = nextId_++;
        LayoutNode existing = *target;
        existing.weight = 1.0f;
        moved.weight = 1.0f;
        LayoutNode branch;
        branch.kind = LayoutNode::Kind::Split;
        branch.id = nextId_++;
        branch.weight = target->weight;
        branch.orientation = side == DockSide::Left || side == DockSide::Right ? Orientation::Horizontal
                                                                              : Orientation::Vertical;
        if (side == DockSide::Left || side == DockSide::Top) branch.children = {moved, existing};
        else branch.children = {existing, moved};
        *target = branch;
    }
    collapse(root_);
    rebuild();
}

void TabManager::fillWindowMenu(MenuBuilder& menu) {
    for (const TabSpawner& known : spawners_) {
        const std::string id = known.id;
        menu.check(known.label, [this, id] { toggleTab(id); }, [this, id] { return isOpen(id); })
                .enabledIf([this, id] { return isClosable(id) || !isOpen(id); });
    }
}

int TabManager::stackCount() const { return static_cast<int>(stacks_.size()); }

std::vector<std::string> TabManager::tabsIn(int stackId) const {
    const auto found = stacks_.find(stackId);
    return found == stacks_.end() ? std::vector<std::string>{} : found->second->tabs();
}

int TabManager::stackOf(const std::string& tab) const {
    const LayoutNode* node = stackWith(root_, tab);
    return node != nullptr ? node->id : -1;
}

const std::shared_ptr<TabStack>& TabManager::stackWidget(int stackId) const {
    const auto found = stacks_.find(stackId);
    return found == stacks_.end() ? NO_STACK : found->second;
}

}
