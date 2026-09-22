#pragma once

#include "platform/InputEvent.hpp"
#include "text/FontSet.hpp"
#include "text/GlyphAtlas.hpp"
#include "ui/core/HitTester.hpp"
#include "ui/core/Style.hpp"
#include "ui/core/Widget.hpp"
#include "ui/framework/Commands.hpp"
#include "ui/framework/PlatformHooks.hpp"

#include <glm/vec2.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace cinder::ui {

class ElementList;

class Application {
public:
    static constexpr float DRAG_THRESHOLD = 6.0f;
    static constexpr double DOUBLE_CLICK_TIME = 0.5;
    static constexpr float DOUBLE_CLICK_DISTANCE = 4.0f;

    Application(PlatformHooks& platform, const cinder::text::FontSet& fonts, Theme theme);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static Application& get();
    static Application* current();

    void setRoot(std::shared_ptr<Widget> root) { root_ = std::move(root); }
    const std::shared_ptr<Widget>& root() const { return root_; }

    void processEvents(std::span<const cinder::platform::InputEvent> events);
    void paint(ElementList& list);

    void setFocus(const std::shared_ptr<Widget>& widget, FocusCause cause = FocusCause::SetDirectly);
    void clearFocus(FocusCause cause = FocusCause::Cleared);
    std::shared_ptr<Widget> focused() const { return focused_.lock(); }
    bool focusWithin(const Widget& widget) const;

    std::shared_ptr<Widget> captor() const { return captor_.lock(); }
    void releaseCapture();
    bool isInteracting() const;

    glm::vec2 cursorPosition() const { return cursor_; }
    std::uint32_t buttons() const { return buttons_; }
    std::uint8_t modifiers() const { return modifiers_; }
    bool keyHeld(int key) const;

    void setMouseEnabled(bool enabled);
    bool mouseEnabled() const { return mouseEnabled_; }

    void addCommands(std::shared_ptr<const CommandList> commands);

    const cinder::text::FontSet& fonts() const { return fonts_; }
    cinder::text::GlyphAtlas& atlas() { return atlas_; }
    const Theme& theme() const { return theme_; }
    void setTheme(Theme theme) { theme_ = std::move(theme); }
    PlatformHooks& platform() { return platform_; }
    double time() const { return time_; }
    float deltaTime() const { return deltaTime_; }
    glm::vec2 windowSize() const { return windowSize_; }
    float scale() const { return scale_; }
    const HitTester& grid() const { return grid_; }

private:
    class Scope;

    struct DragDetect {
        std::weak_ptr<Widget> widget;
        int button = -1;
        glm::vec2 origin{0.0f};
    };

    struct Click {
        int button = -1;
        double time = -1.0e9;
        glm::vec2 position{0.0f};
    };

    PointerEvent pointer(const cinder::platform::InputEvent& event, int button) const;
    WidgetPath focusPath();
    WidgetPath captorPath() const;
    void apply(const Reply& reply);
    void setHover(const WidgetPath& path, const PointerEvent& event);
    void refreshHover();
    void updateCursor();
    void focusFromClick(const WidgetPath& path);

    void mouseDown(const cinder::platform::InputEvent& event);
    void mouseUp(const cinder::platform::InputEvent& event);
    void mouseMove(const cinder::platform::InputEvent& event);
    void wheel(const cinder::platform::InputEvent& event);
    void keyDown(const cinder::platform::InputEvent& event);
    void keyUp(const cinder::platform::InputEvent& event);
    void character(const cinder::platform::InputEvent& event);
    void windowFocusLost();

    PlatformHooks& platform_;
    const cinder::text::FontSet& fonts_;
    cinder::text::GlyphAtlas atlas_;
    Theme theme_;
    HitTester grid_;
    std::shared_ptr<Widget> root_;

    std::weak_ptr<Widget> focused_;
    std::weak_ptr<Widget> captor_;
    std::vector<std::weak_ptr<Widget>> hovered_;
    std::vector<std::shared_ptr<const CommandList>> commands_;
    std::optional<DragDetect> drag_;
    Click lastClick_;

    std::array<bool, cinder::platform::keys::COUNT> keys_{};
    glm::vec2 cursor_{-1.0e6f};
    glm::vec2 lastCursor_{-1.0e6f};
    glm::vec2 windowSize_{0.0f};
    std::uint32_t buttons_ = 0;
    std::uint8_t modifiers_ = 0;
    double time_ = 0.0;
    float deltaTime_ = 0.0f;
    float scale_ = 1.0f;
    float atlasScale_ = 0.0f;
    bool painted_ = false;
    bool mouseEnabled_ = true;
};

}
