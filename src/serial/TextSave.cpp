#include "serial/TextSave.hpp"

#include "serial/NumberText.hpp"

#include <charconv>

namespace cinder::serial {
namespace {

std::string leaf(const cinder::scene::PropValue& value);

std::string sequence(const cinder::scene::PropSeq& values) {
    std::string out;
    for (const cinder::scene::PropValue& value : values) {
        if (!out.empty()) out.push_back(' ');
        out += leaf(value);
    }
    return out;
}

std::string leaf(const cinder::scene::PropValue& value) {
    using namespace cinder::scene;

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

int TextSave::integer(std::string_view name, int value, int fallback) {
    if (value != fallback) {
        field(name, integerText(value));
    }
    return value;
}

bool TextSave::flag(std::string_view name, bool value, bool fallback) {
    if (value != fallback) field(name, value ? "true" : "false");
    return value;
}

std::string TextSave::text(std::string_view name, std::string_view value,
                           std::string_view fallback) {
    if (value != fallback) field(name, quoted(value));
    return std::string(value);
}

void TextSave::vector(std::string_view name, float* values, const float* fallback, int arity) {
    bool same = true;
    for (int i = 0; i < arity; ++i) same = same && values[i] == fallback[i];
    if (same) return;

    field(name, vectorText({values, static_cast<std::size_t>(arity)}));
}

cinder::scene::PropRec TextSave::bag(std::string_view name, const cinder::scene::PropRec& values) {
    if (values.empty()) return values;
    enterRecord(name);
    rec(values);
    leaveRecord();
    return values;
}

void TextSave::rec(const cinder::scene::PropRec& values) {
    for (const auto& [key, value] : values) {
        if (value.is<cinder::scene::PropRec>()) {
            enterRecord(key);
            rec(value.as<cinder::scene::PropRec>());
            leaveRecord();
        } else if (value.is<cinder::scene::PropSeq>()) {
            field(key, sequence(value.as<cinder::scene::PropSeq>()));
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
