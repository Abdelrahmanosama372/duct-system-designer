#pragma once

#include "ductsystem/sheet_metal_node.hpp"

namespace dsd::ductsystem {

class DuctFittingTree {
public:
    explicit DuctFittingTree(std::shared_ptr<SheetMetalNode> root);
    DuctFittingTree(const DuctFittingTree&) = delete;
    DuctFittingTree& operator=(const DuctFittingTree&) = delete;
    DuctFittingTree(DuctFittingTree&&) noexcept = default;
    DuctFittingTree& operator=(DuctFittingTree&&) noexcept = default;

    [[nodiscard]] SheetMetalNode& root() { return *root_; }
    [[nodiscard]] const SheetMetalNode& root() const { return *root_; }
    [[nodiscard]] std::vector<SheetMetalNode*> level_order_traversal();
    [[nodiscard]] std::vector<const SheetMetalNode*> level_order_traversal() const;
    [[nodiscard]] std::vector<SheetMetalNode*> preorder_traversal();
    [[nodiscard]] std::vector<const SheetMetalNode*> preorder_traversal() const;
    [[nodiscard]] std::vector<SheetMetalNode*> connected_components();
    [[nodiscard]] std::vector<const SheetMetalNode*> connected_components() const;

private:
    std::shared_ptr<SheetMetalNode> root_;
};

} // namespace dsd::ductsystem
