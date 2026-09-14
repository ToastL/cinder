#pragma once

#include <optional>
#include <string>

namespace cinder::scene {
class Node;
class Scene;
}

namespace cinder::dev {

class Selection;

class Explorer {
public:
    static constexpr const char* TITLE = "Explorer";

    Explorer(Selection& selection, cinder::scene::Scene& scene);

    Explorer(const Explorer&) = delete;
    Explorer& operator=(const Explorer&) = delete;

    void draw();

private:
    enum class Action { None, Insert, Duplicate, Delete, Reparent };

    void row(cinder::scene::Node& node);
    void dragAndDrop(cinder::scene::Node& node);
    void contextMenu(cinder::scene::Node& node);
    void insertMenu(cinder::scene::Node* parent);
    void apply();

    Selection& selection_;
    cinder::scene::Scene& scene_;
    std::string filter_;
    Action action_ = Action::None;
    int target_ = 0;
    std::optional<int> destination_;
    std::string className_;
};

}
