#include "test_geometry_helpers.hpp"
#include "geometry/placement_context_manager.hpp"
#include <numbers>

namespace dsd::geometry::test {
namespace {

using namespace ductsystem;

std::shared_ptr<SheetMetalNode> duct_node(int id) {
    return std::make_shared<SheetMetalNode>(
        std::make_unique<RectangularDuct>(id, 100, pressure, material, 200, 100, 1000));
}

TEST(SourceGeometryRepositoryTest, RejectsKeysDifferentFromProcessedLineIdentifiers) {
    EXPECT_THROW((void)(SourceGeometryRepository{{{0, Line{{0, 0}, {100, 0}, LineType::Normal, 1}}}}),
                 std::invalid_argument);
}
TEST(SourceGeometryRepositoryTest, OwnsACopyAndReportsMissingSources) {
    std::map<int, Line> lines{{0, Line{{0, 0}, {100, 0}, LineType::Normal, 0}}};
    const SourceGeometryRepository repo{lines};
    lines.at(0).end.x = 200;
    EXPECT_EQ(repo.get_line(0).end.x, 100);
    EXPECT_THROW((void)repo.get_line(99), std::invalid_argument);
}

TEST(PlacementContextTest, StraightDuctUsesSourceOriginDirectionAndXYLength) {
    DuctFittingTree tree{duct_node(0)};
    PlacementContextManager manager{tree,
        SourceGeometryRepository{{{0, Line{{10, 20}, {10, 1020}, LineType::Normal, 0}}}}};
    const auto context = manager.find_context(tree.root().component());
    expect_point(context.transform.origin, {10, 20});
    EXPECT_NEAR(context.transform.angle, std::numbers::pi / 2, 1e-12);
    EXPECT_EQ(context.length, 1000);
}

TEST(PlacementContextTest, RejectsComponentOutsideTheTreeEvenWithMatchingIdentifier) {
    DuctFittingTree tree{duct_node(0)};
    PlacementContextManager manager{tree, SourceGeometryRepository{{}}};
    const RectangularDuct unrelated{0, 100, pressure, material, 200, 100, 1000};
    EXPECT_THROW((void)manager.find_context(unrelated), PlacementError);
}

TEST(PlacementContextTest, RejectsSourceWithoutUsableXYLength) {
    DuctFittingTree tree{duct_node(0)};
    PlacementContextManager manager{tree,
        SourceGeometryRepository{{{0, Line{{0, 0, 0}, {0, 0, 10}, LineType::Normal, 0}}}}};
    EXPECT_THROW((void)manager.find_context(tree.root().component()), PlacementError);
}

TEST(PlacementContextTest, RejectsFittingWithoutUpstreamDuct) {
    DuctFittingTree tree{std::make_shared<SheetMetalNode>(
        std::make_unique<RectangularElbow>(0, 100, pressure, material, 1, 200, 100))};
    PlacementContextManager manager{tree, SourceGeometryRepository{{}}};
    EXPECT_THROW((void)manager.find_context(tree.root().component()), PlacementError);
}

TEST(PlacementContextTest, RequiresFittingLayoutsBeforeConnectingDucts) {
    auto root = duct_node(0);
    root->add_child(std::make_shared<SheetMetalNode>(
        std::make_unique<RectangularElbow>(1, 100, pressure, material, 1, 200, 100)));
    DuctFittingTree tree{root};
    PlacementContextManager manager{tree,
        SourceGeometryRepository{{{0, Line{{0, 0}, {1000, 0}, LineType::Normal, 0}}}}};
    EXPECT_THROW((void)manager.find_context(tree.root().component()), PlacementError);
}

TEST(PlacementContextTest, RejectsFittingSkeletonThatDoesNotFollowSourceAxis) {
    auto root = duct_node(0);
    root->add_child(std::make_shared<SheetMetalNode>(
        std::make_unique<RectangularElbow>(1, 100, pressure, material, 1, 200, 100)));
    DuctFittingTree tree{root};
    PlacementContextManager manager{tree,
        SourceGeometryRepository{{{0, Line{{0, 0}, {1000, 0}, LineType::Normal, 0}}}}};
    manager.set_fitting_layouts({{1, GeometricRepresentation{1, {},
        {Line{{0, 100}, {100, 100}, LineType::CenterLine}}}}});
    EXPECT_THROW((void)manager.find_context(tree.root().component()), PlacementError);
}

TEST(PlacementContextTest, RequiresShoeToReferenceItsSoleBranchChild) {
    auto root = duct_node(0);
    auto shoe = std::make_shared<SheetMetalNode>(std::make_unique<RectangularShoeTakeoff>(
        1, 100, pressure, material, TakeoffSide::Left, 200, 100, 99, 150));
    shoe->add_child(duct_node(2));
    root->add_child(shoe);
    DuctFittingTree tree{root};
    PlacementContextManager manager{tree,
        SourceGeometryRepository{{{0, Line{{0, 0}, {1000, 0}, LineType::Normal, 0}}}}};
    EXPECT_THROW((void)manager.find_context(shoe->component()), PlacementError);
}

} // namespace
} // namespace dsd::geometry::test
