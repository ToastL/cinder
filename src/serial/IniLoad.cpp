#include "serial/IniLoad.hpp"

#include "platform/Log.hpp"

#include <cctype>
#include <charconv>
#include <stdexcept>

namespace cinder::serial {
namespace {

std::string_view trimmed(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

[[noreturn]] void unsupported(const char* what) {
    throw std::logic_error(std::string("INI has no ") + what);
}

template <class T>
bool parsed(std::string_view token, T& out) {
    const char* end = token.data() + token.size();
    auto [at, error] = std::from_chars(token.data(), end, out);
    return error == std::errc{} && at == end;
}

}

IniLoad IniLoad::parse(const std::string& source) {
    IniLoad archive;
    std::string section;
    archive.sections_[section];

    std::size_t start = 0;
    while (start <= source.size()) {
        std::size_t end = source.find('\n', start);
        if (end == std::string::npos) end = source.size();
        const std::string_view line = trimmed(std::string_view(source).substr(start, end - start));
        start = end + 1;

        if (line.empty() || line.front() == ';' || line.front() == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            section = std::string(trimmed(line.substr(1, line.size() - 2)));
            archive.sections_[section];
            continue;
        }

        const std::size_t equals = line.find('=');
        if (equals == std::string_view::npos) {
            cinder::platform::logError("[serial] ini line without '=' ignored: %.*s\n",
                                       static_cast<int>(line.size()), line.data());
            continue;
        }
        archive.sections_[section][std::string(trimmed(line.substr(0, equals)))] =
                std::string(trimmed(line.substr(equals + 1)));
    }
    return archive;
}

bool IniLoad::enterRecord(std::string_view name) {
    if (inSection_) unsupported("nested sections");
    if (sections_.find(std::string(name)) == sections_.end()) return false;
    section_ = name;
    inSection_ = true;
    return true;
}

void IniLoad::leaveRecord() {
    section_.clear();
    inSection_ = false;
}

int IniLoad::enterArray(std::string_view name, int count) { unsupported("arrays"); }

void IniLoad::enterItem(int index, std::string_view type) { unsupported("arrays"); }

std::string IniLoad::itemType(int index) { unsupported("arrays"); }

void IniLoad::leaveItem() { unsupported("arrays"); }

void IniLoad::leaveArray() { unsupported("arrays"); }

const std::string* IniLoad::field(std::string_view name) const {
    const auto section = sections_.find(section_);
    if (section == sections_.end()) return nullptr;
    const auto found = section->second.find(std::string(name));
    return found == section->second.end() ? nullptr : &found->second;
}

int IniLoad::integer(std::string_view name, int value, int fallback) {
    const std::string* found = field(name);
    if (found == nullptr) return fallback;

    int out = 0;
    if (parsed(*found, out)) return out;
    cinder::platform::logError("[serial] %.*s: \"%s\" is not a whole number\n",
                               static_cast<int>(name.size()), name.data(), found->c_str());
    return fallback;
}

bool IniLoad::flag(std::string_view name, bool value, bool fallback) {
    const std::string* found = field(name);
    if (found == nullptr) return fallback;
    if (*found == "true") return true;
    if (*found == "false") return false;

    cinder::platform::logError("[serial] %.*s: \"%s\" is not true or false\n",
                               static_cast<int>(name.size()), name.data(), found->c_str());
    return fallback;
}

std::string IniLoad::text(std::string_view name, std::string_view value,
                          std::string_view fallback) {
    const std::string* found = field(name);
    return found == nullptr ? std::string(fallback) : *found;
}

void IniLoad::vector(std::string_view name, float* values, const float* fallback, int arity) {
    const std::string* found = field(name);
    std::string_view rest = found == nullptr ? std::string_view() : std::string_view(*found);

    for (int i = 0; i < arity; ++i) {
        rest = trimmed(rest);
        std::size_t split = 0;
        while (split < rest.size() && !std::isspace(static_cast<unsigned char>(rest[split]))) split++;
        const std::string_view token = rest.substr(0, split);
        rest.remove_prefix(split);

        values[i] = fallback[i];
        if (token.empty() || parsed(token, values[i])) continue;

        values[i] = fallback[i];
        cinder::platform::logError("[serial] %.*s: \"%.*s\" is not a number\n",
                                   static_cast<int>(name.size()), name.data(),
                                   static_cast<int>(token.size()), token.data());
    }
}

cinder::scene::PropRec IniLoad::bag(std::string_view name, const cinder::scene::PropRec& values) {
    unsupported("attribute bags");
}

}
