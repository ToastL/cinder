#pragma once

#include "text/Font.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace cinder::text {

enum class FontStyle { Regular, Bold, Mono };

class FontSet {
public:
    static FontSet engineDefault();

    void set(FontStyle style, std::unique_ptr<Font> font);
    const Font& get(FontStyle style) const;
    bool empty() const { return !fonts_[0]; }

private:
    static constexpr std::size_t COUNT = 3;

    std::array<std::unique_ptr<Font>, COUNT> fonts_;
};

}
