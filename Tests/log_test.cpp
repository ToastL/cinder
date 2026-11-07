#include <doctest/doctest.h>

#include "Platform/Log.hpp"

#include <string>
#include <vector>

using cinder::Platform::LogLevel;

namespace {

struct Captured {
    LogLevel level;
    std::string text;
};

struct Sink {
    std::vector<Captured> lines;

    Sink() {
        cinder::Platform::setLogSink([this](LogLevel level, std::string_view text) {
            lines.push_back(Captured{level, std::string(text)});
        });
    }

    ~Sink() { cinder::Platform::setLogSink(nullptr); }

    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;
};

}

TEST_CASE("the sink receives formatted lines without the trailing newline") {
    Sink sink;
    cinder::Platform::logInfo("[module] loaded %s\n", "game.dylib");

    REQUIRE(sink.lines.size() == 1);
    CHECK(sink.lines[0].text == "[module] loaded game.dylib");
    CHECK(sink.lines[0].level == LogLevel::Info);
}

TEST_CASE("errors carry their level") {
    Sink sink;
    cinder::Platform::logError("[build] %s:%d: %s\n", "Riser.cpp", 4, "bad");

    REQUIRE(sink.lines.size() == 1);
    CHECK(sink.lines[0].text == "[build] Riser.cpp:4: bad");
    CHECK(sink.lines[0].level == LogLevel::Error);
}

TEST_CASE("a line longer than any stack buffer survives intact") {
    Sink sink;
    const std::string payload(4096, 'x');
    cinder::Platform::logInfo("[test] %s\n", payload.c_str());

    REQUIRE(sink.lines.size() == 1);
    CHECK(sink.lines[0].text.size() == payload.size() + 7);
}

TEST_CASE("clearing the sink stops delivery") {
    {
        Sink sink;
        cinder::Platform::logInfo("[test] one\n");
        CHECK(sink.lines.size() == 1);
    }

    cinder::Platform::logInfo("[test] two\n");

    Sink again;
    CHECK(again.lines.empty());
}
