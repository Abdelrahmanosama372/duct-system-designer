#include "sizing_fixture.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace dsd::sizing::test {
namespace {

class DuctSizerConfigTest : public SizingTest {};

TEST_F(DuctSizerConfigTest, DefaultsToAbsentTargetsAndStandardSolverSettings) {
    const DuctSizerConfig config{25, 25, 2000, 2, 1.2, 1.8e-5, 0.00015};

    EXPECT_FALSE(config.required_velocity.has_value());
    EXPECT_FALSE(config.required_head_loss.has_value());
    EXPECT_DOUBLE_EQ(config.diameter_tolerance_mm, 0.0001);
    EXPECT_EQ(config.dimension_solver_max_iterations, 30);
    EXPECT_DOUBLE_EQ(config.friction_tolerance, 1e-10);
    EXPECT_EQ(config.friction_factor_solver_max_iterations, 80);
}

TEST_F(DuctSizerConfigTest, AcceptsZeroRoughnessForSmoothDucts) {
    roughness = 0;

    const auto config = make_config();

    EXPECT_EQ(config.roughness, 0);
}

TEST_F(DuctSizerConfigTest, LeavesAlignedBoundsUnchanged) {
    const auto config = make_config();

    EXPECT_EQ(config.minimum_duct_dimension, 25);
    EXPECT_EQ(config.maximum_duct_dimension, 2000);
}

TEST_F(DuctSizerConfigTest, RoundsMinimumUpToNextGridSize) {
    minimum_duct_dimension = 110;

    const auto config = make_config();

    EXPECT_EQ(config.minimum_duct_dimension, 125);
    EXPECT_EQ(config.maximum_duct_dimension, 2000);
}

TEST_F(DuctSizerConfigTest, RoundsMaximumDownToPreviousGridSize) {
    maximum_duct_dimension = 490;

    const auto config = make_config();

    EXPECT_EQ(config.minimum_duct_dimension, 25);
    EXPECT_EQ(config.maximum_duct_dimension, 475);
}

TEST_F(DuctSizerConfigTest, AllowsBoundsContainingExactlyOneGridSize) {
    minimum_duct_dimension = 101;
    maximum_duct_dimension = 125;

    const auto config = make_config();

    EXPECT_EQ(config.minimum_duct_dimension, 125);
    EXPECT_EQ(config.maximum_duct_dimension, 125);
}

TEST_F(DuctSizerConfigTest, RejectsMinimumGreaterThanMaximum) {
    minimum_duct_dimension = 500;
    maximum_duct_dimension = 100;

    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_F(DuctSizerConfigTest, RejectsBoundsContainingNoGridSize) {
    minimum_duct_dimension = 101;
    maximum_duct_dimension = 124;

    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_F(DuctSizerConfigTest, NormalizesBoundsNearIntegerLimitWithoutOverflow) {
    // INT_MAX - 100 .. INT_MAX, with 25 mm increments.
    minimum_duct_dimension = 2147483547;
    maximum_duct_dimension = 2147483647;

    const auto config = make_config();

    EXPECT_EQ(config.minimum_duct_dimension, 2147483550);
    EXPECT_EQ(config.maximum_duct_dimension, 2147483625);
}

TEST_F(DuctSizerConfigTest, RejectsMinimumWhoseNextGridSizeExceedsIntegerLimit) {
    minimum_duct_dimension = std::numeric_limits<int>::max();
    maximum_duct_dimension = std::numeric_limits<int>::max();

    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_F(DuctSizerConfigTest, AcceptsIntegerLimitAsAnExactGridSize) {
    size_increment = std::numeric_limits<int>::max();
    minimum_duct_dimension = 1;
    maximum_duct_dimension = std::numeric_limits<int>::max();

    const auto config = make_config();

    EXPECT_EQ(config.minimum_duct_dimension, std::numeric_limits<int>::max());
    EXPECT_EQ(config.maximum_duct_dimension, std::numeric_limits<int>::max());
}

class InvalidPositiveIntegerTest : public DuctSizerConfigTest,
    public ::testing::WithParamInterface<int> {};

TEST_P(InvalidPositiveIntegerTest, RejectsSizeIncrement) {
    size_increment = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveIntegerTest, RejectsMinimumDimension) {
    minimum_duct_dimension = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveIntegerTest, RejectsMaximumDimension) {
    maximum_duct_dimension = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveIntegerTest, RejectsDimensionSolverIterationLimit) {
    dimension_solver_max_iterations = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveIntegerTest, RejectsFrictionFactorSolverIterationLimit) {
    friction_factor_solver_max_iterations = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ConfigValidation, InvalidPositiveIntegerTest,
    ::testing::Values(0, -1), [](const ::testing::TestParamInfo<int>& info) {
        return info.param == 0 ? "Zero" : "Negative";
    });

std::string invalid_number_name(const ::testing::TestParamInfo<double>& info) {
    if (std::isnan(info.param)) return "NaN";
    if (std::isinf(info.param)) {
        return info.param > 0 ? "PositiveInfinity" : "NegativeInfinity";
    }
    return info.param == 0 ? "Zero" : "Negative";
}

class InvalidPositiveNumberTest : public DuctSizerConfigTest,
    public ::testing::WithParamInterface<double> {};

TEST_P(InvalidPositiveNumberTest, RejectsAspectRatio) {
    aspect_ratio = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveNumberTest, RejectsAirDensity) {
    air_density = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveNumberTest, RejectsDynamicViscosity) {
    dynamic_viscosity = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveNumberTest, RejectsRequiredVelocity) {
    required_velocity = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveNumberTest, RejectsRequiredHeadLoss) {
    required_head_loss = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveNumberTest, RejectsDiameterTolerance) {
    diameter_tolerance_mm = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

TEST_P(InvalidPositiveNumberTest, RejectsFrictionTolerance) {
    friction_tolerance = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ConfigValidation, InvalidPositiveNumberTest,
    ::testing::Values(0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity(),
                     -std::numeric_limits<double>::infinity()),
    invalid_number_name);

class InvalidRoughnessTest : public DuctSizerConfigTest,
    public ::testing::WithParamInterface<double> {};

TEST_P(InvalidRoughnessTest, RejectsNegativeOrNonFiniteRoughness) {
    roughness = GetParam();
    EXPECT_THROW((void)make_config(), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ConfigValidation, InvalidRoughnessTest,
    ::testing::Values(-1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity(),
                     -std::numeric_limits<double>::infinity()),
    invalid_number_name);

} // namespace
} // namespace dsd::sizing::test
