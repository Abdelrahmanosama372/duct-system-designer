#include "sizing_fixture.hpp"
#include "sizing/equal_friction.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <tuple>

namespace dsd::sizing::test {
namespace {

class ColebrookTest : public SizingTest,
    public ::testing::WithParamInterface<std::tuple<double, double, double>> {};

TEST_P(ColebrookTest, MatchesIndependentHighPrecisionReference) {
    const auto [reynolds, roughness, expected] = GetParam();
    this->roughness = roughness;
    const double factor = EqualFrictionDuctSizer{make_config()}.friction_factor(reynolds, 500);
    EXPECT_NEAR(factor, expected, expected * 1e-10);
    const double residual = 1 / std::sqrt(factor) + 2 * std::log10(
        roughness / (3.7 * 0.5) + 2.51 / (reynolds * std::sqrt(factor)));
    EXPECT_LT(std::abs(residual), 1e-8);
}

INSTANTIATE_TEST_SUITE_P(KnownFactors, ColebrookTest, ::testing::Values(
    std::tuple{100000.0, 0.00005, 0.01851386607747164},
    std::tuple{100000.0, 0.0, 0.017989773084273838},
    std::tuple{10000.0, 0.0005, 0.03238180636309272}));

TEST_F(SizingTest, LaminarFactorAndTransitionMatchPython) {
    const EqualFrictionDuctSizer sizer{make_config()};
    EXPECT_DOUBLE_EQ(sizer.friction_factor(1000, 100), 0.064);
    EXPECT_DOUBLE_EQ(sizer.friction_factor(2299, 100), 64.0 / 2299);
    EXPECT_NEAR(sizer.friction_factor(2300, 100), 0.048485832104955015, 4.8e-12);
}

TEST_F(SizingTest, LaminarLossMatchesIndependentPoiseuilleEquation) {
    const double q = 0.1 * 0.0004719474432;
    const double expected = 128 * dynamic_viscosity * q /
                            (std::numbers::pi * std::pow(0.1, 4));
    EXPECT_NEAR(EqualFrictionDuctSizer{make_config()}.pressure_loss_pa_m(0.1, 100),
                expected, expected * 1e-10);
}

TEST_F(SizingTest, EquivalentDiameterMatchesIndependentDarcyCase) {
    const double velocity = 100000 * dynamic_viscosity / (air_density * 0.5);
    const double flow = velocity * std::numbers::pi * 0.5 * 0.5 / 4 / 0.0004719474432;
    const double loss = 0.01851386607747164 * air_density * velocity * velocity;
    roughness = 0.00005;
    required_head_loss = loss * 30.48 / 249.08891;
    const EqualFrictionDuctSizer sizer{make_config()};
    const double diameter = sizer.find_equivalent_diameter(flow);
    EXPECT_NEAR(diameter, 500, diameter_tolerance_mm);
    EXPECT_LE(sizer.pressure_loss_pa_m(flow, diameter), loss);
}

TEST_F(SizingTest, FrictionSelectionRespectsTargetAndPreferredWidth) {
    const EqualFrictionDuctSizer sizer{make_config()};
    for (double flow : {1.0, 100.0, 1000.0, 5000.0}) {
        for (const auto width : {std::optional<double>{}, std::optional<double>{500}}) {
            const auto result = sizer.size_duct(SizingContext{flow, width});
            const double diameter = 1.3 * std::pow(static_cast<double>(result.width) * result.height, 0.625) /
                                    std::pow(static_cast<double>(result.width) + result.height, 0.25);
            EXPECT_LE(sizer.pressure_loss_pa_m(flow, diameter),
                      *required_head_loss * 249.08891 / 30.48);
            EXPECT_EQ(result.width % 25, 0);
            EXPECT_EQ(result.height % 25, 0);
            EXPECT_GE(result.width, 25);
            EXPECT_GE(result.height, 25);
            EXPECT_LE(result.width, 2000);
            EXPECT_LE(result.height, 2000);
            EXPECT_LE(static_cast<double>(result.width) / result.height, 2);
            if (width) {
                EXPECT_LE(result.width, *width);
            }
        }
    }
}

TEST_F(SizingTest, RoughnessIncreasesRequiredDiameter) {
    const double rough = EqualFrictionDuctSizer{make_config()}.find_equivalent_diameter(1000);
    roughness = 0;
    EXPECT_GT(rough, EqualFrictionDuctSizer{make_config()}.find_equivalent_diameter(1000));
}

TEST_F(SizingTest, FrictionReportsImpossibleRequestsAndAcceptsSmallFlows) {
    const EqualFrictionDuctSizer sizer{make_config()};
    EXPECT_EQ(sizer.find_equivalent_diameter(0.00001), 20);
    EXPECT_EQ(sizer.size_duct(SizingContext{0.00001}).height, 25);
    EXPECT_THROW((void)sizer.size_duct(SizingContext{1000, 10}), NoSolutionWithinBounds);
    maximum_duct_dimension = 100;
    EXPECT_THROW((void)EqualFrictionDuctSizer{make_config()}.size_duct(SizingContext{100000}), NoSolutionWithinBounds);
    roughness = 10;
    EXPECT_THROW((void)EqualFrictionDuctSizer{make_config()}.friction_factor(100000, 1), NoSolutionWithinBounds);
}

TEST_F(SizingTest, FrictionTargetIsMandatoryAndIterationExhaustionIsExplicit) {
    required_head_loss.reset();
    EXPECT_THROW((void)(EqualFrictionDuctSizer{make_config()}), std::invalid_argument);
    required_head_loss = 0.1;
    dimension_solver_max_iterations = 1;
    EXPECT_THROW((void)EqualFrictionDuctSizer{make_config()}.size_duct(SizingContext{1000}), NumericalConvergenceError);
    dimension_solver_max_iterations = 30;
    friction_factor_solver_max_iterations = 1;
    EXPECT_THROW((void)EqualFrictionDuctSizer{make_config()}.size_duct(SizingContext{1000}), NumericalConvergenceError);
    friction_factor_solver_max_iterations = 5; // Brackets the root, but cannot converge.
    EXPECT_THROW((void)EqualFrictionDuctSizer{make_config()}.friction_factor(100000, 500), NumericalConvergenceError);
}

TEST_F(SizingTest, PublicFrictionHelpersRejectInvalidInputs) {
    const EqualFrictionDuctSizer sizer{make_config()};
    for (double invalid : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
        EXPECT_THROW((void)sizer.find_equivalent_diameter(invalid), std::invalid_argument);
        EXPECT_THROW((void)sizer.pressure_loss_pa_m(invalid, 100), std::invalid_argument);
        EXPECT_THROW((void)sizer.pressure_loss_pa_m(100, invalid), std::invalid_argument);
        EXPECT_THROW((void)sizer.friction_factor(invalid, 100), std::invalid_argument);
        EXPECT_THROW((void)sizer.friction_factor(100000, invalid), std::invalid_argument);
    }
}

struct FrictionCase {
    double flow;
    double loss_pa_m;
    int width;
    int height;
    double equivalent_mm;
};

class FrictionRegressionTest : public SizingTest,
    public ::testing::WithParamInterface<FrictionCase> {};

TEST_P(FrictionRegressionTest, MatchesPythonDimensionsAndMeetsFrictionTarget) {
    const auto reference = GetParam();
    roughness = 0.000213;
    required_head_loss = reference.loss_pa_m * 30.48 / 249.08891;
    const EqualFrictionDuctSizer sizer{make_config()};
    const auto result = sizer.size_duct(SizingContext{reference.flow});
    EXPECT_EQ(result, (DuctDimension{reference.width, reference.height}));
    const double diameter = sizer.equivalent_diameter(result.width, result.height);
    EXPECT_NEAR(diameter, reference.equivalent_mm, 0.001);
    EXPECT_LE(sizer.pressure_loss_pa_m(reference.flow, diameter), reference.loss_pa_m * (1 + 1e-12));
}

INSTANTIATE_TEST_SUITE_P(ReferenceCases, FrictionRegressionTest, ::testing::Values(
    FrictionCase{100, 0.5, 225, 125, 181.388259},
    FrictionCase{100, 0.8, 200, 100, 152.337487},
    FrictionCase{100, 1.2, 200, 100, 152.337487},
    FrictionCase{500, 0.5, 400, 200, 304.674973},
    FrictionCase{500, 0.8, 350, 200, 286.443489},
    FrictionCase{500, 1.2, 350, 175, 266.590602},
    FrictionCase{1000, 0.5, 500, 275, 400.919438},
    FrictionCase{1000, 0.8, 450, 250, 362.776519},
    FrictionCase{1000, 1.2, 425, 225, 333.865980},
    FrictionCase{2500, 0.5, 725, 375, 562.439712},
    FrictionCase{2500, 0.8, 650, 325, 495.096831},
    FrictionCase{2500, 1.2, 600, 300, 457.012460},
    FrictionCase{5000, 0.5, 950, 475, 723.603061},
    FrictionCase{5000, 0.8, 850, 425, 647.434318},
    FrictionCase{5000, 1.2, 800, 400, 609.349946}));

} // namespace
} // namespace dsd::sizing::test
