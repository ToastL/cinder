#pragma once

#include <glm/vec2.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace cinder::text {

class Font;

struct AtlasRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const AtlasRect&) const = default;
};

struct AtlasGlyph {
    int page = -1;
    AtlasRect rect;
    glm::vec2 bearing{0.0f};

    bool empty() const { return page < 0; }
};

class GlyphAtlas {
public:
    static constexpr int PAGE_SIZE = 1024;
    static constexpr int PADDING = 1;

    GlyphAtlas();
    ~GlyphAtlas();

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;

    const AtlasGlyph& glyph(const Font& font, std::uint32_t glyph, float pixelSize);

    int pageCount() const { return static_cast<int>(pages_.size()); }
    const std::vector<std::uint8_t>& pixels(int page) const;
    std::optional<AtlasRect> dirty(int page) const;
    void clearDirty(int page);
    void clear();

    std::size_t size() const { return glyphs_.size(); }

private:
    struct Page;

    struct Key {
        const Font* font = nullptr;
        std::uint32_t glyph = 0;
        std::uint32_t size = 0;

        bool operator==(const Key&) const = default;
    };

    struct KeyHash {
        std::size_t operator()(const Key& key) const;
    };

    std::optional<AtlasRect> place(int width, int height, int& page);
    void markDirty(int page, const AtlasRect& rect);

    std::vector<std::unique_ptr<Page>> pages_;
    std::unordered_map<Key, AtlasGlyph, KeyHash> glyphs_;
};

}
