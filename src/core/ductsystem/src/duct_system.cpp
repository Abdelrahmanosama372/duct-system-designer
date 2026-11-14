#include "ductsystem/duct_system.hpp"
#include <utility>

namespace dsd::ductsystem {

DuctSystem::DuctSystem(std::string name, DuctFittingTree tree,
                       DuctSystemConfig config)
    : system_name_(std::move(name)), tree_(std::move(tree)),
      config_(config) {}

std::vector<SheetMetalNode*> DuctSystem::level_order_traversal() {
    return tree_.level_order_traversal();
}
std::vector<const SheetMetalNode*> DuctSystem::level_order_traversal() const {
    return tree_.level_order_traversal();
}
std::vector<SheetMetalNode*> DuctSystem::preorder_traversal() {
    return tree_.preorder_traversal();
}
std::vector<const SheetMetalNode*> DuctSystem::preorder_traversal() const {
    return tree_.preorder_traversal();
}

} // namespace dsd::ductsystem
