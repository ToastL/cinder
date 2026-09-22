#include "dev/panels/Layout.hpp"

#include "dev/panels/Console.hpp"
#include "dev/panels/SceneView.hpp"
#include "dev/panels/Toolbar.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Splitter.hpp"

namespace cinder::dev::panels {

using namespace cinder::ui;

std::shared_ptr<Widget> editorLayout(Toolbar& toolbar, SceneView& scene, Console& console) {
    return make<Overlay>()
        + Overlay::slot()
              [make<Border>()
                       .brush([] { return Application::get().theme().get<Brush>("Brush.Background"); })
                       .padding(Margin(0.0f))
                   [make<VerticalBox>()
                    + VerticalBox::slot().autoHeight()[toolbar.widget()]
                    + VerticalBox::slot().fill(1.0f)
                          [make<Splitter>().orientation(Orientation::Vertical)
                           + Splitter::slot().value(3.0f)[scene.widget()]
                           + Splitter::slot().value(1.0f)[console.widget()]]]]
        + Overlay::slot()[toolbar.prompt()];
}

}
