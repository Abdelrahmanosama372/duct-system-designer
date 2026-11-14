#include "test_helpers.hpp"
#include "ductsystem/duct_system.hpp"
#include <stdexcept>
#include <utility>

namespace dsd::ductsystem::test {
namespace {

class SystemTypeTest : public ::testing::TestWithParam<SystemType> {};

TEST_P(SystemTypeTest, ConstructsNamedSystemWithOwnedTreeAndConfiguration) {
    const DuctSystemConfig config{material, pressure, {150, 300, 1.5}, GetParam()};
    const DuctSystem system{"Main", DuctFittingTree{duct_node(0)}, config};

    EXPECT_EQ(system.system_name(), "Main");
    EXPECT_EQ(system.config().system_type, GetParam());
    EXPECT_EQ(system.config().material, material);
    EXPECT_EQ(system.config().pressure_level, pressure);
    EXPECT_EQ(system.config().fittings.takeoff_length, 150);
    EXPECT_EQ(system.config().fittings.reducer_length, 300);
    EXPECT_EQ(system.config().fittings.elbow_inside_radius_ratio, 1.5);
    ASSERT_EQ(system.level_order_traversal().size(), 1);
    EXPECT_EQ(system.level_order_traversal().front(), &system.dftree().root());
}

INSTANTIATE_TEST_SUITE_P(SupportedTypes, SystemTypeTest,
    ::testing::Values(SystemType::Supply, SystemType::Return));

TEST(DuctSystemTest, DelegatesBothTraversalsToItsTree) {
    auto root = duct_node(0);
    auto& first = root->add_child(duct_node(1));
    root->add_child(duct_node(2));
    first.add_child(duct_node(3));
    DuctSystem system{"Supply", DuctFittingTree{std::move(root)},
                      {material, pressure, {150, 300, 1.5}, SystemType::Supply}};

    EXPECT_EQ(system.level_order_traversal(), system.dftree().level_order_traversal());
    EXPECT_EQ(system.preorder_traversal(), system.dftree().preorder_traversal());
    const auto& const_system = system;
    EXPECT_EQ(const_system.level_order_traversal(), const_system.dftree().level_order_traversal());
    EXPECT_EQ(const_system.preorder_traversal(), const_system.dftree().preorder_traversal());
}

} // namespace
} // namespace dsd::ductsystem::test
