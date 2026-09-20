#pragma once

#include "physics/Geometry.hpp"

#include <vector>

namespace cinder::physics {

class Broadphase {
public:
    static constexpr int LEAF_SIZE = 4;

    void build(const std::vector<Bounds>& bounds);
    void query(const Bounds& bounds, std::vector<int>& found) const;
    int depth() const;

private:
    struct Node {
        Bounds bounds;
        int left = -1;
        int right = -1;
        int first = 0;
        int last = 0;
    };

    int split(const std::vector<Bounds>& bounds, int first, int last);
    int depth(int node) const;

    std::vector<Node> nodes_;
    std::vector<int> order_;
    mutable std::vector<int> stack_;
    int root_ = -1;
};

}
