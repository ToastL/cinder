#include <doctest/doctest.h>

#include "core/GameConfig.hpp"
#include "core/GameLoop.hpp"

using cinder::core::GameConfig;
using cinder::core::GameLoop;

TEST_CASE("one step per exact period") {
    GameLoop loop(60);
    CHECK(loop.advance(1.0 / 60) == 1);
    CHECK(loop.alpha() == doctest::Approx(0.0f).epsilon(1e-5));
}

TEST_CASE("partial frames accumulate instead of dropping") {
    GameLoop loop(60);
    CHECK(loop.advance(1.0 / 120) == 0);
    CHECK(loop.advance(1.0 / 120) == 1);
}

TEST_CASE("alpha stays in range") {
    GameLoop loop(60);
    for (int i = 0; i < 500; ++i) {
        loop.advance(1.0 / 144);
        const float alpha = loop.alpha();
        CHECK(alpha >= 0.0f);
        CHECK(alpha < 1.0f);
    }
}

TEST_CASE("a stall does not trigger a thousand catch-up steps") {
    GameLoop loop(60);
    CHECK(loop.advance(10.0) == 15);
}

TEST_CASE("step rate tracks real time without drift") {
    GameLoop loop(60);
    int steps = 0;
    for (int i = 0; i < 240; ++i) steps += loop.advance(1.0 / 240);
    CHECK(steps >= 59);
    CHECK(steps <= 61);
}

TEST_CASE("non-positive rates are rejected") {
    CHECK_THROWS_AS(GameLoop(0), std::runtime_error);
    CHECK_THROWS_AS(GameLoop(-60), std::runtime_error);
}

TEST_CASE("config rejects a non-positive fixed hz") {
    CHECK_THROWS_AS(GameConfig("t", 640, 480, "s.lua", 0), std::runtime_error);
}
