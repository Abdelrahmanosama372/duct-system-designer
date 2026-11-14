#include "test_helpers.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include <numbers>
#include <stdexcept>

namespace dsd::ductsystem::test {
namespace {

TEST(RectangularElbowTest, AcceptsEitherTurnDirectionAndBoundaryAngles) {
    for (double angle : {-std::numbers::pi, -std::numbers::pi / 2,
                          std::numbers::pi / 2, std::numbers::pi}) {
        const RectangularElbow elbow{0, 100, pressure, material, angle};
        EXPECT_EQ(elbow.angle, angle);
    }
}

TEST(RectangularElbowTest, RejectsAnglesOutsideSignedPiRange) {
    EXPECT_THROW((void)(RectangularElbow{0, 100, pressure, material, 4}), std::invalid_argument);
    EXPECT_THROW((void)(RectangularElbow{0, 100, pressure, material, -4}), std::invalid_argument);
}

class InvalidElbowAngleTest : public ::testing::TestWithParam<double> {};

TEST_P(InvalidElbowAngleTest, RejectsZeroOrNonFiniteAngle) {
    EXPECT_THROW((void)(RectangularElbow{0, 100, pressure, material, GetParam()}),
                 std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ElbowValidation, InvalidElbowAngleTest,
    ::testing::Values(0.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()), invalid_number_name);

TEST(RectangularElbowTest, RadiusIsAbsentUntilWidthIsAssigned) {
    const RectangularElbow elbow{0, 100, pressure, material, 1};
    EXPECT_FALSE(elbow.inside_radius(1.5));
    EXPECT_FALSE(elbow.centerline_radius(1.5));
}

TEST(RectangularElbowTest, RadiiUseTheCurrentWidth) {
    RectangularElbow elbow{0, 100, pressure, material, 1, 200, 100};
    EXPECT_EQ(elbow.inside_radius(1.5), 300);
    EXPECT_EQ(elbow.centerline_radius(1.5), 400);

    elbow.width = 300;
    EXPECT_EQ(elbow.inside_radius(1.5), 450);
    EXPECT_EQ(elbow.centerline_radius(1.5), 600);
}

class InvalidRadiusRatioTest : public ::testing::TestWithParam<double> {};

TEST_P(InvalidRadiusRatioTest, RejectedEvenWhenElbowIsUnsized) {
    const RectangularElbow elbow{0, 100, pressure, material, 1};
    EXPECT_THROW((void)elbow.inside_radius(GetParam()), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ElbowValidation, InvalidRadiusRatioTest,
    ::testing::Values(0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()), invalid_number_name);

TEST(RectangularReducerTest, CanRemainUnsized) {
    const RectangularReducer reducer{0, 100, pressure, material};
    EXPECT_FALSE(reducer.inlet_width);
    EXPECT_FALSE(reducer.inlet_height);
    EXPECT_FALSE(reducer.outlet_width);
    EXPECT_FALSE(reducer.outlet_height);
    EXPECT_FALSE(reducer.length);
}

TEST(RectangularReducerTest, ValidatesEachReducerSpecificDimension) {
    EXPECT_THROW((void)(RectangularReducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, -1}), std::invalid_argument);
    EXPECT_THROW((void)(RectangularReducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, std::nullopt, -1}), std::invalid_argument);
    EXPECT_THROW((void)(RectangularReducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, -1}), std::invalid_argument);
    EXPECT_THROW((void)(RectangularReducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, -1}),
        std::invalid_argument);
    EXPECT_THROW((void)(RectangularReducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, -1}),
        std::invalid_argument);
}

TEST(RectangularShoeTakeoffTest, OpeningRequiresBothWidthAndTakeoffLength) {
    RectangularShoeTakeoff shoe{0, 100, pressure, material, TakeoffSide::Left};
    EXPECT_FALSE(shoe.width1());
    shoe.width = 200;
    EXPECT_FALSE(shoe.width1());
    shoe.takeoff_length = 150;
    EXPECT_EQ(shoe.width1(), 350);
    shoe.width = 250;
    EXPECT_EQ(shoe.width1(), 400);
}

TEST(RectangularShoeTakeoffTest, AcceptsBothSidesAndStoresBranchReference) {
    const RectangularShoeTakeoff left{0, 100, pressure, material, TakeoffSide::Left};
    const RectangularShoeTakeoff right{1, 100, pressure, material, TakeoffSide::Right,
                                     200, 150, 42, 150};
    EXPECT_EQ(left.side, TakeoffSide::Left);
    EXPECT_EQ(right.side, TakeoffSide::Right);
    EXPECT_EQ(right.branch_duct_id, 42);
}

TEST(RectangularShoeTakeoffTest, RejectsInvalidSide) {
    EXPECT_THROW((void)(RectangularShoeTakeoff{
        0, 100, pressure, material, static_cast<TakeoffSide>(99)}), std::invalid_argument);
}

TEST(RectangularShoeTakeoffTest, RejectsInvalidTakeoffLength) {
    EXPECT_THROW((void)(RectangularShoeTakeoff{0, 100, pressure, material, TakeoffSide::Left,
        std::nullopt, std::nullopt, std::nullopt, 0}), std::invalid_argument);
}

TEST(RectangularEndcapTest, SupportsDeferredDimensionAssignment) {
    RectangularEndcap endcap{0, 0, pressure, material};
    EXPECT_FALSE(endcap.width);
    EXPECT_FALSE(endcap.height);
    endcap.width = 300;
    endcap.height = 150;
    EXPECT_EQ(endcap.width, 300);
    EXPECT_EQ(endcap.height, 150);
}

} // namespace
} // namespace dsd::ductsystem::test
