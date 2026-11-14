#include "test_helpers.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include <stdexcept>
#include <utility>

namespace dsd::ductsystem::test {
namespace {

TEST(SheetMetalNodeTest, RejectsNullComponent) {
    EXPECT_THROW((void)(SheetMetalNode{nullptr}), std::invalid_argument);
}

TEST(SheetMetalNodeTest, AttachingChildSharesOwnershipAndSetsWeakParent) {
    auto root = duct_node(0);
    auto child = duct_node(1);
    auto* child_address = child.get();

    auto& attached = root->add_child(child);

    EXPECT_TRUE(child);
    EXPECT_EQ(&attached, child_address);
    EXPECT_EQ(attached.parent().lock().get(), root.get());
    ASSERT_EQ(root->children().size(), 1);
    EXPECT_EQ(root->children().front().get(), child_address);
    EXPECT_TRUE(root->parent().expired());
}

TEST(SheetMetalNodeTest, DistinguishesDuctFromFitting) {
    const auto duct = duct_node(0);
    const SheetMetalNode reducer{std::make_unique<RectangularReducer>(1, 100, pressure, material)};
    EXPECT_FALSE(duct->is_duct_fitting());
    EXPECT_TRUE(reducer.is_duct_fitting());
}

TEST(SheetMetalNodeTest, RejectsNullChild) {
    const auto root = duct_node(0);
    EXPECT_THROW(root->add_child(nullptr), std::invalid_argument);
    EXPECT_TRUE(root->children().empty());
}

TEST(SheetMetalNodeTest, RejectsSelfAttachmentWithoutChangingLinks) {
    auto root = duct_node(0);
    auto* original = root.get();
    EXPECT_THROW(root->add_child(root), std::invalid_argument);
    ASSERT_EQ(root.get(), original);
    EXPECT_TRUE(root->children().empty());
    EXPECT_TRUE(root->parent().expired());
}

TEST(SheetMetalNodeTest, RejectsAncestorAttachmentWithoutChangingLinks) {
    auto root = duct_node(0);
    auto& child = root->add_child(duct_node(1));
    auto* original = root.get();

    EXPECT_THROW(child.add_child(root), std::invalid_argument);

    ASSERT_EQ(root.get(), original);
    EXPECT_EQ(child.parent().lock().get(), root.get());
    EXPECT_TRUE(child.children().empty());
}

TEST(SheetMetalNodeTest, InsertionPreservesSiblingOrderAndParentLinks) {
    auto root = duct_node(0);
    auto& child = root->add_child(duct_node(1));
    auto& sibling = root->add_child(duct_node(2));
    auto reducer = std::make_shared<SheetMetalNode>(
        std::make_unique<RectangularReducer>(3, 100, pressure, material));

    auto& inserted = root->insert_between(child, reducer);

    EXPECT_TRUE(reducer);
    ASSERT_EQ(root->children().size(), 2);
    EXPECT_EQ(root->children()[0].get(), &inserted);
    EXPECT_EQ(root->children()[1].get(), &sibling);
    EXPECT_EQ(inserted.parent().lock().get(), root.get());
    ASSERT_EQ(inserted.children().size(), 1);
    EXPECT_EQ(inserted.children()[0].get(), &child);
    EXPECT_EQ(child.parent().lock().get(), &inserted);
    EXPECT_EQ(sibling.parent().lock().get(), root.get());
}

TEST(SheetMetalNodeTest, RejectsInsertionForAnUnrelatedChild) {
    auto root = duct_node(0);
    auto unrelated = duct_node(1);
    auto intermediate = duct_node(2);

    EXPECT_THROW(root->insert_between(*unrelated, intermediate), std::invalid_argument);

    EXPECT_TRUE(intermediate);
    EXPECT_TRUE(unrelated->parent().expired());
    EXPECT_TRUE(root->children().empty());
}

TEST(SheetMetalNodeTest, RejectsNonemptyIntermediateWithoutChangingTree) {
    auto root = duct_node(0);
    auto& child = root->add_child(duct_node(1));
    auto intermediate = duct_node(2);
    intermediate->add_child(duct_node(3));

    EXPECT_THROW(root->insert_between(child, intermediate), std::invalid_argument);

    ASSERT_TRUE(intermediate);
    EXPECT_EQ(child.parent().lock().get(), root.get());
    EXPECT_EQ(root->children().front().get(), &child);
    EXPECT_EQ(intermediate->children().size(), 1);
}

TEST(SheetMetalNodeTest, RejectsNullIntermediateWithoutChangingTree) {
    auto root = duct_node(0);
    auto& child = root->add_child(duct_node(1));
    EXPECT_THROW(root->insert_between(child, nullptr), std::invalid_argument);
    EXPECT_EQ(child.parent().lock().get(), root.get());
    EXPECT_EQ(root->children().front().get(), &child);
}

TEST(SheetMetalNodeTest, RejectsChildAlreadyAttachedToAnotherParent) {
    auto original_parent = duct_node(0);
    auto other_parent = duct_node(1);
    auto child = duct_node(2);
    original_parent->add_child(child);

    EXPECT_THROW(other_parent->add_child(child), std::invalid_argument);

    EXPECT_EQ(child->parent().lock(), original_parent);
    EXPECT_TRUE(other_parent->children().empty());
}

TEST(SheetMetalNodeTest, ParentWeakPointerExpiresWhileRetainedChildSurvives) {
    auto parent = duct_node(0);
    auto child = duct_node(1);
    parent->add_child(child);
    const auto parent_link = child->parent();

    parent.reset();

    EXPECT_TRUE(parent_link.expired());
    EXPECT_TRUE(child->parent().expired());
    EXPECT_EQ(child->component().id, 1);
}

TEST(SheetMetalNodeTest, RetainedChildCanAttachToNewParentAfterOldParentExpires) {
    auto child = duct_node(1);
    {
        auto original_parent = duct_node(0);
        original_parent->add_child(child);
    }
    auto new_parent = duct_node(2);

    new_parent->add_child(child);

    EXPECT_EQ(child->parent().lock(), new_parent);
}

TEST(SheetMetalNodeTest, WeakParentDoesNotKeepEntireHierarchyAlive) {
    std::weak_ptr<SheetMetalNode> root_link;
    std::weak_ptr<SheetMetalNode> child_link;
    {
        auto root = duct_node(0);
        auto child = duct_node(1);
        root->add_child(child);
        root_link = root;
        child_link = child;
    }

    EXPECT_TRUE(root_link.expired());
    EXPECT_TRUE(child_link.expired());
}

TEST(SheetMetalNodeTest, RejectsUnmanagedParentBeforeChangingChildLinks) {
    SheetMetalNode parent{std::make_unique<RectangularDuct>(0, 100, pressure, material)};
    auto child = duct_node(1);

    EXPECT_THROW(parent.add_child(child), std::invalid_argument);

    EXPECT_TRUE(parent.children().empty());
    EXPECT_TRUE(child->parent().expired());
}

TEST(SheetMetalNodeTest, RejectsAlreadyAttachedIntermediateWithoutChangingEitherParent) {
    auto root = duct_node(0);
    auto& child = root->add_child(duct_node(1));
    auto other_parent = duct_node(2);
    auto intermediate = duct_node(3);
    other_parent->add_child(intermediate);

    EXPECT_THROW(root->insert_between(child, intermediate), std::invalid_argument);

    EXPECT_EQ(child.parent().lock(), root);
    EXPECT_EQ(intermediate->parent().lock(), other_parent);
}

} // namespace
} // namespace dsd::ductsystem::test
