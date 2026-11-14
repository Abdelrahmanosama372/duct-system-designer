#include "../unit/geometry/test_geometry_helpers.hpp"
#include "ductsystem/visitors/geometry_constructor.hpp"
#include "ductsystem/visitors/area_calculator.hpp"
#include <numbers>

namespace dsd::geometry::test {
namespace {

using namespace ductsystem;

template<class Component, class... Args>
std::shared_ptr<SheetMetalNode> node(Args&&... args) {
    return std::make_shared<SheetMetalNode>(std::make_unique<Component>(std::forward<Args>(args)...));
}

DuctFittingTree main_network() {
    auto root = node<RectangularDuct>(0, 1000, pressure, material, 400, 200, 3000);
    auto reducer = node<RectangularReducer>(6, 600, pressure, material,
        400, 200, 400, 200, 300, 150, 300);
    reducer->add_child(node<RectangularDuct>(1, 600, pressure, material, 300, 150, 3000));
    root->add_child(reducer);
    auto shoe = node<RectangularShoeTakeoff>(4, 400, pressure, material, TakeoffSide::Left,
                                            200, 100, 2, 150);
    auto branch = node<RectangularDuct>(2, 400, pressure, material, 200, 100, 3000);
    auto elbow = node<RectangularElbow>(5, 400, pressure, material, -std::numbers::pi / 2, 200, 100);
    elbow->add_child(node<RectangularDuct>(3, 400, pressure, material, 200, 100, 2000));
    branch->add_child(elbow);
    shoe->add_child(branch);
    root->add_child(shoe);
    return DuctFittingTree{root};
}

std::map<int, Line> main_sources(const Transform2D& transform = {}) {
    std::map<int, Line> result{
        {0, Line{{0, 0}, {3000, 0}, LineType::Normal, 0}},
        {1, Line{{3000, 0}, {6000, 0}, LineType::Normal, 1}},
        {2, Line{{3000, 0}, {3000, 3000}, LineType::Normal, 2}},
        {3, Line{{3000, 3000}, {5000, 3000}, LineType::Normal, 3}}};
    for (auto& [id, line] : result) {
        (void)id;
        line = line.transformed(transform);
    }
    return result;
}

class GeometryPipelineTest : public ::testing::TestWithParam<double> {};

TEST_P(GeometryPipelineTest, FitsAllDuctsToPlacedFittingSkeletonsAndUpdatesFinalLengths) {
    auto tree = main_network();
    const Transform2D transform{{100, 200}, GetParam()};
    const SourceGeometryRepository repo{main_sources(transform)};
    PlacementContextManager placement{tree, repo};
    RectangularGeometricRepresentationBuilder builder{{150, 300, 1.5}};
    GeometryConstructorVisitor visitor{placement, builder};

    visitor.construct();

    const std::map<int, std::vector<Line>> expected{
        {0, {Line{{0, 0}, {3600, 0}}}},
        {1, {Line{{3900, 0}, {6000, 0}}}},
        {2, {Line{{3000, 350}, {3000, 2600}}}},
        {3, {Line{{3400, 3000}, {5000, 3000}}}},
        {4, {Line{{3000, 200}, {3000, 350}}}},
        {5, {Line{{3000, 2600}, {3000, 3000}}, Line{{3000, 3000}, {3400, 3000}}}},
        {6, {Line{{3600, 0}, {3900, 0}}}}};
    const auto first = visitor.representations();
    ASSERT_EQ(first.size(), expected.size());
    for (const auto& [id, lines] : expected) {
        SCOPED_TRACE(id);
        const auto& representation = first.at(id);
        EXPECT_EQ(representation.primitives.size(), 4);
        ASSERT_EQ(representation.layout_skeleton.size(), lines.size());
        for (std::size_t i = 0; i < lines.size(); ++i) {
            expect_line(representation.layout_skeleton[i],
                        transform.apply(lines[i].start), transform.apply(lines[i].end));
        }
    }
    const std::map<int, double> expected_lengths{{0, 3600}, {1, 2100}, {2, 2250}, {3, 1600}};
    for (auto* item : tree.preorder_traversal()) {
        if (auto* duct = dynamic_cast<RectangularDuct*>(&item->component())) {
            ASSERT_TRUE(duct->length);
            EXPECT_NEAR(*duct->length, expected_lengths.at(duct->id), 1e-7);
        }
    }
    EXPECT_NEAR(repo.get_line(0).length(), 3000, 1e-7);
    const auto normalized_elbow = builder.normalized_representations().at(5);
    visitor.construct();
    EXPECT_EQ(builder.normalized_representations().at(5), normalized_elbow);
    for (const auto& [id, representation] : first) {
        const auto& repeated = visitor.representations().at(id).layout_skeleton;
        for (std::size_t i = 0; i < repeated.size(); ++i) {
            expect_line(repeated[i], representation.layout_skeleton[i].start,
                        representation.layout_skeleton[i].end);
        }
    }
}
INSTANTIATE_TEST_SUITE_P(RotatedNetworks, GeometryPipelineTest,
    ::testing::Values(0.0, std::numbers::pi / 2, std::numbers::pi / 4, -std::numbers::pi / 2));

TEST(GeometryPipelineTest, ExtendsTerminalMainAndPlacesEndcapAndBothShoeDirections) {
    auto root = node<RectangularDuct>(0, 1000, pressure, material, 400, 200, 3000);
    auto left = node<RectangularShoeTakeoff>(3, 400, pressure, material,
        TakeoffSide::Left, 200, 100, 1, 150);
    left->add_child(node<RectangularDuct>(1, 400, pressure, material, 200, 100, 3000));
    auto right = node<RectangularShoeTakeoff>(4, 600, pressure, material,
        TakeoffSide::Right, 300, 150, 2, 150);
    right->add_child(node<RectangularDuct>(2, 600, pressure, material, 300, 150, 3000));
    root->add_child(left);
    root->add_child(right);
    root->add_child(node<RectangularEndcap>(5, 0, pressure, material, 400, 200));
    DuctFittingTree tree{root};
    PlacementContextManager placement{tree, SourceGeometryRepository{{
        {0, Line{{0, 0}, {3000, 0}, LineType::Normal, 0}},
        {1, Line{{3000, 0}, {3000, 3000}, LineType::Normal, 1}},
        {2, Line{{3000, 0}, {3000, -3000}, LineType::Normal, 2}}}}};
    RectangularGeometricRepresentationBuilder builder{{150, 300, 1.5}};
    GeometryConstructorVisitor visitor{placement, builder};
    visitor.construct();
    const auto& result = visitor.representations();
    expect_line(result.at(0).layout_skeleton[0], {0, 0}, {3600, 0});
    expect_line(result.at(1).layout_skeleton[0], {3000, 350}, {3000, 3000});
    expect_line(result.at(2).layout_skeleton[0], {3000, -350}, {3000, -3000});
    EXPECT_TRUE(result.at(5).layout_skeleton.empty());
    expect_line(std::get<Line>(result.at(5).primitives[0]), {3600, -200}, {3600, 200});
    EXPECT_EQ(dynamic_cast<RectangularDuct&>(root->component()).length, 3600);
}

TEST(GeometryPipelineTest, ValidatesAllConnectionsBeforeChangingAnyDuctLengths) {
    auto tree = main_network();
    auto sources = main_sources();
    sources.at(2).end = {3000, 100}; // This later branch cannot accommodate its elbow.
    PlacementContextManager placement{tree, SourceGeometryRepository{sources}};
    RectangularGeometricRepresentationBuilder builder{{150, 300, 1.5}};
    GeometryConstructorVisitor visitor{placement, builder};

    EXPECT_THROW(visitor.construct(), PlacementError);

    // The valid root would become 3600 mm if it had been changed prematurely.
    EXPECT_EQ(dynamic_cast<RectangularDuct&>(tree.root().component()).length, 3000);
}

TEST(GeometryPipelineTest, DirectDuctVisitRequiresItsFittingsToBePlacedFirst) {
    auto tree = main_network();
    PlacementContextManager placement{tree, SourceGeometryRepository{main_sources()}};
    RectangularGeometricRepresentationBuilder builder{{150, 300, 1.5}};
    GeometryConstructorVisitor visitor{placement, builder};
    EXPECT_THROW(tree.root().component().accept(visitor), PlacementError);
}

TEST(GeometryPipelineTest, DirectVisitOfStandaloneDuctPublishesGeometryAndFinalLength) {
    auto root = node<RectangularDuct>(0, 100, pressure, material, 200, 100);
    DuctFittingTree tree{root};
    PlacementContextManager placement{tree, SourceGeometryRepository{{
        {0, Line{{10, 20}, {10, 1020}, LineType::Normal, 0}}}}};
    RectangularGeometricRepresentationBuilder builder{{150, 300, 1.5}};
    GeometryConstructorVisitor visitor{placement, builder};
    root->component().accept(visitor);
    EXPECT_EQ(dynamic_cast<RectangularDuct&>(root->component()).length, 1000);
    expect_line(visitor.representations().at(0).layout_skeleton[0], {10, 20}, {10, 1020});

    AreaCalculatorVisitor area{{150, 300, 1.5}};
    root->component().accept(area);
    EXPECT_NEAR(area.area_m2(), 0.6, 1e-12);
}

} // namespace
} // namespace dsd::geometry::test
