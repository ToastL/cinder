#include "dev/panels/Layout.hpp"

#include "dev/panels/Console.hpp"
#include "dev/panels/SceneView.hpp"
#include "dev/panels/Toolbar.hpp"
#include "ui/docking/TabManager.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Menu.hpp"

namespace cinder::dev::panels {

using namespace cinder::ui;

std::shared_ptr<Widget> editorLayout(Toolbar& toolbar, SceneView& scene, Console& console, TabManager& tabs) {
    tabs.registerTab("Scene", "Scene", [&scene] { return scene.widget(); }, false);
    tabs.registerTab("Console", "Console", [&console] { return console.widget(); });
    std::shared_ptr<Widget> area = tabs.restore(
            TabManager::split(Orientation::Vertical,
                              {TabManager::stack({"Scene"}, 3.0f), TabManager::stack({"Console"}, 1.0f)}));
    toolbar.setWindowMenu([&tabs](MenuBuilder& menu) { tabs.fillWindowMenu(menu); });

    return make<Overlay>()
        + Overlay::slot()
              [make<Border>()
                       .brush([] { return Application::get().theme().get<Brush>("Brush.Background"); })
                       .padding(Margin(0.0f))
                   [make<VerticalBox>()
                    + VerticalBox::slot().autoHeight()[toolbar.widget()]
                    + VerticalBox::slot().fill(1.0f)[area]]]
        + Overlay::slot()[toolbar.prompt()];
}

}
