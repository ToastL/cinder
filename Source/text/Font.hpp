#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct FT_LibraryRec_;
struct FT_FaceRec_;
struct hb_blob_t;
struct hb_face_t;
struct hb_font_t;

namespace cinder::text {

struct FontMetrics {
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineHeight = 0.0f;
};

struct GlyphBitmap {
    int width = 0;
    int height = 0;
    glm::vec2 bearing{0.0f};
    std::vector<std::uint8_t> coverage;
};

class Font {
public:
    explicit Font(std::vector<std::uint8_t> bytes);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    static std::unique_ptr<Font> load(const std::filesystem::path& path);

    const std::string& family() const { return family_; }
    float unitsPerEm() const { return unitsPerEm_; }
    FontMetrics metrics(float pixelSize) const;
    std::uint32_t glyphIndex(char32_t codepoint) const;
    GlyphBitmap rasterize(std::uint32_t glyph, float pixelSize) const;

    hb_font_t* shaper() const { return font_; }

private:
    std::vector<std::uint8_t> bytes_;
    FT_LibraryRec_* library_ = nullptr;
    FT_FaceRec_* face_ = nullptr;
    hb_blob_t* blob_ = nullptr;
    hb_face_t* hbFace_ = nullptr;
    hb_font_t* font_ = nullptr;
    std::string family_;
    float unitsPerEm_ = 1.0f;
};

}
