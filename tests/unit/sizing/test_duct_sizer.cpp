#include "sizing_fixture.hpp"
#include "sizing/equal_velocity.hpp"

#include <cmath>
#include <limits>

namespace dsd::sizing::test {
namespace {

// Expose the protected solver without adding a production API for testing.
class SolverAccess : public DuctSizer {
public:
    using DuctSizer::bisect;
};

TEST(SizingUnits, ConvertsExternalUnitsToSI) {
    EXPECT_NEAR(CFM_TO_M3_S, 0.0004719474432, 1e-16);
    EXPECT_NEAR(FPM_TO_M_S, 0.00508, 1e-15);
    EXPECT_NEAR(IN_WG_PER_100_FT_TO_PA_M, 8.172208333333334, 1e-7);
}

TEST_F(SizingTest, EquivalentDiameterMatchesReferenceAndIsSymmetric) {
    EXPECT_NEAR(DuctSizer::equivalent_diameter(600, 300), 457.01246, 0.00005);
    EXPECT_DOUBLE_EQ(DuctSizer::equivalent_diameter(600, 300),
                     DuctSizer::equivalent_diameter(300, 600));
    for (double invalid : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
        EXPECT_THROW((void)DuctSizer::equivalent_diameter(invalid, 300), std::invalid_argument);
        EXPECT_THROW((void)DuctSizer::equivalent_diameter(600, invalid), std::invalid_argument);
        EXPECT_THROW((void)EqualVelocityDuctSizer{make_config()}.find_eqv_rectangular_duct_dimensions(invalid, 550),
                     std::invalid_argument);
    }
}

TEST_F(SizingTest, EquivalentRectangleRespectsDiscreteConstraints) {
    const EqualVelocityDuctSizer sizer{make_config()};
    const auto result = sizer.find_eqv_rectangular_duct_dimensions(400, 550);
    EXPECT_GE(sizer.equivalent_diameter(result.width, result.height), 400);
    EXPECT_EQ(result.width % 25, 0);
    EXPECT_EQ(result.height % 25, 0);
    EXPECT_LE(static_cast<double>(result.width) / result.height, 2);
    EXPECT_THROW((void)sizer.find_eqv_rectangular_duct_dimensions(10000, 550), NoSolutionWithinBounds);
    EXPECT_THROW((void)sizer.find_eqv_rectangular_duct_dimensions(400, 0), NoSolutionWithinBounds);
    EXPECT_THROW((void)sizer.find_eqv_rectangular_duct_dimensions(400, -1), std::invalid_argument);
}

TEST_F(SizingTest, HeightBisectionReportsExhaustion) {
    dimension_solver_max_iterations = 1;
    EXPECT_THROW((void)EqualVelocityDuctSizer{make_config()}.find_eqv_rectangular_duct_dimensions(400, 550),
                 NumericalConvergenceError);
}

TEST_F(SizingTest, EquivalentRectangleStartsFromSuppliedGridWidth) {
    const EqualVelocityDuctSizer sizer{make_config()};
    const auto result = sizer.find_eqv_rectangular_duct_dimensions(400, 400);

    EXPECT_EQ(result.width, 400);
    EXPECT_GE(sizer.equivalent_diameter(result.width, result.height), 400);
}

TEST_F(SizingTest, EquivalentRectangleRejectsNonGridOrOutOfBoundsStartingWidth) {
    const EqualVelocityDuctSizer sizer{make_config()};

    EXPECT_THROW((void)sizer.find_eqv_rectangular_duct_dimensions(400, 499),
                 std::invalid_argument);
    EXPECT_THROW((void)sizer.find_eqv_rectangular_duct_dimensions(400, 2025),
                 std::invalid_argument);
}

TEST(SizingSolver, PreservesConservativeBoundsAtConfiguredTolerance) {
    for (double epsilon : {1e-3, 1e-9}) {
        const auto [lower, upper] = SolverAccess::bisect(0, 2,
            [](double x) { return x * x >= 2; }, epsilon, 40);
        EXPECT_LE(lower, std::sqrt(2));
        EXPECT_GE(upper, std::sqrt(2));
        EXPECT_LE(upper - lower, epsilon);
        EXPECT_LE(lower * lower, 2);
        EXPECT_GE(upper * upper, 2);
    }
}

TEST(SizingSolver, AcceptsLastIterationAndReportsExhaustion) {
    const auto predicate = [](double x) { return x * x >= 2; };
    const auto [lower, upper] = SolverAccess::bisect(0, 2, predicate, 1, 1);
    EXPECT_EQ(lower, 1);
    EXPECT_EQ(upper, 2);
    EXPECT_THROW((void)SolverAccess::bisect(0, 2, predicate, 1e-9, 1), NumericalConvergenceError);
}

} // namespace
} // namespace dsd::sizing::test
