#include <doctest/doctest.h>

#include "dev/Packager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

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

TEST_CASE("cancelling stops the build's whole process group, and so does destroying the packager") {
    const Platform host = cinder::dev::hostPlatform();
    if (!cinder::dev::platformSupported(host)) return;

    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "cinder_packager_test";
    std::filesystem::create_directories(dir);
    const std::filesystem::path slow = dir / "cmake";
    {
        std::ofstream out(slow);
        out << "#!/bin/sh\nsleep 30 &\nwait\n";
    }
    std::filesystem::permissions(slow, std::filesystem::perms::owner_all);

    PackageTools slowTools = tools();
    slowTools.cmake = slow;
    const auto settle = [](const Packager& packager) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (packager.busy() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    Packager packager(dir / "Game", {}, slowTools);
    REQUIRE(packager.start(host));
    CHECK(packager.busy());
    CHECK(packager.current() == host);
    packager.cancel();
    settle(packager);
    CHECK_FALSE(packager.busy());
    packager.update();

    const auto before = std::chrono::steady_clock::now();
    {
        Packager doomed(dir / "Game", {}, slowTools);
        REQUIRE(doomed.start(host));
    }
    CHECK(std::chrono::steady_clock::now() - before < std::chrono::seconds(5));
    std::filesystem::remove_all(dir);
}
