#include "ductsystem/duct_fitting_tree.hpp"
#include <deque>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace dsd::ductsystem {
namespace {

template<class Node>
std::vector<Node*> traverse(Node& root, bool depth_first) {
    std::deque<Node*> pending{&root};
    std::unordered_set<const SheetMetalNode*> visited;
    std::unordered_set<int> identifiers;
    std::vector<Node*> result;
    while (!pending.empty()) {
        Node* node;
        if (depth_first) {
            node = pending.back();
            pending.pop_back();
        } else {
            node = pending.front();
            pending.pop_front();
        }
        if (!visited.insert(node).second) {
            throw std::invalid_argument("Component tree contains a cycle or shared node");
        }
        if (!identifiers.insert(node->component().id).second) {
            throw std::invalid_argument("Component IDs must be unique within a system");
        }
        for (const auto& child : node->children()) {
            if (child->parent().lock().get() != node) {
                throw std::invalid_argument("Child and parent links are inconsistent");
            }
        }
        result.push_back(node);
        if (depth_first) {
            for (auto it = node->children().rbegin(); it != node->children().rend(); ++it) {
                pending.push_back(it->get());
            }
        } else {
            for (const auto& child : node->children()) pending.push_back(child.get());
        }
    }
    return result;
}

} // namespace

DuctFittingTree::DuctFittingTree(std::shared_ptr<SheetMetalNode> root)
    : root_(std::move(root)) {
    if (!root_) throw std::invalid_argument("Tree root must not be null");
    if (!root_->parent().expired()) throw std::invalid_argument("Tree root must not have a parent");
}

std::vector<SheetMetalNode*> DuctFittingTree::level_order_traversal() {
    return traverse(*root_, false);
}
std::vector<const SheetMetalNode*> DuctFittingTree::level_order_traversal() const {
    return traverse(static_cast<const SheetMetalNode&>(*root_), false);
}
std::vector<SheetMetalNode*> DuctFittingTree::preorder_traversal() {
    return traverse(*root_, true);
}
std::vector<const SheetMetalNode*> DuctFittingTree::preorder_traversal() const {
    return traverse(static_cast<const SheetMetalNode&>(*root_), true);
}
std::vector<SheetMetalNode*> DuctFittingTree::connected_components() {
    return level_order_traversal();
}
std::vector<const SheetMetalNode*> DuctFittingTree::connected_components() const {
    return level_order_traversal();
}

} // namespace dsd::ductsystem
