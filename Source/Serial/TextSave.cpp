#include "Serial/TextSave.hpp"

#include "Serial/NumberText.hpp"

#include <charconv>

namespace cinder::Serial {
namespace {

std::string leaf(const cinder::Scene::PropValue& value);

std::string sequence(const cinder::Scene::PropSeq& values) {
    std::string out;
    for (const cinder::Scene::PropValue& value : values) {
        if (!out.empty()) out.push_back(' ');
        out += leaf(value);
    }
    return out;
}

std::string leaf(const cinder::Scene::PropValue& value) {
    using namespace cinder::Scene;

    if (value.is<std::int64_t>()) {
        return integerText(value.as<std::int64_t>());
    }
    if (value.is<double>()) {
        return realText(value.as<double>());
    }
    if (value.is<std::string>()) return TextSave::quoted(value.as<std::string>());
    if (value.is<bool>()) return value.as<bool>() ? "true" : "false";
    return "0";
}

}

bool TextSave::enterRecord(std::string_view name) {
    indent();
    out_ += name;
    out_ += " {\n";
    depth_++;
    return true;
}

void TextSave::leaveRecord() {
    depth_--;
    indent();
    out_ += "}\n";
}

int TextSave::enterArray(std::string_view name, int count) {
    arrays_.push_back(count);
    if (count > 0) enterRecord(name);
    return count;
}

void TextSave::enterItem(int index, std::string_view type) { enterRecord(type); }

void TextSave::leaveArray() {
    const int count = arrays_.back();
    arrays_.pop_back();
    if (count > 0) leaveRecord();
}

std::string TextSave::text(std::string_view name, std::string_view value,
                           std::string_view fallback) {
    if (value != fallback) field(name, quoted(value));
    return std::string(value);
}

cinder::Scene::PropRec TextSave::bag(std::string_view name, const cinder::Scene::PropRec& values) {
    if (values.empty()) return values;
    enterRecord(name);
    rec(values);
    leaveRecord();
    return values;
}

cinder::Scene::RawFields TextSave::raw(const cinder::Scene::RawFields& values) {
    for (const auto& [key, text] : values) field(key, text);
    return values;
}

void TextSave::rec(const cinder::Scene::PropRec& values) {
    for (const auto& [key, value] : values) {
        if (value.is<cinder::Scene::PropRec>()) {
            enterRecord(key);
            rec(value.as<cinder::Scene::PropRec>());
            leaveRecord();
        } else if (value.is<cinder::Scene::PropSeq>()) {
            field(key, sequence(value.as<cinder::Scene::PropSeq>()));
        } else {
            field(key, leaf(value));
        }
    }
}

void TextSave::field(std::string_view name, std::string_view value) {
    indent();
    out_ += name;
    out_.push_back(' ');
    out_ += value;
    out_.push_back('\n');
}

void TextSave::indent() { out_.append(static_cast<std::size_t>(depth_) * 4, ' '); }

std::string TextSave::quoted(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');

    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f) {
                    const int code = 1000 + static_cast<unsigned char>(c);
                    char buffer[8];
                    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), code);
                    out.push_back('\\');
                    out.append(buffer + 1, end);
                } else {
                    out.push_back(c);
                }
        }
    }

    out.push_back('"');
    return out;
}

}
