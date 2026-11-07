#pragma once

#include "Platform/InputEvent.hpp"
#include "Text/FontSet.hpp"
#include "UI/Core/Args.hpp"
#include "UI/Core/ElementList.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Framework/PlatformHooks.hpp"
#include "UI/Widgets/DefaultTheme.hpp"

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace uitest {

using cinder::Platform::InputEvent;
using cinder::Platform::InputEventType;

inline const cinder::Text::FontSet& fonts() {
    static const cinder::Text::FontSet set = cinder::Text::FontSet::engineDefault();
    return set;
}

struct Harness {
    cinder::UI::HeadlessPlatform platform;
    cinder::UI::Application app{platform, fonts(), cinder::UI::defaultTheme()};
    cinder::UI::ElementList list;
    glm::vec2 size{400.0f, 300.0f};
    glm::vec2 cursor{0.0f};
    std::uint8_t mods = 0;

    explicit Harness(std::shared_ptr<cinder::UI::Widget> root = nullptr) {
        if (root) app.setRoot(std::move(root));
        frame();
    }

    void frame() {
        list.reset(size, 1.0f);
        app.paint(list);
    }

    void send(InputEvent event) {
        event.modifiers = mods;
        if (event.position == glm::vec2(0.0f)) event.position = cursor;
        const InputEvent events[] = {event};
        app.processEvents(events);
    }

    void move(float x, float y) {
        cursor = {x, y};
        InputEvent event;
        event.type = InputEventType::MouseMove;
        event.position = cursor;
        send(event);
    }

    void press(int button = cinder::Platform::buttons::LEFT) {
        InputEvent event;
        event.type = InputEventType::MouseDown;
        event.code = button;
        event.position = cursor;
        send(event);
    }

    void release(int button = cinder::Platform::buttons::LEFT) {
        InputEvent event;
        event.type = InputEventType::MouseUp;
        event.code = button;
        event.position = cursor;
        send(event);
    }

    void click(float x, float y, int button = cinder::Platform::buttons::LEFT) {
        move(x, y);
        press(button);
        release(button);
    }

    void key(int code, std::uint8_t modifiers = 0) {
        const std::uint8_t saved = mods;
        mods = modifiers;
        InputEvent down;
        down.type = InputEventType::KeyDown;
        down.code = code;
        send(down);
        InputEvent up;
        up.type = InputEventType::KeyUp;
        up.code = code;
        send(up);
        mods = saved;
    }

    void type(std::u32string_view text) {
        for (const char32_t character : text) {
            InputEvent event;
            event.type = InputEventType::Char;
            event.character = character;
            send(event);
        }
    }

    void wheel(float y) {
        InputEvent event;
        event.type = InputEventType::Wheel;
        event.wheel = {0.0f, y};
        event.position = cursor;
        send(event);
    }

    cinder::UI::Geometry geometryOf(const std::shared_ptr<cinder::UI::Widget>& widget) const {
        return app.grid().geometryOf(widget.get()).value_or(cinder::UI::Geometry{});
    }

    cinder::UI::Rect rectOf(const std::shared_ptr<cinder::UI::Widget>& widget) const {
        return geometryOf(widget).rect();
    }

    bool painted(const std::shared_ptr<cinder::UI::Widget>& widget) const { return app.grid().contains(widget.get()); }
};

class Probe : public cinder::UI::LeafWidget {
public:
    struct Args : ::cinder::UI::Args<Args, Probe> {
        UI_ARG(glm::vec2, size, glm::vec2(20.0f))
        UI_ARG(bool, handles, true)
        UI_ARG(bool, focusable)
    };

    void construct(const Args& args) {
        size_ = args.size_;
        handles_ = args.handles_;
        focusable_ = args.focusable_;
    }

    std::vector<std::string> log;
    bool handles_ = true;

    cinder::UI::Reply onMouseDown(const cinder::UI::Geometry&, const cinder::UI::PointerEvent&) override {
        log.push_back("down");
        return reply();
    }
    cinder::UI::Reply onMouseUp(const cinder::UI::Geometry&, const cinder::UI::PointerEvent&) override {
        log.push_back("up");
        return reply();
    }
    cinder::UI::Reply onMouseDoubleClick(const cinder::UI::Geometry&, const cinder::UI::PointerEvent&) override {
        log.push_back("double");
        return reply();
    }
    cinder::UI::Reply onMouseWheel(const cinder::UI::Geometry&, const cinder::UI::PointerEvent&) override {
        log.push_back("wheel");
        return reply();
    }
    cinder::UI::Reply onKeyDown(const cinder::UI::Geometry&, const cinder::UI::KeyEvent&) override {
        log.push_back("key");
        return reply();
    }
    cinder::UI::Reply onDragDetected(const cinder::UI::Geometry&, const cinder::UI::PointerEvent&) override {
        log.push_back("drag");
        return reply();
    }
    void onMouseEnter(const cinder::UI::Geometry&, const cinder::UI::PointerEvent&) override { log.push_back("enter"); }
    void onMouseLeave(const cinder::UI::PointerEvent&) override { log.push_back("leave"); }
    void onFocusLost(const cinder::UI::FocusEvent&) override { log.push_back("blur"); }
    bool supportsKeyboardFocus() const override { return focusable_; }

protected:
    glm::vec2 computeDesiredSize(float) const override { return size_; }
    int onPaint(const cinder::UI::PaintArgs&, const cinder::UI::Geometry&, cinder::UI::ElementList&, int layer,
                const cinder::UI::PaintStyle&, bool) const override {
        return layer;
    }

private:
    cinder::UI::Reply reply() const {
        return handles_ ? cinder::UI::Reply::handled() : cinder::UI::Reply::unhandled();
    }

    glm::vec2 size_{20.0f};
    bool focusable_ = false;
};

inline int count(const std::vector<std::string>& log, std::string_view entry) {
    int total = 0;
    for (const std::string& item : log) {
        if (item == entry) ++total;
    }
    return total;
}

}
