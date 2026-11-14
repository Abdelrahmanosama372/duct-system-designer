#include "ductsystem/sheet_metal_node.hpp"
#include "ductsystem/models/rectangular/rectangular_duct_fitting.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace dsd::ductsystem {

SheetMetalNode::SheetMetalNode(std::unique_ptr<SheetMetalComponent> component)
    : component_(std::move(component)) {
    if (!component_) throw std::invalid_argument("Node requires a component");
}

bool SheetMetalNode::is_duct_fitting() const {
    return dynamic_cast<const RectangularDuctFitting*>(component_.get()) != nullptr;
}

SheetMetalNode& SheetMetalNode::add_child(const std::shared_ptr<SheetMetalNode>& child) {
    if (!child) throw std::invalid_argument("Child must not be null");
    if (!child->parent_.expired()) {
        throw std::invalid_argument("Child is already attached to a parent");
    }
    const auto self = weak_from_this().lock();
    if (!self) throw std::invalid_argument("Parent node must be owned by shared_ptr");
    for (auto ancestor = self; ancestor; ancestor = ancestor->parent_.lock()) {
        if (ancestor == child) {
            throw std::invalid_argument("Attaching this child would create a cycle");
        }
    }
    children_.push_back(child);
    child->parent_ = self;
    return *child;
}

SheetMetalNode& SheetMetalNode::insert_between(
    SheetMetalNode& child, const std::shared_ptr<SheetMetalNode>& intermediate) {
    const auto position = std::find_if(children_.begin(), children_.end(),
        [&child](const auto& candidate) { return candidate.get() == &child; });
    if (child.parent_.lock().get() != this || position == children_.end()) {
        throw std::invalid_argument("Child is not attached to this parent");
    }
    if (!intermediate || !intermediate->parent_.expired() || !intermediate->children_.empty()) {
        throw std::invalid_argument("Intermediate node must be unattached and have no children");
    }
    const auto self = weak_from_this().lock();
    if (!self) throw std::invalid_argument("Parent node must be owned by shared_ptr");
    for (auto ancestor = self; ancestor; ancestor = ancestor->parent_.lock()) {
        if (ancestor == intermediate || &child == intermediate.get()) {
            throw std::invalid_argument("Intermediate node would create a cycle");
        }
    }
    // Allocate before modifying links so allocation failure leaves the tree intact.
    intermediate->children_.push_back(*position);
    child.parent_ = intermediate;
    intermediate->parent_ = self;
    *position = intermediate;
    return *intermediate;
}

} // namespace dsd::ductsystem
