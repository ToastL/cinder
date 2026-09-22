#include "core/Engine.hpp"
#include "core/ProjectConfig.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Log.hpp"
#include "text/FontSet.hpp"
#include "text/GlyphAtlas.hpp"
#include "text/Shaper.hpp"
#include "ui/core/ElementList.hpp"

#include <GLFW/glfw3.h>

#include <glm/trigonometric.hpp>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

using cinder::text::FontStyle;
using cinder::ui::BoxStyle;
using cinder::ui::ElementList;
using cinder::ui::LinearColor;
using cinder::ui::Rect;
using cinder::ui::Transform2D;

namespace {

const LinearColor BACKGROUND = LinearColor::hex(0x151515FF);
const LinearColor PANEL = LinearColor::hex(0x242424FF);
const LinearColor RAISED = LinearColor::hex(0x383838FF);
const LinearColor OUTLINE = LinearColor::hex(0x575757FF);
const LinearColor TEXT = LinearColor::hex(0xC0C0C0FF);
const LinearColor BRIGHT = LinearColor::hex(0xF0F0F0FF);
const LinearColor DIM = LinearColor::hex(0x808080FF);
const LinearColor ACCENT = LinearColor::hex(0x0070E0FF);
const LinearColor WARNING = LinearColor::hex(0xFFB02EFF);
const LinearColor RED = LinearColor::hex(0xE84848FF);
const LinearColor GREEN = LinearColor::hex(0x78CC50FF);

constexpr std::string_view PANGRAM = "The quick brown fox jumps over the lazy dog 0123456789";
constexpr std::string_view SHORT = "Sphinx of black quartz, judge my vow";

class Gallery {
public:
    Gallery() : fonts_(cinder::text::FontSet::engineDefault()) {}

    void paint(ElementList& list) {
        const Rect window = list.bounds();
        list.box(0, window, BoxStyle{BACKGROUND});
        heading(list, glm::vec2(24.0f, 40.0f), "cinder UI gallery");
        caption(list, glm::vec2(24.0f, 62.0f),
                "points " + std::to_string(static_cast<int>(window.width())) + " x "
                        + std::to_string(static_cast<int>(window.height())) + ", "
                        + std::to_string(list.scale()).substr(0, 4) + " pixels per point");

        boxes(list, Rect::fromSize({24.0f, 84.0f}, {520.0f, 150.0f}));
        text(list, Rect::fromSize({24.0f, 250.0f}, {520.0f, 250.0f}));
        strokes(list, Rect::fromSize({560.0f, 84.0f}, {500.0f, 190.0f}));
        shapes(list, Rect::fromSize({560.0f, 290.0f}, {500.0f, 210.0f}));
        clipping(list, Rect::fromSize({24.0f, 516.0f}, {1036.0f, 120.0f}));
    }

private:
    void write(ElementList& list, int layer, glm::vec2 baseline, std::string_view text, FontStyle style,
               float size, LinearColor color) {
        list.text(layer, baseline, cinder::text::shape(fonts_.get(style), text, size), color, atlas_);
    }

    float width(std::string_view text, FontStyle style, float size) {
        return cinder::text::shape(fonts_.get(style), text, size).width;
    }

    void heading(ElementList& list, glm::vec2 at, std::string_view text) {
        write(list, 10, at, text, FontStyle::Bold, 22.0f, BRIGHT);
    }

    void caption(ElementList& list, glm::vec2 at, std::string_view text) {
        write(list, 10, at, text, FontStyle::Regular, 12.0f, DIM);
    }

    void panel(ElementList& list, const Rect& rect, std::string_view title) {
        list.box(1, rect, BoxStyle{PANEL, OUTLINE, 1.0f, glm::vec4(6.0f)});
        write(list, 10, rect.min + glm::vec2(12.0f, 20.0f), title, FontStyle::Bold, 13.0f, TEXT);
    }

    void boxes(ElementList& list, const Rect& area) {
        panel(list, area, "Boxes: radius, border, alpha");
        const std::array<float, 5> radii = {0.0f, 2.0f, 4.0f, 8.0f, 20.0f};
        for (std::size_t i = 0; i < radii.size(); ++i) {
            const Rect box = Rect::fromSize(area.min + glm::vec2(12.0f + static_cast<float>(i) * 100.0f, 34.0f),
                                            {88.0f, 44.0f});
            list.box(2, box, BoxStyle{RAISED, OUTLINE, 1.0f, glm::vec4(radii[i])});
            caption(list, box.min + glm::vec2(8.0f, 27.0f), "r " + std::to_string(static_cast<int>(radii[i])));
        }

        const Rect button = Rect::fromSize(area.min + glm::vec2(12.0f, 92.0f), {120.0f, 28.0f});
        list.box(2, button, BoxStyle{ACCENT, LinearColor::transparent(), 0.0f, glm::vec4(4.0f)});
        const float label = width("Play", FontStyle::Bold, 13.0f);
        write(list, 3, glm::vec2(button.center().x - label * 0.5f, button.min.y + 19.0f), "Play", FontStyle::Bold,
              13.0f, BRIGHT);

        const Rect ring = Rect::fromSize(area.min + glm::vec2(148.0f, 92.0f), {120.0f, 28.0f});
        list.box(2, ring, BoxStyle{LinearColor::transparent(), ACCENT, 2.0f, glm::vec4(14.0f)});

        const Rect mixed = Rect::fromSize(area.min + glm::vec2(284.0f, 92.0f), {60.0f, 28.0f});
        list.box(2, mixed, BoxStyle{PANEL, WARNING, 1.0f, glm::vec4(0.0f, 14.0f, 0.0f, 14.0f)});

        for (int i = 0; i < 3; ++i) {
            const Rect glass = Rect::fromSize(area.min + glm::vec2(360.0f + i * 40.0f, 84.0f + i * 8.0f), {60.0f, 40.0f});
            const LinearColor color = i == 0 ? RED : (i == 1 ? GREEN : ACCENT);
            list.box(4 + i, glass, BoxStyle{color.withAlpha(0.5f), LinearColor::transparent(), 0.0f, glm::vec4(6.0f)});
        }
    }

    void text(ElementList& list, const Rect& area) {
        panel(list, area, "Text: Roboto, Roboto Bold, Roboto Mono");
        float y = area.min.y + 48.0f;
        for (const float size : {11.0f, 12.0f, 13.0f, 16.0f, 20.0f}) {
            const std::string_view sample = size > 14.0f ? SHORT : PANGRAM;
            write(list, 10, {area.min.x + 12.0f, y}, std::to_string(static_cast<int>(size)) + "  " + std::string(sample),
                  FontStyle::Regular, size, TEXT);
            y += size + 12.0f;
        }
        write(list, 10, {area.min.x + 12.0f, y}, "Bold 13  Properties  Explorer  Transform", FontStyle::Bold, 13.0f,
              BRIGHT);
        y += 24.0f;
        write(list, 10, {area.min.x + 12.0f, y}, "Mono 12  > engine.time()  1.2500 -0.0314", FontStyle::Mono, 12.0f,
              GREEN);
        y += 24.0f;
        write(list, 10, {area.min.x + 12.0f, y}, "Kerning: AV Wa To Ty  \xC3\xA9\xC3\xA0\xC3\xBC \xE2\x80\x94 12\xC2\xB0",
              FontStyle::Regular, 16.0f, WARNING);
    }

    void strokes(ElementList& list, const Rect& area) {
        panel(list, area, "Lines: thickness and joints");
        const std::array<float, 5> widths = {0.5f, 1.0f, 1.5f, 2.5f, 4.0f};
        for (std::size_t i = 0; i < widths.size(); ++i) {
            const float x = area.min.x + 20.0f + static_cast<float>(i) * 36.0f;
            const std::array<glm::vec2, 2> line = {glm::vec2(x, area.min.y + 40.0f), glm::vec2(x + 24.0f, area.max.y - 16.0f)};
            list.lines(2, line, BRIGHT, widths[i]);
        }

        std::vector<glm::vec2> zigzag;
        for (int i = 0; i < 8; ++i) {
            zigzag.emplace_back(area.min.x + 210.0f + i * 22.0f, area.min.y + (i % 2 == 0 ? 50.0f : 90.0f));
        }
        list.lines(2, zigzag, ACCENT, 2.0f);

        std::vector<glm::vec2> circle;
        const glm::vec2 centre = area.min + glm::vec2(290.0f, 140.0f);
        for (int i = 0; i < 48; ++i) {
            const float angle = glm::radians(static_cast<float>(i) * 7.5f);
            circle.push_back(centre + glm::vec2(std::cos(angle), std::sin(angle)) * 34.0f);
        }
        list.lines(2, circle, WARNING, 1.5f, true);

        const std::array<glm::vec2, 4> square = {centre + glm::vec2(90.0f, -30.0f), centre + glm::vec2(150.0f, -30.0f),
                                                 centre + glm::vec2(150.0f, 30.0f), centre + glm::vec2(90.0f, 30.0f)};
        list.lines(2, square, GREEN, 3.0f, true);
    }

    void shapes(ElementList& list, const Rect& area) {
        panel(list, area, "Polygons and a render transform");
        const glm::vec2 origin = area.min + glm::vec2(60.0f, 110.0f);
        const std::array<glm::vec2, 3> triangle = {origin + glm::vec2(0.0f, -40.0f), origin + glm::vec2(40.0f, 30.0f),
                                                   origin + glm::vec2(-40.0f, 30.0f)};
        list.polygon(2, triangle, RED);

        std::vector<glm::vec2> hexagon;
        for (int i = 0; i < 6; ++i) {
            const float angle = glm::radians(60.0f * static_cast<float>(i));
            hexagon.push_back(origin + glm::vec2(120.0f, 0.0f) + glm::vec2(std::cos(angle), std::sin(angle)) * 40.0f);
        }
        list.polygon(2, hexagon, GREEN.withAlpha(0.8f));

        const glm::vec2 pivot = origin + glm::vec2(290.0f, 0.0f);
        const float angle = glm::radians(20.0f);
        Transform2D turn;
        turn.linear = glm::mat2(std::cos(angle), std::sin(angle), -std::sin(angle), std::cos(angle));
        list.pushTransform(Transform2D::translate(-pivot).then(turn).then(Transform2D::translate(pivot)));
        const Rect card = Rect::fromSize(pivot - glm::vec2(70.0f, 32.0f), {140.0f, 64.0f});
        list.box(2, card, BoxStyle{RAISED, ACCENT, 1.5f, glm::vec4(8.0f)});
        write(list, 3, card.min + glm::vec2(16.0f, 38.0f), "Rotated 20\xC2\xB0", FontStyle::Regular, 14.0f, BRIGHT);
        list.popTransform();
    }

    void clipping(ElementList& list, const Rect& area) {
        panel(list, area, "Clipping: text runs past its box");
        const Rect clip = Rect::fromSize(area.min + glm::vec2(12.0f, 32.0f), {300.0f, 72.0f});
        list.box(2, clip, BoxStyle{RAISED, OUTLINE, 1.0f});
        list.pushClip(clip.inset(4.0f));
        for (int row = 0; row < 4; ++row) {
            write(list, 3, clip.min + glm::vec2(8.0f, 20.0f + row * 18.0f), PANGRAM, FontStyle::Regular, 13.0f, TEXT);
        }
        list.popClip();

        const Rect viewport = Rect::fromSize(area.min + glm::vec2(330.0f, 32.0f), {200.0f, 72.0f});
        list.image(2, viewport, cinder::ui::TextureRef::viewport(), LinearColor::white(), glm::vec2(0.0f),
                   glm::vec2(1.0f), true);
        caption(list, viewport.max + glm::vec2(8.0f, -4.0f), "scene target, drawn opaque");
    }

    cinder::text::FontSet fonts_;
    cinder::text::GlyphAtlas atlas_;
};

}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    cinder::platform::Glfw::acquire();
    int frameLimit = 0;
    std::string capture;
    float gamma = cinder::gfx::UiRenderer::DEFAULT_TEXT_GAMMA;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) frameLimit = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--capture-window") == 0 && i + 1 < argc) capture = argv[++i];
        else if (std::strcmp(argv[i], "--text-gamma") == 0 && i + 1 < argc) gamma = static_cast<float>(std::atof(argv[++i]));
        else if (std::strcmp(argv[i], "--lowdpi") == 0) glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
    }

    try {
        {
            cinder::core::ProjectConfig config;
            config.title = "UI Gallery";
            config.width = 1084;
            config.height = 660;
            cinder::core::Engine engine(config);
            engine.renderer().ui().setTextGamma(gamma);

            Gallery gallery;
            engine.renderer().setUiPaint([&gallery](ElementList& list) { gallery.paint(list); });

            int frames = 0;
            while (engine.running()) {
                cinder::platform::Glfw::pollEvents();
                if (engine.minimized()) {
                    cinder::platform::Glfw::waitEvents();
                    continue;
                }
                const bool last = frameLimit > 0 && frames + 1 >= frameLimit;
                if (last && !capture.empty()) engine.renderer().requestWindowCapture(capture);
                engine.render(0.0f);
                if (last) break;
                ++frames;
            }
            engine.renderer().setUiPaint(nullptr);
        }
        cinder::platform::Glfw::release();
        return 0;
    } catch (const std::exception& error) {
        cinder::platform::logError("[fatal] %s\n", error.what());
        return 1;
    }
}
