#include "test_helpers.hpp"
#include "ductsystem/duct_fitting_tree.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace dsd::ductsystem::test {
namespace {

DuctFittingTree branched_tree() {
    auto root = duct_node(0);
    auto& first = root->add_child(duct_node(1));
    auto& second = root->add_child(duct_node(2));
    first.add_child(duct_node(3));
    first.add_child(duct_node(4));
    second.add_child(duct_node(5));
    return DuctFittingTree{std::move(root)};
}

template<class Node>
std::vector<int> identifiers(const std::vector<Node*>& nodes) {
    std::vector<int> result;
    for (const auto* node : nodes) result.push_back(node->component().id);
    return result;
}

static_assert(!std::is_copy_constructible_v<SheetMetalNode>);
static_assert(!std::is_move_constructible_v<SheetMetalNode>);
static_assert(!std::is_copy_constructible_v<DuctFittingTree>);
static_assert(std::is_move_constructible_v<DuctFittingTree>);
static_assert(std::is_same_v<decltype(std::declval<const DuctFittingTree&>().root()),
                             const SheetMetalNode&>);

TEST(DuctFittingTreeTest, RejectsNullRoot) {
    EXPECT_THROW((void)(DuctFittingTree{nullptr}), std::invalid_argument);
}

TEST(DuctFittingTreeTest, RejectsRootAlreadyAttachedToParent) {
    auto parent = duct_node(0);
    auto child = duct_node(1);
    parent->add_child(child);

    EXPECT_THROW((void)(DuctFittingTree{child}), std::invalid_argument);

    EXPECT_EQ(child->parent().lock(), parent);
}

TEST(DuctFittingTreeTest, SingleNodeIsIncludedInBothTraversals) {
    DuctFittingTree tree{duct_node(7)};
    EXPECT_EQ(tree.level_order_traversal(), (std::vector<SheetMetalNode*>{&tree.root()}));
    EXPECT_EQ(tree.preorder_traversal(), (std::vector<SheetMetalNode*>{&tree.root()}));
}

TEST(DuctFittingTreeTest, LevelOrderVisitsParentsBeforeEachDeeperLevel) {
    auto tree = branched_tree();
    EXPECT_EQ(identifiers(tree.level_order_traversal()), (std::vector<int>{0, 1, 2, 3, 4, 5}));
    EXPECT_EQ(tree.connected_components(), tree.level_order_traversal());
}

TEST(DuctFittingTreeTest, PreorderCompletesEachBranchBeforeNextSibling) {
    const auto tree = branched_tree();
    EXPECT_EQ(identifiers(tree.preorder_traversal()), (std::vector<int>{0, 1, 3, 4, 2, 5}));
    EXPECT_EQ(identifiers(tree.level_order_traversal()), (std::vector<int>{0, 1, 2, 3, 4, 5}));
    EXPECT_EQ(tree.connected_components(), tree.level_order_traversal());
}

TEST(DuctFittingTreeTest, BothTraversalsRejectDuplicateComponentIdentifiers) {
    auto root = duct_node(0);
    root->add_child(duct_node(0));
    DuctFittingTree tree{std::move(root)};
    EXPECT_THROW((void)tree.level_order_traversal(), std::invalid_argument);
    EXPECT_THROW((void)tree.preorder_traversal(), std::invalid_argument);
}

TEST(DuctFittingTreeTest, MovingTreePreservesNodeAddressesAndParentLinks) {
    auto original = branched_tree();
    const auto before = original.level_order_traversal();
    auto moved = std::move(original);
    EXPECT_EQ(moved.level_order_traversal(), before);
    EXPECT_EQ(before[1]->parent().lock().get(), &moved.root());
}

} // namespace
} // namespace dsd::ductsystem::test
