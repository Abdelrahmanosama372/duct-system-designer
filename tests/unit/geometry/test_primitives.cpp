#include "test_geometry_helpers.hpp"
#include <limits>
#include <numbers>

namespace dsd::geometry::test {
namespace {

TEST(GeometryPrimitivesTest, PointAndLineMeasureDistancesAndDirection) {
    const Point a{0, 0, 0}, b{3, 4, 12};
    EXPECT_EQ(a.distance(b), 13);
    const Line line{{0, 0}, {0, 100}};
    EXPECT_EQ(line.length(), 100);
    EXPECT_NEAR(line.direction(), std::numbers::pi / 2, 1e-12);
}

TEST(GeometryPrimitivesTest, TransformRotatesThenTranslatesInXYPlane) {
    const Transform2D transform{{10, 20}, std::numbers::pi / 2};
    expect_point(transform.apply({100, 0}), {10, 120});
    expect_point(transform.translated_local(100, 0).origin, {10, 120});
    expect_point(transform.rotated_local(-std::numbers::pi / 2).apply({100, 0}), {110, 20});
}

TEST(GeometryPrimitivesTest, CompositionMatchesSuccessiveTransforms) {
    const Transform2D outer{{10, 20}, std::numbers::pi / 2};
    const Transform2D inner{{3, 4}, -std::numbers::pi / 4};
    const Point point{50, -20};
    expect_point(outer.composed(inner).apply(point), outer.apply(inner.apply(point)));
}

TEST(GeometryPrimitivesTest, SignedArcSweepDeterminesEndpoint) {
    const Arc left{{1, 0}, {0, 0}, std::numbers::pi / 2};
    const Arc right{{1, 0}, {0, 0}, -std::numbers::pi / 2};
    expect_point(left.end(), {0, 1});
    expect_point(right.end(), {0, -1});
}

TEST(GeometryPrimitivesTest, TransformsPreservePrimitiveMetadataAndLeaveOriginalsUnchanged) {
    const Transform2D transform{{10, 20}, std::numbers::pi / 2};
    const Line line{{0, 0}, {100, 0}, LineType::CenterLine, 42};
    const auto placed = line.transformed(transform);
    EXPECT_EQ(placed.id, 42);
    EXPECT_EQ(placed.line_type, LineType::CenterLine);
    expect_line(placed, {10, 20}, {10, 120});
    expect_line(line, {0, 0}, {100, 0});

    const Arc arc{{1, 0}, {0, 0}, -std::numbers::pi / 2, LineType::Hidden, 7};
    const auto placed_arc = arc.transformed(transform);
    EXPECT_EQ(placed_arc.id, 7);
    EXPECT_EQ(placed_arc.line_type, LineType::Hidden);
    EXPECT_EQ(placed_arc.angle, arc.angle);
    expect_point(placed_arc.end(), transform.apply(arc.end()));
}

TEST(GeometryPrimitivesTest, RejectsNonFiniteCoordinatesAndAngles) {
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((void)(Point{nan, 0}), std::invalid_argument);
    EXPECT_THROW((void)(Transform2D{{}, nan}), std::invalid_argument);
    EXPECT_THROW((void)(Arc{{1, 0}, {}, nan}), std::invalid_argument);
}

TEST(GeometryRepresentationTest, TransformsOutlineAndSkeletonWithoutMutatingNormalizedGeometry) {
    const GeometricRepresentation original{9,
        {Line{{0, -10}, {100, -10}}, Arc{{1, 0}, {}, std::numbers::pi / 2}},
        {Line{{0, 0}, {100, 0}, LineType::CenterLine}}};
    const auto placed = original.transformed({{10, 20}, std::numbers::pi / 2});
    EXPECT_EQ(placed.component_id, 9);
    expect_line(std::get<Line>(placed.primitives[0]), {20, 20}, {20, 120});
    expect_line(placed.layout_skeleton[0], {10, 20}, {10, 120});
    expect_line(std::get<Line>(original.primitives[0]), {0, -10}, {100, -10});
    EXPECT_TRUE(std::holds_alternative<Arc>(placed.primitives[1]));
}

} // namespace
} // namespace dsd::geometry::test
