#pragma once

#include "Serial/DeltaSave.hpp"

#include <string>

namespace cinder::Serial {

class IniSave : public DeltaSave {
public:
    bool enterRecord(std::string_view name) override;
    void leaveRecord() override;

    int enterArray(std::string_view name, int count) override;
    void enterItem(int index, std::string_view type) override;
    std::string itemType(int index) override;
    void leaveItem() override;
    void leaveArray() override;

    using DeltaSave::text;
    std::string text(std::string_view name, std::string_view value,
                     std::string_view fallback) override;

    cinder::Scene::PropRec bag(std::string_view name, const cinder::Scene::PropRec& values) override;
    cinder::Scene::RawFields raw(const cinder::Scene::RawFields& values) override;

private:
    void field(std::string_view name, std::string_view value) override;

    std::string section_;
    bool inSection_ = false;
    bool headed_ = false;
    bool sectioned_ = false;
};

}
