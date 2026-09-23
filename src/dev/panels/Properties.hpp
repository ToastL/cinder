#pragma once

#include "reflect/PropDef.hpp"
#include "ui/core/Widget.hpp"

#include <functional>
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
class Border;
class MenuBuilder;
}

namespace cinder::dev {
class History;
class Selection;
}

namespace cinder::dev::panels {

class Properties {
public:
    using Target = std::function<void*()>;

    Properties(Selection& selection, History& history, cinder::scene::Scene& scene, cinder::ui::Application& app);

    Properties(const Properties&) = delete;
    Properties& operator=(const Properties&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }

    void update();

private:
    cinder::scene::Node* node() const;
    void rebuild();
    void touched();
    std::shared_ptr<cinder::ui::Widget> body(cinder::scene::Node& node);
    std::shared_ptr<cinder::ui::Widget> rows(const cinder::reflect::PropList& defs, Target target);
    std::shared_ptr<cinder::ui::Widget> row(std::string label, std::shared_ptr<cinder::ui::Widget> editor,
                                            std::shared_ptr<cinder::ui::Widget> trailing = nullptr);
    std::shared_ptr<cinder::ui::Widget> editorFor(const cinder::reflect::PropDef& def, Target target);
    std::shared_ptr<cinder::ui::Widget> attributesSection(int id);
    std::shared_ptr<cinder::ui::Widget> attributeEditor(int id, const std::string& name);
    std::shared_ptr<cinder::ui::Widget> addAttributePopup(int id);

    Selection& selection_;
    History& history_;
    cinder::scene::Scene& scene_;
    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::Border> host_;
    std::optional<int> shown_;
    std::vector<std::string> keys_;
    std::string newName_;
    std::string newKind_ = "Number";
};

}
