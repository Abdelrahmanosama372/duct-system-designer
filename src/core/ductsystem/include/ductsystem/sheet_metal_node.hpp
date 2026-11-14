#pragma once

#include "ductsystem/models/sheet_metal_component.hpp"
#include <memory>
#include <vector>

namespace dsd::ductsystem {

class SheetMetalNode : public std::enable_shared_from_this<SheetMetalNode> {
public:
    explicit SheetMetalNode(std::unique_ptr<SheetMetalComponent> component);
    SheetMetalNode(const SheetMetalNode&) = delete;
    SheetMetalNode& operator=(const SheetMetalNode&) = delete;
    SheetMetalNode(SheetMetalNode&&) = delete;
    SheetMetalNode& operator=(SheetMetalNode&&) = delete;

    [[nodiscard]] SheetMetalComponent& component() { return *component_; }
    [[nodiscard]] const SheetMetalComponent& component() const { return *component_; }
    [[nodiscard]] std::weak_ptr<SheetMetalNode> parent() { return parent_; }
    [[nodiscard]] std::weak_ptr<const SheetMetalNode> parent() const { return parent_; }
    [[nodiscard]] const std::vector<std::shared_ptr<SheetMetalNode>>& children() const {
        return children_;
    }
    [[nodiscard]] bool is_duct_fitting() const;

    // Nodes used as parents must be owned by shared_ptr.
    SheetMetalNode& add_child(const std::shared_ptr<SheetMetalNode>& child);
    SheetMetalNode& insert_between(SheetMetalNode& child,
                                   const std::shared_ptr<SheetMetalNode>& intermediate);

private:
    std::unique_ptr<SheetMetalComponent> component_;
    std::weak_ptr<SheetMetalNode> parent_;
    std::vector<std::shared_ptr<SheetMetalNode>> children_;
};

} // namespace dsd::ductsystem
