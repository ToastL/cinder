#include "text/FontSet.hpp"

#include "platform/Assets.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::text {

FontSet FontSet::engineDefault() {
    FontSet set;
    set.set(FontStyle::Regular, Font::load(cinder::platform::enginePath("fonts/Roboto-Regular.ttf")));
    set.set(FontStyle::Bold, Font::load(cinder::platform::enginePath("fonts/Roboto-Bold.ttf")));
    set.set(FontStyle::Mono, Font::load(cinder::platform::enginePath("fonts/RobotoMono-Regular.ttf")));
    return set;
}

void FontSet::set(FontStyle style, std::unique_ptr<Font> font) {
    fonts_[static_cast<std::size_t>(style)] = std::move(font);
}

const Font& FontSet::get(FontStyle style) const {
    if (const auto& font = fonts_[static_cast<std::size_t>(style)]) return *font;
    if (fonts_[0]) return *fonts_[0];
    throw std::logic_error("FontSet has no regular font");
}

}
