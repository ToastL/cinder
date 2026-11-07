#pragma once

#include "ui/core/Attribute.hpp"
#include "ui/core/Style.hpp"

#include <memory>
#include <string>

namespace cinder::ui { class Widget; }

namespace cinder::dev::panels {

cinder::ui::Theme editorTheme();
cinder::ui::Attribute<cinder::ui::Color> themeColor(std::string name);
std::shared_ptr<cinder::ui::Widget> titleBar(std::shared_ptr<cinder::ui::Widget> content,
                                             cinder::ui::Margin padding = cinder::ui::Margin(6.0f, 2.0f));

}
