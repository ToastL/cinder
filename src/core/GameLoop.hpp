#pragma once

namespace cinder::core {

class Engine;

class GameLoop {
public:
    static constexpr double MAX_FRAME_TIME = 0.25;

    explicit GameLoop(int hz);

    void tick(Engine& engine);
    void idle(Engine& engine);
    void step(Engine& engine);

    int advance(double frameTime);
    float alpha() const;

    static void requirePositiveHz(int hz, const char* what);

private:
    bool begin(Engine& engine);
    void resetClock();

    double fixedDt_;
    double last_ = -1.0;
    double accumulator_ = 0.0;
};

}
