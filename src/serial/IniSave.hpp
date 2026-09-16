#pragma once

#include "serial/Archive.hpp"

#include <string>

namespace cinder::serial {

class IniSave : public Archive {
public:
    const std::string& text() const { return out_; }

    bool loading() const override { return false; }

    bool enterRecord(std::string_view name) override;
    void leaveRecord() override;

    int enterArray(std::string_view name, int count) override;
    void enterItem(int index, std::string_view type) override;
    std::string itemType(int index) override;
    void leaveItem() override;
    void leaveArray() override;

    int integer(std::string_view name, int value, int fallback) override;
    bool flag(std::string_view name, bool value, bool fallback) override;
    std::string text(std::string_view name, std::string_view value,
                     std::string_view fallback) override;
    void vector(std::string_view name, float* values, const float* fallback, int arity) override;

    cinder::scene::PropRec bag(std::string_view name, const cinder::scene::PropRec& values) override;

private:
    void field(std::string_view name, std::string_view value);

    std::string out_;
    std::string section_;
    bool inSection_ = false;
    bool headed_ = false;
    bool sectioned_ = false;
};

}
