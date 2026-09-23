#include "ui/core/Style.hpp"

#include "platform/Log.hpp"
#include "ui/core/ElementList.hpp"

#include <set>
#include <string>

namespace cinder::ui {

void Brush::paint(ElementList& list, int layer, const Rect& rect, const PaintStyle& style, float scale) const {
    if (drawAs == DrawAs::None || rect.empty()) return;
    if (drawAs == DrawAs::Image) {
        list.image(layer, rect, list.named(image), style.apply(fill));
        return;
    }
    if (fill.a <= 0.0f && (outline.a <= 0.0f || outlineWidth <= 0.0f)) return;
    list.box(layer, rect, BoxStyle{style.apply(fill), style.apply(outline), outlineWidth * scale, radii * scale});
}

void Theme::missing(std::string_view name) const {
    static std::set<std::string, std::less<>> reported;
    if (reported.insert(std::string(name)).second) {
        cinder::platform::logError("[ui] style \"%.*s\" is missing or has another type\n",
                                   static_cast<int>(name.size()), name.data());
    }
}

}
