#pragma once

#include "gfx/mtl/Commands.hpp"
#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Fwd.hpp"
#include "gfx/rhi/Handles.hpp"

#include <glm/vec2.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::platform { class Window; }

namespace cinder::gfx::rhi {

struct Frame {
    std::uint32_t index = 0;
    std::uint32_t image = 0;
    Commands commands;
    Uploads uploads;

    std::unique_ptr<cinder::gfx::mtl::CommandState> state;
    void* commandBuffer = nullptr;
    void* drawable = nullptr;
    void* pool = nullptr;
};

class Presenter {
public:
    Presenter(const Ctx& ctx, cinder::platform::Window& window,
              std::uint32_t framesInFlight);
    ~Presenter();

    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    void onRecreated(std::function<void(bool formatChanged)> handler) {
        recreated_ = std::move(handler);
    }

    const Ctx& ctx() const { return ctx_; }
    Format colorFormat() const { return Format::BGRA8Srgb; }
    glm::uvec2 extent() const;
    float pixelsPerPoint() const { return pixelsPerPoint_; }

    std::unique_ptr<RenderTarget> createTarget(glm::uvec2 size) const;

    std::optional<Frame> begin();
    void beginScenePass(Frame& frame, const RenderTarget& target, float r, float g, float b);
    void beginPresentPass(Frame& frame, float r, float g, float b);
    void endPass(Frame& frame);
    void end(Frame& frame);

    void captureTarget(const RenderTarget& target, const std::string& path) const;
    void requestWindowCapture(const std::string& path) { windowCapture_ = path; }
    bool windowCapturePending() const { return !windowCapture_.empty(); }

private:
    void resize();
    void capture(void* texture, std::uint32_t width, std::uint32_t height,
                 const std::string& path) const;

    const Ctx& ctx_;
    cinder::platform::Window& window_;
    std::uint32_t framesInFlight_ = 0;
    std::uint32_t frame_ = 0;
    void* layer_ = nullptr;
    std::vector<void*> inFlight_;
    std::function<void(bool)> recreated_;
    std::string windowCapture_;
    float pixelsPerPoint_ = 1.0f;
};

}
