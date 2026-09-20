#include "physics/Broadphase.hpp"

#include <glm/common.hpp>

#include <algorithm>
#include <cstddef>

namespace cinder::physics {

namespace {

Bounds merged(const Bounds& a, const Bounds& b) {
    return {glm::min(a.min, b.min), glm::max(a.max, b.max)};
}

glm::vec3 centreOf(const Bounds& bounds) { return (bounds.min + bounds.max) * 0.5f; }

}

void Broadphase::build(const std::vector<Bounds>& bounds) {
    nodes_.clear();
    order_.resize(bounds.size());
    for (std::size_t i = 0; i < bounds.size(); ++i) order_[i] = static_cast<int>(i);

    root_ = bounds.empty() ? -1 : split(bounds, 0, static_cast<int>(bounds.size()));
}

int Broadphase::split(const std::vector<Bounds>& bounds, int first, int last) {
    const auto boundsAt = [&bounds, this](int slot) -> const Bounds& {
        return bounds[static_cast<std::size_t>(order_[static_cast<std::size_t>(slot)])];
    };

    Node node;
    node.bounds = boundsAt(first);
    for (int i = first + 1; i < last; ++i) node.bounds = merged(node.bounds, boundsAt(i));
    node.first = first;
    node.last = last;

    const int index = static_cast<int>(nodes_.size());
    nodes_.push_back(node);
    if (last - first <= LEAF_SIZE) return index;

    const glm::vec3 extent = node.bounds.max - node.bounds.min;
    const int axis = extent.x >= extent.y && extent.x >= extent.z ? 0 : (extent.y >= extent.z ? 1 : 2);
    const int middle = first + (last - first) / 2;

    std::nth_element(order_.begin() + first, order_.begin() + middle, order_.begin() + last,
                     [&bounds, axis](int a, int b) {
                         const float left = centreOf(bounds[static_cast<std::size_t>(a)])[axis];
                         const float right = centreOf(bounds[static_cast<std::size_t>(b)])[axis];
                         return left != right ? left < right : a < b;
                     });

    const int left = split(bounds, first, middle);
    const int right = split(bounds, middle, last);
    nodes_[static_cast<std::size_t>(index)].left = left;
    nodes_[static_cast<std::size_t>(index)].right = right;
    return index;
}

void Broadphase::query(const Bounds& bounds, std::vector<int>& found) const {
    found.clear();
    if (root_ < 0) return;

    stack_.clear();
    stack_.push_back(root_);

    while (!stack_.empty()) {
        const Node& node = nodes_[static_cast<std::size_t>(stack_.back())];
        stack_.pop_back();
        if (!overlaps(node.bounds, bounds)) continue;

        if (node.left < 0) {
            for (int i = node.first; i < node.last; ++i) {
                found.push_back(order_[static_cast<std::size_t>(i)]);
            }
            continue;
        }

        stack_.push_back(node.left);
        stack_.push_back(node.right);
    }
}

int Broadphase::depth() const { return depth(root_); }

int Broadphase::depth(int node) const {
    if (node < 0) return 0;
    const Node& entry = nodes_[static_cast<std::size_t>(node)];
    if (entry.left < 0) return 1;
    return 1 + std::max(depth(entry.left), depth(entry.right));
}

}
