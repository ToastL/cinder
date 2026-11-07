#include "Serial/IniSave.hpp"

#include "Platform/Log.hpp"

#include <stdexcept>

namespace cinder::Serial {
namespace {

[[noreturn]] void unsupported(const char* what) {
    throw std::logic_error(std::string("INI has no ") + what);
}

}

bool IniSave::enterRecord(std::string_view name) {
    if (inSection_) unsupported("nested sections");
    section_ = name;
    inSection_ = true;
    headed_ = false;
    return true;
}

void IniSave::leaveRecord() {
    section_.clear();
    inSection_ = false;
}

int IniSave::enterArray(std::string_view name, int count) { unsupported("arrays"); }

void IniSave::enterItem(int index, std::string_view type) { unsupported("arrays"); }

std::string IniSave::itemType(int index) { unsupported("arrays"); }

void IniSave::leaveItem() { unsupported("arrays"); }

void IniSave::leaveArray() { unsupported("arrays"); }

std::string IniSave::text(std::string_view name, std::string_view value,
                          std::string_view fallback) {
    if (value != fallback) field(name, value);
    return std::string(value);
}

cinder::Scene::RawFields IniSave::raw(const cinder::Scene::RawFields& values) {
    unsupported("verbatim fields");
}

cinder::Scene::PropRec IniSave::bag(std::string_view name, const cinder::Scene::PropRec& values) {
    unsupported("attribute bags");
}

void IniSave::field(std::string_view name, std::string_view value) {
    if (value.find_first_of("\r\n") != std::string_view::npos) {
        cinder::Platform::logError("[serial] %.*s dropped: an INI value cannot hold a line break\n",
                                   static_cast<int>(name.size()), name.data());
        return;
    }
    if (!inSection_ && sectioned_) {
        throw std::logic_error("INI keys outside a section must come before the first section");
    }

    if (inSection_ && !headed_) {
        if (!out_.empty()) out_.push_back('\n');
        out_.push_back('[');
        out_ += section_;
        out_ += "]\n";
        headed_ = true;
        sectioned_ = true;
    }

    out_ += name;
    out_.push_back('=');
    out_ += value;
    out_.push_back('\n');
}

}
