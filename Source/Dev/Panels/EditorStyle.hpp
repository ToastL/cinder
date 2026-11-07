#pragma once

#include "UI/Core/Attribute.hpp"
#include "UI/Core/Style.hpp"

#include <memory>
#include <string>

namespace cinder::UI { class Widget; }

namespace cinder::Dev::Panels {

cinder::UI::Theme editorTheme();
cinder::UI::Attribute<cinder::UI::Color> themeColor(std::string name);
std::shared_ptr<cinder::UI::Widget> titleBar(std::shared_ptr<cinder::UI::Widget> content,
                                             cinder::UI::Margin padding = cinder::UI::Margin(6.0f, 2.0f));

}
