#include "test_geometry_helpers.hpp"
#include <numbers>

namespace dsd::geometry::test {
namespace {

using namespace ductsystem;

class GeometryBuilderTest : public ::testing::Test {
protected:
    RectangularGeometricRepresentationBuilder builder{{150, 300, 1.5}};
    PlacementContext identity{};
};

TEST_F(GeometryBuilderTest, DuctHasFourClosedEdgesAndFiniteCenterline) {
    const RectangularDuct duct{0, 100, pressure, material, 200, 100, 1000};
    const auto result = builder.build_duct(duct, identity);
    ASSERT_EQ(result.primitives.size(), 4);
    expect_line(std::get<Line>(result.primitives[0]), {0, -100}, {1000, -100});
    expect_line(std::get<Line>(result.primitives[1]), {1000, -100}, {1000, 100});
    expect_line(std::get<Line>(result.primitives[2]), {1000, 100}, {0, 100});
    expect_line(std::get<Line>(result.primitives[3]), {0, 100}, {0, -100});
    ASSERT_EQ(result.layout_skeleton.size(), 1);
    expect_line(result.layout_skeleton[0], {0, 0}, {1000, 0});
    EXPECT_EQ(result.layout_skeleton[0].line_type, LineType::CenterLine);
}

class ElbowGeometryTest : public GeometryBuilderTest, public ::testing::WithParamInterface<double> {};

TEST_P(ElbowGeometryTest, SignedTurnsHaveTwoArcsAndTwoIntersectingSkeletonLegs) {
    const double side = GetParam();
    const RectangularElbow elbow{0, 100, pressure, material, side * std::numbers::pi / 2, 200, 100};
    const auto result = builder.build_elbow(elbow, identity);
    ASSERT_EQ(result.primitives.size(), 4);
    const auto& inner = std::get<Arc>(result.primitives[0]);
    const auto& outer = std::get<Arc>(result.primitives[1]);
    expect_point(inner.center, {-400, side * 400});
    expect_point(inner.start, {-400, side * 100});
    expect_point(outer.start, {-400, -side * 100});
    EXPECT_EQ(inner.angle, elbow.angle);
    EXPECT_EQ(outer.angle, elbow.angle);
    expect_line(std::get<Line>(result.primitives[3]), {-100, side * 400}, {100, side * 400});
    ASSERT_EQ(result.layout_skeleton.size(), 2);
    expect_line(result.layout_skeleton[0], {-400, 0}, {0, 0});
    expect_line(result.layout_skeleton[1], {0, 0}, {0, side * 400});
}
INSTANTIATE_TEST_SUITE_P(BothDirections, ElbowGeometryTest, ::testing::Values(1.0, -1.0));

TEST_F(GeometryBuilderTest, ShoeBevelWidensTheCorrectSideOfTheMainOpening) {
    const RectangularShoeTakeoff left{0, 100, pressure, material, TakeoffSide::Left,
                                     200, 100, std::nullopt, 150};
    const RectangularShoeTakeoff right{1, 100, pressure, material, TakeoffSide::Right,
                                      200, 100, std::nullopt, 150};
    const auto a = builder.build_shoetakeoff(left, identity);
    const auto b = builder.build_shoetakeoff(right, identity);
    expect_line(std::get<Line>(a.primitives[0]), {0, -250}, {150, -100});
    expect_point(std::get<Line>(a.primitives[2]).end, {0, 100});
    expect_line(std::get<Line>(b.primitives[0]), {0, -100}, {150, -100});
    expect_point(std::get<Line>(b.primitives[2]).end, {0, 250});
    EXPECT_EQ(a.layout_skeleton[0].length(), 150);
}

TEST_F(GeometryBuilderTest, ReducerOutlineUsesCenteredPortWidths) {
    const RectangularReducer reducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, 400, 200, 200, 100, 300};
    const auto result = builder.build_reducer(reducer, identity);
    ASSERT_EQ(result.primitives.size(), 4);
    expect_line(std::get<Line>(result.primitives[0]), {0, -200}, {300, -100});
    expect_line(std::get<Line>(result.primitives[1]), {300, -100}, {300, 100});
    expect_line(std::get<Line>(result.primitives[2]), {300, 100}, {0, 200});
    expect_line(std::get<Line>(result.primitives[3]), {0, 200}, {0, -200});
    EXPECT_EQ(result.layout_skeleton[0].length(), 300);
}

TEST_F(GeometryBuilderTest, EndcapHasOneLineAndNoSkeleton) {
    const RectangularEndcap cap{0, 0, pressure, material, 400, 200};
    const auto result = builder.build_endcap(cap, identity);
    ASSERT_EQ(result.primitives.size(), 1);
    expect_line(std::get<Line>(result.primitives[0]), {0, -200}, {0, 200});
    EXPECT_TRUE(result.layout_skeleton.empty());
}

TEST_F(GeometryBuilderTest, ReusesNormalizedGeometryForPlacementAndRebuildsAfterDimensionChanges) {
    RectangularDuct duct{0, 100, pressure, material, 200, 100, 1000};
    (void)builder.build_duct(duct, identity);
    const auto original = builder.normalized_representations().at(0);
    const auto placed = builder.build_duct(duct, {Transform2D{{10, 20}, std::numbers::pi / 2}});
    EXPECT_EQ(builder.normalized_representations().at(0), original);
    expect_line(std::get<Line>(original->primitives[0]), {0, -100}, {1000, -100});
    expect_line(std::get<Line>(placed.primitives[0]), {110, 20}, {110, 1020});
    duct.width = 400;
    (void)builder.build_duct(duct, identity);
    EXPECT_NE(builder.normalized_representations().at(0), original);
}

TEST_F(GeometryBuilderTest, RejectsUnsizedAndMutatedInvalidDimensions) {
    RectangularDuct duct{0, 100, pressure, material};
    EXPECT_THROW((void)builder.build_duct(duct, identity), GeometryConstructionError);
    duct.width = 200;
    duct.height = 100;
    duct.length = 0;
    EXPECT_THROW((void)builder.build_duct(duct, identity), GeometryConstructionError);
}

TEST_F(GeometryBuilderTest, RejectsHalfTurnElbowUnsupportedByPlanPlacement) {
    const RectangularElbow elbow{0, 100, pressure, material, std::numbers::pi, 200, 100};
    EXPECT_THROW((void)builder.build_elbow(elbow, identity), GeometryConstructionError);
}

TEST_F(GeometryBuilderTest, RejectsMutatedInvalidShoeSideAndUnsupportedView) {
    RectangularShoeTakeoff shoe{0, 100, pressure, material, TakeoffSide::Left,
                               200, 100, std::nullopt, 150};
    shoe.side = static_cast<TakeoffSide>(99);
    EXPECT_THROW((void)builder.build_shoetakeoff(shoe, identity), GeometryConstructionError);
    const RectangularEndcap cap{1, 0, pressure, material, 400, 200};
    identity.view_type = static_cast<ViewType>(99);
    EXPECT_THROW((void)builder.build_endcap(cap, identity), GeometryConstructionError);
}

} // namespace
} // namespace dsd::geometry::test
