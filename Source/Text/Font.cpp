#include "Text/Font.hpp"

#include "Platform/Files.hpp"
#include "Text/Rasterizer.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::Text {

Font::Font(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)), face_(bytes_) {}

std::unique_ptr<Font> Font::load(const std::filesystem::path& path) {
    std::vector<std::uint8_t> bytes = cinder::Platform::readBinaryFile(path);
    try {
        return std::make_unique<Font>(std::move(bytes));
    } catch (const std::runtime_error& error) {
        throw std::runtime_error(path.string() + ": " + error.what());
    }
}

FontMetrics Font::metrics(float pixelSize) const {
    const float scale = pixelSize / unitsPerEm();
    const auto ascent = static_cast<float>(face_.ascender());
    const auto descent = static_cast<float>(face_.descender());
    const auto gap = static_cast<float>(face_.lineGap());
    return FontMetrics{ascent * scale, descent * scale, (ascent - descent + gap) * scale};
}

GlyphBitmap Font::rasterize(std::uint32_t glyph, float pixelSize) const {
    return rasterizeOutline(face_.outline(glyph), pixelSize / unitsPerEm());
}

}
