#include "Dev/Panels/Layout.hpp"

#include "Dev/Panels/Console.hpp"
#include "Dev/Panels/Outliner.hpp"
#include "Dev/Panels/Details.hpp"
#include "Dev/Panels/SceneView.hpp"
#include "Dev/Panels/Toolbar.hpp"
#include "UI/Docking/TabManager.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Menu.hpp"

namespace cinder::Dev::Panels {

using namespace cinder::UI;

std::shared_ptr<Widget> editorLayout(Toolbar& toolbar, SceneView& scene, Outliner& outliner, Details& details,
                                     Console& console, TabManager& tabs) {
    tabs.registerTab("Scene", "Scene", [&scene] { return scene.widget(); }, false);
    tabs.registerTab("Outliner", "Outliner", [&outliner] { return outliner.widget(); });
    tabs.registerTab("Details", "Details", [&details] { return details.widget(); });
    tabs.registerTab("Console", "Console", [&console] { return console.widget(); });
    std::shared_ptr<Widget> area = tabs.restore(TabManager::split(
            Orientation::Horizontal,
            {TabManager::stack({"Outliner"}, 1.0f),
             TabManager::split(Orientation::Vertical,
                               {TabManager::stack({"Scene"}, 3.0f), TabManager::stack({"Console"}, 1.0f)}, 4.0f),
             TabManager::stack({"Details"}, 1.4f)}));
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
