#pragma once

#include "UI/Core/Widget.hpp"

#include <memory>

namespace cinder::UI { class TabManager; }

namespace cinder::Dev::Panels {

class Console;
class Explorer;
class Properties;
class SceneView;
class Toolbar;

std::shared_ptr<cinder::UI::Widget> editorLayout(Toolbar& toolbar, SceneView& scene, Explorer& explorer,
                                                 Properties& properties, Console& console,
                                                 cinder::UI::TabManager& tabs);

}
