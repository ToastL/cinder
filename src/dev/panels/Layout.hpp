#pragma once

#include "ui/core/Widget.hpp"

#include <memory>

namespace cinder::ui { class TabManager; }

namespace cinder::dev::panels {

class Console;
class SceneView;
class Toolbar;

std::shared_ptr<cinder::ui::Widget> editorLayout(Toolbar& toolbar, SceneView& scene, Console& console,
                                                 cinder::ui::TabManager& tabs);

}
