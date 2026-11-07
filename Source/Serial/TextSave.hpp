#pragma once

#include "Serial/DeltaSave.hpp"

#include <string>
#include <vector>

namespace cinder::Serial {

class TextSave : public DeltaSave {
public:
    bool enterRecord(std::string_view name) override;
    void leaveRecord() override;

    int enterArray(std::string_view name, int count) override;
    void enterItem(int index, std::string_view type) override;
    std::string itemType(int index) override { return {}; }
    void leaveItem() override { leaveRecord(); }
    void leaveArray() override;

    using DeltaSave::text;
    std::string text(std::string_view name, std::string_view value,
                     std::string_view fallback) override;

    cinder::Scene::PropRec bag(std::string_view name, const cinder::Scene::PropRec& values) override;
    cinder::Scene::RawFields raw(const cinder::Scene::RawFields& values) override;

    static std::string quoted(std::string_view value);

private:
    void rec(const cinder::Scene::PropRec& values);
    void field(std::string_view name, std::string_view value) override;
    void indent();

    std::vector<int> arrays_;
    int depth_ = 0;
};

}
