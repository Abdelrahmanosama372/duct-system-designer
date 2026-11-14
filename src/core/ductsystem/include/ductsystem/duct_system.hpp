#pragma once

#include "ductsystem/duct_fitting_tree.hpp"
#include "ductsystem/duct_system_config.hpp"
#include <string>

namespace dsd::ductsystem {

class DuctSystem {
public:
    DuctSystem(std::string system_name, DuctFittingTree tree,
               DuctSystemConfig config);

    [[nodiscard]] const std::string& system_name() const { return system_name_; }
    [[nodiscard]] const DuctSystemConfig& config() const { return config_; }
    [[nodiscard]] DuctFittingTree& dftree() { return tree_; }
    [[nodiscard]] const DuctFittingTree& dftree() const { return tree_; }

    [[nodiscard]] std::vector<SheetMetalNode*> level_order_traversal();
    [[nodiscard]] std::vector<const SheetMetalNode*> level_order_traversal() const;
    [[nodiscard]] std::vector<SheetMetalNode*> preorder_traversal();
    [[nodiscard]] std::vector<const SheetMetalNode*> preorder_traversal() const;

private:
    std::string system_name_;
    DuctFittingTree tree_;
    DuctSystemConfig config_;
};

} // namespace dsd::ductsystem
