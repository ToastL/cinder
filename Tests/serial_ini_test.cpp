#include <doctest/doctest.h>

#include "platform/Log.hpp"
#include "serial/IniLoad.hpp"
#include "serial/IniSave.hpp"

#include <clocale>
#include <stdexcept>
#include <string>

using cinder::serial::IniLoad;
using cinder::serial::IniSave;

namespace {

struct Errors {
    int count = 0;

    Errors() {
        cinder::platform::setLogSink([this](cinder::platform::LogLevel, std::string_view) { count++; });
    }

    ~Errors() { cinder::platform::setLogSink(nullptr); }

    Errors(const Errors&) = delete;
    Errors& operator=(const Errors&) = delete;
};

}

TEST_CASE("ini keys land under their section and defaults are omitted") {
    IniSave save;
    save.enterRecord("Game");
    save.text("title", "Cinder", "Cinder");
    save.text("scene", "Levels/one.scene", "Scenes/main.scene");
    save.leaveRecord();
    save.enterRecord("Window");
    save.integer("width", 1280, 1280);
    save.leaveRecord();
    save.enterRecord("Loop");
    save.integer("fixedHz", 30, 60);
    save.flag("vsync", false, true);
    save.leaveRecord();

    CHECK(save.text() == "[Game]\nscene=Levels/one.scene\n\n[Loop]\nfixedHz=30\nvsync=false\n");
}

TEST_CASE("ini values read back through the same calls") {
    IniLoad load = IniLoad::parse(
            "[Game]\nscene=Levels/one.scene\n\n[Loop]\nfixedHz=30\nvsync=false\nspawn=1 2.5 -3\n");

    REQUIRE(load.enterRecord("Game"));
    CHECK(load.text("scene", "", "fallback") == "Levels/one.scene");
    CHECK(load.text("title", "", "Cinder") == "Cinder");
    load.leaveRecord();

    REQUIRE(load.enterRecord("Loop"));
    CHECK(load.integer("fixedHz", 0, 60) == 30);
    CHECK_FALSE(load.flag("vsync", true, true));

    float spawn[3]{};
    const float none[3]{9, 9, 9};
    load.vector("spawn", spawn, none, 3);
    CHECK(spawn[0] == 1.0f);
    CHECK(spawn[1] == 2.5f);
    CHECK(spawn[2] == -3.0f);
    load.leaveRecord();

    CHECK_FALSE(load.enterRecord("Window"));
}

TEST_CASE("ini comments, blank lines and spacing are tolerated") {
    IniLoad load = IniLoad::parse("; comment\r\n# another\r\n\r\n[ Game ]\r\n  title =  Two Words  \r\n");

    REQUIRE(load.enterRecord("Game"));
    CHECK(load.text("title", "", "") == "Two Words");
}

TEST_CASE("a malformed ini number falls back and logs") {
    Errors errors;
    IniLoad load = IniLoad::parse("[Window]\nwidth=wide\nheight=720.5\nsize=4 x\n");

    REQUIRE(load.enterRecord("Window"));
    CHECK(load.integer("width", 0, 1280) == 1280);
    CHECK(load.integer("height", 0, 720) == 720);

    float size[2]{};
    const float fallback[2]{7, 8};
    load.vector("size", size, fallback, 2);
    CHECK(size[0] == 4.0f);
    CHECK(size[1] == 8.0f);

    CHECK(errors.count == 3);
}

TEST_CASE("a line break cannot be saved into an ini value") {
    Errors errors;
    IniSave save;
    save.enterRecord("Game");
    save.text("title", "two\nlines", "");
    save.leaveRecord();

    CHECK(save.text().empty());
    CHECK(errors.count == 1);
}

TEST_CASE("ini has no arrays, bags, nested sections or keys after a section") {
    IniSave save;
    CHECK_THROWS_AS(save.enterArray("nodes", 1), std::logic_error);
    CHECK_THROWS_AS(save.bag("attributes", {}), std::logic_error);
    save.enterRecord("Game");
    CHECK_THROWS_AS(save.enterRecord("Inner"), std::logic_error);
    save.integer("fixedHz", 30, 60);
    save.leaveRecord();
    CHECK_THROWS_AS(save.integer("loose", 1, 0), std::logic_error);

    IniLoad load = IniLoad::parse("[Game]\n");
    CHECK_THROWS_AS(load.enterArray("nodes", 0), std::logic_error);
    REQUIRE(load.enterRecord("Game"));
    CHECK_THROWS_AS(load.enterRecord("Game"), std::logic_error);
}

TEST_CASE("ini numbers do not depend on the default locale") {
    const float zero[1]{0};
    float fov[1]{0.5f};
    float back[1]{};

    const char* before = std::setlocale(LC_ALL, nullptr);
    const std::string saved = before != nullptr ? before : "C";

    std::setlocale(LC_ALL, "tr_TR.UTF-8");
    IniSave save;
    save.enterRecord("Camera");
    save.vector("fov", fov, zero, 1);
    save.leaveRecord();
    IniLoad load = IniLoad::parse(save.text());
    const bool entered = load.enterRecord("Camera");
    if (entered) load.vector("fov", back, zero, 1);
    std::setlocale(LC_ALL, saved.c_str());

    CHECK(save.text() == "[Camera]\nfov=0.5\n");
    CHECK(entered);
    CHECK(back[0] == 0.5f);
}
