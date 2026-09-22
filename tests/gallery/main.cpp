#include "core/Engine.hpp"
#include "core/ProjectConfig.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/InputScript.hpp"
#include "platform/Log.hpp"
#include "text/FontSet.hpp"
#include "text/Shaper.hpp"
#include "ui/core/Args.hpp"
#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/framework/WindowPlatform.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/DefaultTheme.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/ScrollBox.hpp"
#include "ui/widgets/SizeBox.hpp"
#include "ui/widgets/Splitter.hpp"
#include "ui/widgets/TextField.hpp"
#include "ui/widgets/Viewport.hpp"

#include <GLFW/glfw3.h>

#include <glm/trigonometric.hpp>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <optional>
#include <string>
#include <vector>

using namespace cinder::ui;
using cinder::text::FontStyle;

namespace {

class Drawing : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Drawing> {};

    void construct(const Args&) {}

protected:
    glm::vec2 computeDesiredSize(float) const override { return {420.0f, 520.0f}; }

    int onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer, const PaintStyle& style,
                bool) const override {
        Application& app = Application::get();
        const Theme& theme = app.theme();
        const glm::vec2 origin = geometry.position;
        const auto write = [&](glm::vec2 at, std::string_view text, float size, Color color) {
            list.text(layer + 2, origin + at, cinder::text::shape(app.fonts().get(FontStyle::Regular), text, size),
                      color, app.atlas());
        };

        const std::array<float, 4> radii = {0.0f, 4.0f, 8.0f, 20.0f};
        for (std::size_t i = 0; i < radii.size(); ++i) {
            const Rect box = Rect::fromSize(origin + glm::vec2(static_cast<float>(i) * 96.0f, 0.0f), {84.0f, 40.0f});
            list.box(layer, box, BoxStyle{theme.color("Color.Dropdown"), theme.color("Color.Outline"), 1.0f,
                                          glm::vec4(radii[i])});
            write({static_cast<float>(i) * 96.0f + 8.0f, 25.0f}, "r " + std::to_string(static_cast<int>(radii[i])),
                  11.0f, theme.color("Color.ForegroundDim"));
        }
        for (int i = 0; i < 3; ++i) {
            const Color color = theme.color(i == 0 ? "Color.AxisX" : (i == 1 ? "Color.AxisY" : "Color.AxisZ"));
            list.box(layer + 1 + i, Rect::fromSize(origin + glm::vec2(i * 36.0f, 60.0f + i * 8.0f), {60.0f, 40.0f}),
                     BoxStyle{color.withAlpha(0.5f), Color::transparent(), 0.0f, glm::vec4(6.0f)});
        }

        const std::array<float, 5> widths = {0.5f, 1.0f, 1.5f, 2.5f, 4.0f};
        for (std::size_t i = 0; i < widths.size(); ++i) {
            const float x = origin.x + 160.0f + static_cast<float>(i) * 30.0f;
            const std::array<glm::vec2, 2> line = {glm::vec2(x, origin.y + 60.0f), glm::vec2(x + 20.0f, origin.y + 150.0f)};
            list.lines(layer, line, theme.color("Color.ForegroundBright"), widths[i]);
        }

        std::vector<glm::vec2> circle;
        const glm::vec2 centre = origin + glm::vec2(360.0f, 105.0f);
        for (int i = 0; i < 48; ++i) {
            const float angle = glm::radians(static_cast<float>(i) * 7.5f);
            circle.push_back(centre + glm::vec2(std::cos(angle), std::sin(angle)) * 40.0f);
        }
        list.lines(layer, circle, theme.color("Color.Warning"), 1.5f, true);

        const glm::vec2 base = origin + glm::vec2(40.0f, 230.0f);
        const std::array<glm::vec2, 3> triangle = {base + glm::vec2(0.0f, -40.0f), base + glm::vec2(40.0f, 30.0f),
                                                   base + glm::vec2(-40.0f, 30.0f)};
        list.polygon(layer, triangle, theme.color("Color.AxisX"));

        const glm::vec2 pivot = origin + glm::vec2(230.0f, 230.0f);
        const float angle = glm::radians(20.0f);
        Transform2D turn;
        turn.linear = glm::mat2(std::cos(angle), std::sin(angle), -std::sin(angle), std::cos(angle));
        list.pushTransform(Transform2D::translate(-pivot).then(turn).then(Transform2D::translate(pivot)));
        list.box(layer, Rect::fromSize(pivot - glm::vec2(70.0f, 30.0f), {140.0f, 60.0f}),
                 BoxStyle{theme.color("Color.Dropdown"), theme.color("Color.Primary"), 1.5f, glm::vec4(8.0f)});
        write({230.0f - 50.0f, 236.0f}, "Rotated 20\xC2\xB0", 14.0f, style.foreground);
        list.popTransform();

        const Rect scene = Rect::fromSize(origin + glm::vec2(0.0f, 310.0f), {200.0f, 110.0f});
        list.image(layer, scene, TextureRef::viewport(), Color::white(), glm::vec2(0.0f), glm::vec2(1.0f), true);
        write({210.0f, 360.0f}, "scene target, opaque", 12.0f, theme.color("Color.ForegroundDim"));
        return layer + 4;
    }
};

std::shared_ptr<Widget> section(std::string title, std::shared_ptr<Widget> content) {
    return make<VerticalBox>()
           + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 10.0f, 0.0f, 6.0f))
                     [make<Label>().text(std::move(title)).textStyle("Label.Header")]
           + VerticalBox::slot().autoHeight()[std::move(content)];
}

struct Gallery {
    int clicks = 0;
    bool grid = true;
    bool snap = false;
    std::string name = "Box";
    std::string committed;
    std::shared_ptr<ScrollBox> log;

    std::shared_ptr<Widget> build() {
        auto rows = make<ScrollBox>().assign(log);
        for (int i = 0; i < 40; ++i) {
            rows + ScrollBox::slot().padding(Margin(4.0f, 1.0f))
                           [make<Label>().text("[lua] row " + std::to_string(i) + "  print(engine.time())").textStyle("Label.Mono")];
        }

        auto widgets = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()[section("Buttons",
                make<HorizontalBox>()
                + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                      [make<Button>().text("Play").buttonStyle("Button.Primary").onClicked([this] {
                          ++clicks;
                          return Reply::handled();
                      })]
                + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                      [make<Button>().text("Stop").onClicked([this] {
                          clicks = 0;
                          return Reply::handled();
                      })]
                + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 12.0f, 0.0f))
                      [make<Button>().text("Step").isEnabled(false).toolTipText("Only while paused")]
                + HorizontalBox::slot().vAlign(VAlign::Center)
                      [make<Label>().text([this] { return "clicked " + std::to_string(clicks) + " times"; })])]
            + VerticalBox::slot().autoHeight()[section("Check boxes",
                make<HorizontalBox>()
                + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 16.0f, 0.0f))
                      [make<CheckBox>().isChecked([this] { return grid; }).onCheckStateChanged([this](bool on) { grid = on; })
                           [make<Label>().text("Show grid")]]
                + HorizontalBox::slot().autoWidth()
                      [make<CheckBox>().isChecked([this] { return snap; }).onCheckStateChanged([this](bool on) { snap = on; })
                           [make<Label>().text("Snap")]])]
            + VerticalBox::slot().autoHeight()[section("Text",
                make<VerticalBox>()
                + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 6.0f))
                      [make<TextBox>().text([this] { return name; }).onTextCommitted([this](const std::string& text, TextCommit) {
                          name = text;
                          committed = "committed \"" + text + "\"";
                      })]
                + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 6.0f))
                      [make<TextBox>().hintText("Filter").selectAllOnFocus(true)]
                + VerticalBox::slot().autoHeight()
                      [make<Label>().text([this] { return committed; }).textStyle("Label.Small")])]
            + VerticalBox::slot().autoHeight()[section("Labels",
                make<VerticalBox>()
                + VerticalBox::slot().autoHeight()[make<Label>().text("Small 11: The quick brown fox jumps over the lazy dog").textStyle("Label.Small")]
                + VerticalBox::slot().autoHeight()[make<Label>().text("Regular 13: Kerning AV Wa To \xC3\xA9\xC3\xA0\xC3\xBC \xE2\x80\x94 12\xC2\xB0")]
                + VerticalBox::slot().autoHeight()[make<Label>().text("Bold 13: Properties  Explorer  Transform").textStyle("Label.Bold")]
                + VerticalBox::slot().autoHeight()[make<Label>().text("Mono 12: > engine.time()  1.2500").textStyle("Label.Mono")])]
            + VerticalBox::slot().fill(1.0f).padding(Margin(0.0f, 10.0f, 0.0f, 0.0f))
                  [make<Border>().brush(Brush::rounded(Color::hex(0x0F0F0FFF), 4.0f)).padding(Margin(2.0f))[rows]];

        return make<Border>().brush(Brush::color(Color::hex(0x151515FF))).padding(Margin(0.0f))
            [make<Splitter>()
             + Splitter::slot().value(1.0f)
                   [make<Border>().brush(Brush::color(Color::hex(0x242424FF))).padding(Margin(16.0f))[widgets]]
             + Splitter::slot().value(1.0f)
                   [make<Border>().brush(Brush::color(Color::hex(0x1A1A1AFF))).padding(Margin(16.0f))
                        [section("Drawing", make<Drawing>())]]];
    }
};

}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);
    cinder::platform::Glfw::acquire();

    int frameLimit = 0;
    std::string capture;
    std::string script;
    float gamma = cinder::gfx::UiRenderer::DEFAULT_TEXT_GAMMA;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) frameLimit = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--capture-window") == 0 && i + 1 < argc) capture = argv[++i];
        else if (std::strcmp(argv[i], "--input-script") == 0 && i + 1 < argc) script = argv[++i];
        else if (std::strcmp(argv[i], "--text-gamma") == 0 && i + 1 < argc) gamma = static_cast<float>(std::atof(argv[++i]));
        else if (std::strcmp(argv[i], "--lowdpi") == 0) glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
    }

    try {
        {
            cinder::core::ProjectConfig config;
            config.title = "UI Gallery";
            config.width = 1100;
            config.height = 720;
            cinder::core::Engine engine(config);
            engine.renderer().ui().setTextGamma(gamma);

            const cinder::text::FontSet fonts = cinder::text::FontSet::engineDefault();
            WindowPlatform platform(engine.window());
            Application app(platform, fonts, defaultTheme());
            Gallery gallery;
            app.setRoot(gallery.build());

            cinder::platform::Input& input = engine.input();
            input.setRecording(true);
            std::optional<cinder::platform::InputScript> steps;
            if (!script.empty()) {
                steps = cinder::platform::InputScript::load(script);
                input.setIgnoreSystem(true);
            }

            engine.renderer().setUiPaint([&](ElementList& list) {
                const std::vector<cinder::platform::InputEvent> events = input.takeEvents();
                app.processEvents(events);
                app.paint(list);
            });

            int frames = 0;
            while (engine.running()) {
                cinder::platform::Glfw::pollEvents();
                if (engine.minimized()) {
                    cinder::platform::Glfw::waitEvents();
                    continue;
                }
                if (steps) {
                    steps->apply(input, frames);
                    if (std::optional<std::string> path = steps->capture(frames)) engine.renderer().requestWindowCapture(*path);
                    if (steps->quits(frames)) break;
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
