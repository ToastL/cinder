#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cinder::dev {

struct Toolchain {
    std::string compiler;
    std::string abi;
    std::vector<std::string> flags;
    std::vector<std::string> includes;
    std::vector<std::string> defines;

    static std::filesystem::path defaultPath();
    static std::optional<Toolchain> load(const std::filesystem::path& manifest, std::string& error);

    std::vector<std::string> compile(const std::filesystem::path& source, const std::filesystem::path& object,
                                     const std::filesystem::path& depfile) const;
    std::vector<std::string> link(const std::vector<std::filesystem::path>& objects,
                                  const std::filesystem::path& library) const;
    std::string fingerprint() const;
};

}
