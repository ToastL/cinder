#include "text/Font.hpp"

#include "platform/Files.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace cinder::text {

namespace {

constexpr float FIXED = 64.0f;

void setSize(FT_Face face, float pixelSize) {
    const auto size = static_cast<FT_F26Dot6>(std::lround(pixelSize * FIXED));
    if (FT_Set_Char_Size(face, 0, size, 72, 72) != 0) throw std::runtime_error("FreeType could not set a size");
}

}

Font::Font(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {
    if (FT_Init_FreeType(&library_) != 0) throw std::runtime_error("FreeType failed to initialize");
    if (FT_New_Memory_Face(library_, bytes_.data(), static_cast<FT_Long>(bytes_.size()), 0, &face_) != 0) {
        FT_Done_FreeType(library_);
        throw std::runtime_error("FreeType could not read the font");
    }
    if (face_->family_name) family_ = face_->family_name;
    unitsPerEm_ = static_cast<float>(face_->units_per_EM);

    blob_ = hb_blob_create(reinterpret_cast<const char*>(bytes_.data()), static_cast<unsigned>(bytes_.size()),
                           HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    hbFace_ = hb_face_create(blob_, 0);
    font_ = hb_font_create(hbFace_);
    const int scale = static_cast<int>(unitsPerEm_);
    hb_font_set_scale(font_, scale, scale);
}

Font::~Font() {
    hb_font_destroy(font_);
    hb_face_destroy(hbFace_);
    hb_blob_destroy(blob_);
    FT_Done_Face(face_);
    FT_Done_FreeType(library_);
}

std::unique_ptr<Font> Font::load(const std::filesystem::path& path) {
    const std::string text = cinder::platform::readTextFile(path);
    std::vector<std::uint8_t> bytes(text.size());
    std::memcpy(bytes.data(), text.data(), text.size());
    try {
        return std::make_unique<Font>(std::move(bytes));
    } catch (const std::runtime_error& error) {
        throw std::runtime_error(path.string() + ": " + error.what());
    }
}

FontMetrics Font::metrics(float pixelSize) const {
    const float scale = pixelSize / unitsPerEm_;
    return FontMetrics{static_cast<float>(face_->ascender) * scale, static_cast<float>(face_->descender) * scale,
                       static_cast<float>(face_->height) * scale};
}

std::uint32_t Font::glyphIndex(char32_t codepoint) const {
    return FT_Get_Char_Index(face_, static_cast<FT_ULong>(codepoint));
}

GlyphBitmap Font::rasterize(std::uint32_t glyph, float pixelSize) const {
    setSize(face_, pixelSize);
    if (FT_Load_Glyph(face_, glyph, FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT) != 0) return {};
    if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_LIGHT) != 0) return {};

    const FT_Bitmap& bitmap = face_->glyph->bitmap;
    GlyphBitmap result;
    result.width = static_cast<int>(bitmap.width);
    result.height = static_cast<int>(bitmap.rows);
    result.bearing = glm::vec2(static_cast<float>(face_->glyph->bitmap_left),
                               -static_cast<float>(face_->glyph->bitmap_top));
    result.coverage.resize(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height));
    for (int row = 0; row < result.height; ++row) {
        const unsigned char* source = bitmap.buffer + static_cast<std::ptrdiff_t>(row) * bitmap.pitch;
        std::memcpy(result.coverage.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(result.width),
                    source, static_cast<std::size_t>(result.width));
    }
    return result;
}

}
