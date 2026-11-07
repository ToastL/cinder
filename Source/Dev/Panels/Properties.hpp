#pragma once

#include "Reflect/PropDef.hpp"
#include "UI/Core/Widget.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cinder::Scene {
class Node;
class Scene;
}

namespace cinder::UI {
class Application;
class Border;
}

namespace cinder::Dev {
class History;
class Selection;
}

namespace cinder::Dev::Panels {

class Properties {
public:
    using Target = std::function<void*()>;

    Properties(Selection& selection, History& history, cinder::Scene::Scene& scene, cinder::UI::Application& app);

    Properties(const Properties&) = delete;
    Properties& operator=(const Properties&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }

    void update();
    void rebuild();

private:
    cinder::Scene::Node* node() const;
    void touched();
    std::shared_ptr<cinder::UI::Widget> body(cinder::Scene::Node& node);
    std::shared_ptr<cinder::UI::Widget> rows(const cinder::Reflect::PropList& defs, Target target);
    std::shared_ptr<cinder::UI::Widget> row(std::string label, std::shared_ptr<cinder::UI::Widget> editor,
                                            std::shared_ptr<cinder::UI::Widget> trailing = nullptr);
    std::shared_ptr<cinder::UI::Widget> editorFor(const cinder::Reflect::PropDef& def, Target target);
    std::shared_ptr<cinder::UI::Widget> colorEditorFor(const cinder::Reflect::PropDef& def, Target target);

    template <class Write>
    auto writing(Target target, Write write) {
        return [this, target = std::move(target), write](auto&&... args) {
            if (void* at = target()) {
                write(at, std::forward<decltype(args)>(args)...);
                touched();
            }
        };
    }
    std::shared_ptr<cinder::UI::Widget> attributesSection(int id);
    std::shared_ptr<cinder::UI::Widget> attributeEditor(int id, const std::string& name);
    std::shared_ptr<cinder::UI::Widget> addAttributePopup(int id);

    Selection& selection_;
    History& history_;
    cinder::Scene::Scene& scene_;
    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::Border> host_;
    std::optional<int> shown_;
    std::vector<std::string> keys_;
    std::string newName_;
    std::string newKind_ = "Number";
};

}
