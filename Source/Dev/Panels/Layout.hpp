#pragma once

#include "UI/Core/Widget.hpp"

#include <memory>

namespace cinder::UI { class TabManager; }

namespace cinder::Dev::Panels {

class Console;
class ContentBrowser;
class Outliner;
class Details;
class SceneView;
class Toolbar;

std::shared_ptr<cinder::UI::Widget> editorLayout(Toolbar& toolbar, SceneView& scene, Outliner& outliner,
                                                 Details& details, Console& console, ContentBrowser& content,
                                                 cinder::UI::TabManager& tabs);

}
