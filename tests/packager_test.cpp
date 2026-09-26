#include <doctest/doctest.h>

#include "dev/Packager.hpp"

#include <filesystem>
#include <string>

using cinder::dev::PackageTools;
using cinder::dev::Packager;
using cinder::dev::Platform;

namespace {

PackageTools tools() {
    return PackageTools{"/usr/bin/cmake", "/b", "/b/player", "/src/cmake/PackageGame.cmake", "/src/engine",
                        "/b/shaders"};
}

}

TEST_CASE("every platform is listed, and only the host desktop can package today") {
    CHECK(cinder::dev::platforms().size() == 5);
    CHECK(cinder::dev::platformName(Platform::IOS) == "iOS");
    CHECK(cinder::dev::platformName(Platform::Android) == "Android");
    CHECK_FALSE(cinder::dev::platformSupported(Platform::IOS));
    CHECK_FALSE(cinder::dev::platformSupported(Platform::Android));
    CHECK_FALSE(cinder::dev::platformSupported(Platform::Windows));
    const Platform host = cinder::dev::hostPlatform();
    CHECK(cinder::dev::platformSupported(host) == (host == Platform::MacOS || host == Platform::Linux));
}

TEST_CASE("targetPlatforms narrows what a project packages for, and empty means all") {
    const Platform host = cinder::dev::hostPlatform();
    const Packager open("/nowhere/Game", {}, tools());
    CHECK(open.targets(Platform::Android));
    CHECK(open.canPackage(host) == cinder::dev::platformSupported(host));
    CHECK(open.unavailableReason(Platform::IOS) == "iOS is not supported yet");

    const Packager mobile("/nowhere/Game", {"iOS", "Android"}, tools());
    CHECK_FALSE(mobile.targets(host));
    CHECK_FALSE(mobile.canPackage(host));
    CHECK(mobile.outputDir(Platform::Android) == std::filesystem::path("/nowhere/Game/Saved/Builds/Android/Game"));
}

TEST_CASE("the package command builds the player and then stages it with PackageGame") {
    const std::string command = cinder::dev::packageCommand(tools(), Platform::Linux, "/p/My Game", "/p/out's");
    CHECK(command.find("'/usr/bin/cmake' --build '/b' --target player && ") != std::string::npos);
    CHECK(command.find("'-DHOST_PLATFORM=Linux'") != std::string::npos);
    CHECK(command.find("'-DPROJECT_DIR=/p/My Game'") != std::string::npos);
    CHECK(command.find("'-DOUT_DIR=/p/out'\\''s'") != std::string::npos);
    CHECK(command.find(" -P '/src/cmake/PackageGame.cmake') 2>&1") != std::string::npos);
}

TEST_CASE("starting an unavailable platform does nothing") {
    Packager packager("/nowhere/Game", {}, tools());
    CHECK_FALSE(packager.start(Platform::IOS));
    CHECK_FALSE(packager.busy());
    packager.update();
}
