#include "../test_helpers.hpp"
#include "ductsystem/visitors/area_calculator.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include <numbers>
#include <optional>
#include <tuple>

namespace dsd::ductsystem::test {
namespace {

double calculate_area(AreaCalculatorVisitor& visitor, SheetMetalComponent& component) {
    component.accept(visitor);
    return visitor.area_m2();
}

class AreaCalculatorTest : public ::testing::Test {
protected:
    AreaCalculatorVisitor visitor{{150, 300, 1.5}};
};

TEST_F(AreaCalculatorTest, ResultIsUnavailableBeforeAComponentIsVisited) {
    EXPECT_THROW((void)visitor.area_m2(), std::logic_error);
}

TEST_F(AreaCalculatorTest, FailedCalculationClearsPreviousComponentResult) {
    RectangularDuct duct{0, 100, pressure, material, 200, 100, 1000};
    duct.accept(visitor);
    duct.length.reset();

    EXPECT_THROW(duct.accept(visitor), AreaCalculationError);
    EXPECT_THROW((void)visitor.area_m2(), std::logic_error);
}

TEST_F(AreaCalculatorTest, AreaVisitLeavesComponentFieldsUnchanged) {
    RectangularElbow elbow{7, 500, pressure, material, 1, 200, 100};
    elbow.accept(visitor);

    EXPECT_EQ(elbow.id, 7);
    EXPECT_EQ(elbow.flow, 500);
    EXPECT_EQ(elbow.width, 200);
    EXPECT_EQ(elbow.height, 100);
    EXPECT_EQ(elbow.angle, 1);
}

struct DuctAreaCase {
    double width;
    double height;
    double length;
    double expected_m2;
};
class DuctAreaTest : public AreaCalculatorTest,
    public ::testing::WithParamInterface<DuctAreaCase> {};

TEST_P(DuctAreaTest, CalculatesFourFacesWithoutOpenEndsInSquareMetres) {
    const auto value = GetParam();
    RectangularDuct duct{0, 100, pressure, material,
                              value.width, value.height, value.length};
    EXPECT_NEAR(calculate_area(visitor, duct), value.expected_m2, 1e-12);
}

INSTANTIATE_TEST_SUITE_P(PythonReferences, DuctAreaTest, ::testing::Values(
    DuctAreaCase{400, 200, 1000, 1.2},
    DuctAreaCase{1000, 1000, 1000, 4.0},
    DuctAreaCase{200, 100, 500, 0.3}));

class ElbowAreaTest : public AreaCalculatorTest,
    public ::testing::WithParamInterface<std::tuple<double, double>> {};

TEST_P(ElbowAreaTest, CalculatesBothCheeksAndCurvedFacesForSignedAngles) {
    const auto [angle, expected_m2] = GetParam();
    RectangularElbow elbow{0, 100, pressure, material, angle, 200, 100};
    EXPECT_NEAR(calculate_area(visitor, elbow), expected_m2, 1e-12);
}

INSTANTIATE_TEST_SUITE_P(PythonReferences, ElbowAreaTest, ::testing::Values(
    std::tuple{std::numbers::pi / 2, 0.12 * std::numbers::pi},
    std::tuple{-std::numbers::pi / 2, 0.12 * std::numbers::pi},
    std::tuple{std::numbers::pi, 0.24 * std::numbers::pi}));

class ElbowRadiusAreaTest : public ::testing::TestWithParam<std::tuple<double, double>> {};

TEST_P(ElbowRadiusAreaTest, UsesConfiguredInsideRadiusRatio) {
    const auto [ratio, expected_m2] = GetParam();
    AreaCalculatorVisitor visitor{{150, 300, ratio}};
    RectangularElbow elbow{0, 100, pressure, material, std::numbers::pi / 2, 200, 100};
    EXPECT_NEAR(calculate_area(visitor, elbow), expected_m2, 1e-12);
}

INSTANTIATE_TEST_SUITE_P(CustomRadius, ElbowRadiusAreaTest, ::testing::Values(
    std::tuple{1.0, 0.09 * std::numbers::pi}, std::tuple{2.0, 0.15 * std::numbers::pi}));

class ShoeAreaTest : public AreaCalculatorTest,
    public ::testing::WithParamInterface<TakeoffSide> {};

TEST_P(ShoeAreaTest, CalculatesCheeksStraightSideAndBevelUsingComponentLength) {
    RectangularShoeTakeoff shoe{0, 100, pressure, material,
                                     GetParam(), 200, 100, std::nullopt, 100};
    EXPECT_NEAR(calculate_area(visitor, shoe), 0.07414213562373095, 1e-12);
}

INSTANTIATE_TEST_SUITE_P(BothSides, ShoeAreaTest,
                         ::testing::Values(TakeoffSide::Left, TakeoffSide::Right));

struct ReducerAreaCase {
    double inlet_width;
    double inlet_height;
    double outlet_width;
    double outlet_height;
    double expected_m2;
};
class ReducerAreaTest : public ::testing::TestWithParam<ReducerAreaCase> {};

TEST_P(ReducerAreaTest, CalculatesFourCenteredFacesUsingComponentPortSizesAndLength) {
    const auto value = GetParam();
    AreaCalculatorVisitor visitor{{150, 900, 1.5}};
    RectangularReducer reducer{0, 100, pressure, material,
        std::nullopt, std::nullopt, value.inlet_width, value.inlet_height,
        value.outlet_width, value.outlet_height, 300};
    EXPECT_NEAR(calculate_area(visitor, reducer), value.expected_m2, 1e-12);
}

INSTANTIATE_TEST_SUITE_P(PythonReferences, ReducerAreaTest, ::testing::Values(
    ReducerAreaCase{400, 200, 200, 200, 0.3064911064067352},
    ReducerAreaCase{200, 400, 200, 200, 0.3064911064067352},
    ReducerAreaCase{600, 400, 200, 200, 0.4693152893413097},
    ReducerAreaCase{200, 200, 600, 400, 0.4693152893413097},
    ReducerAreaCase{400, 200, 400, 200, 0.36}));

TEST_F(AreaCalculatorTest, EndcapIsOneFlatPlate) {
    RectangularEndcap cap{0, 0, pressure, material, 400, 200};
    EXPECT_NEAR(calculate_area(visitor, cap), 0.08, 1e-12);
}

TEST_F(AreaCalculatorTest, RepeatedCallsUseCurrentDimensionsWithoutCachingByIdentifier) {
    RectangularDuct first{0, 100, pressure, material, 200, 100, 1000};
    RectangularDuct second{0, 100, pressure, material, 400, 200, 1000};
    EXPECT_NEAR(calculate_area(visitor, first), 0.6, 1e-12);
    EXPECT_NEAR(calculate_area(visitor, second), 1.2, 1e-12);
    first.length = 500;
    EXPECT_NEAR(calculate_area(visitor, first), 0.3, 1e-12);
}

TEST_F(AreaCalculatorTest, RejectsUnsizedDuct) {
    RectangularDuct duct{0, 100, pressure, material};
    EXPECT_THROW((void)calculate_area(visitor, duct), AreaCalculationError);
}
TEST_F(AreaCalculatorTest, RejectsUnsizedElbow) {
    RectangularElbow elbow{0, 100, pressure, material, 1};
    EXPECT_THROW((void)calculate_area(visitor, elbow), AreaCalculationError);
}
TEST_F(AreaCalculatorTest, RejectsShoeWithoutTakeoffLength) {
    RectangularShoeTakeoff shoe{0, 100, pressure, material, TakeoffSide::Left, 200, 100};
    EXPECT_THROW((void)calculate_area(visitor, shoe), AreaCalculationError);
}
TEST_F(AreaCalculatorTest, RejectsReducerWithoutPortSizes) {
    RectangularReducer reducer{0, 100, pressure, material};
    EXPECT_THROW((void)calculate_area(visitor, reducer), AreaCalculationError);
}
TEST_F(AreaCalculatorTest, RejectsUnsizedEndcap) {
    RectangularEndcap cap{0, 0, pressure, material};
    EXPECT_THROW((void)calculate_area(visitor, cap), AreaCalculationError);
}

class InvalidAreaDimensionTest : public AreaCalculatorTest,
    public ::testing::WithParamInterface<std::optional<double>> {};

TEST_P(InvalidAreaDimensionTest, RejectsMissingOrInvalidLengthAfterComponentMutation) {
    RectangularDuct duct{42, 100, pressure, material, 200, 100, 1000};
    duct.length = GetParam();
    try {
        (void)calculate_area(visitor, duct);
        FAIL() << "Expected AreaCalculationError";
    } catch (const AreaCalculationError& error) {
        EXPECT_NE(std::string{error.what()}.find("42"), std::string::npos);
        EXPECT_NE(std::string{error.what()}.find("length"), std::string::npos);
    }
}

INSTANTIATE_TEST_SUITE_P(DimensionValidation, InvalidAreaDimensionTest,
    ::testing::Values(std::optional<double>{}, std::optional<double>{0},
                     std::optional<double>{-1},
                     std::optional<double>{std::numeric_limits<double>::quiet_NaN()},
                     std::optional<double>{std::numeric_limits<double>::infinity()}),
    [](const ::testing::TestParamInfo<std::optional<double>>& info) {
        if (!info.param) return std::string{"Missing"};
        return invalid_number_name(::testing::TestParamInfo<double>{*info.param, info.index});
    });

TEST_F(AreaCalculatorTest, RejectsMutatedInvalidElbowAngle) {
    RectangularElbow elbow{0, 100, pressure, material, 1, 200, 100};
    for (double invalid : {0.0, 4.0, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
        SCOPED_TRACE(invalid);
        elbow.angle = invalid;
        EXPECT_THROW((void)calculate_area(visitor, elbow), AreaCalculationError);
    }
}

TEST_F(AreaCalculatorTest, RejectsOverflowInsteadOfReturningInfiniteArea) {
    RectangularDuct duct{0, 100, pressure, material, 400, 200,
                               std::numeric_limits<double>::max()};
    EXPECT_THROW((void)calculate_area(visitor, duct), AreaCalculationError);
}

} // namespace
} // namespace dsd::ductsystem::test
