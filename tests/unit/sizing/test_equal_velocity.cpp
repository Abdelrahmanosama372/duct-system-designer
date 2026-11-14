#include "sizing_fixture.hpp"
#include "sizing/equal_velocity.hpp"

#include <limits>

namespace dsd::sizing::test {
namespace {

TEST_F(SizingTest, VelocitySizingMatchesPythonSelectionRules) {
    const EqualVelocityDuctSizer sizer{make_config()};
    EXPECT_EQ(sizer.size_duct(SizingContext{1000}), (DuctDimension{450, 225}));
    EXPECT_EQ(sizer.size_duct(SizingContext{1000, 400}), (DuctDimension{400, 250}));
    EXPECT_EQ(sizer.size_duct(SizingContext{1000, 600}), (DuctDimension{450, 225}));
    EXPECT_EQ(sizer.size_duct(SizingContext{1, 600}), (DuctDimension{50, 25}));
    const auto parent = sizer.size_duct(SizingContext{1000});
    EXPECT_EQ(sizer.size_duct(SizingContext{1000, parent.width}), parent);
}

TEST_F(SizingTest, NonGridBoundsAndPreferredWidthAreRespected) {
    minimum_duct_dimension = 110;
    maximum_duct_dimension = 490;
    const auto result = EqualVelocityDuctSizer{make_config()}.size_duct(SizingContext{1000, 499});
    EXPECT_GE(result.width, 125);
    EXPECT_LE(result.width, 475);
    EXPECT_GE(result.height, 125);
    EXPECT_LE(result.height, 475);
    EXPECT_EQ(result.width % 25, 0);
    EXPECT_EQ(result.height % 25, 0);
    EXPECT_LE(1000 / (result.width * result.height / (304.8 * 304.8)), 1000);
}

TEST_F(SizingTest, ImpossibleRequestsDoNotEnlargePreferredWidth) {
    const EqualVelocityDuctSizer sizer{make_config()};
    EXPECT_THROW((void)sizer.size_duct(SizingContext{1000000}), NoSolutionWithinBounds);
    EXPECT_THROW((void)sizer.size_duct(SizingContext{1000, 10}), NoSolutionWithinBounds);
    EXPECT_THROW((void)sizer.size_duct(SizingContext{1000000, 100}), NoSolutionWithinBounds);
    EXPECT_THROW((void)sizer.size_duct(SizingContext{std::numeric_limits<double>::max()}),
                 NoSolutionWithinBounds);
}

TEST_F(SizingTest, VelocityTargetIsMandatory) {
    required_velocity.reset();
    EXPECT_THROW((void)(EqualVelocityDuctSizer{make_config()}), std::invalid_argument);
}

class VelocityFlowTest : public SizingTest, public ::testing::WithParamInterface<double> {};

TEST_P(VelocityFlowTest, ActualImperialVelocityMeetsTarget) {
    const EqualVelocityDuctSizer sizer{make_config()};
    for (const auto preferred : {std::optional<double>{}, std::optional<double>{2000}}) {
        const auto size = sizer.size_duct(SizingContext{GetParam(), preferred});
        const double area_ft2 = static_cast<double>(size.width) * size.height / (304.8 * 304.8);
        EXPECT_LE(GetParam() / area_ft2, 1000);
        EXPECT_LE(static_cast<double>(size.width) / size.height, aspect_ratio);
        EXPECT_EQ(size.width % 25, 0);
        EXPECT_EQ(size.height % 25, 0);
        EXPECT_GE(size.width, 25);
        EXPECT_GE(size.height, 25);
        EXPECT_LE(size.width, 2000);
        EXPECT_LE(size.height, 2000);
    }
}

INSTANTIATE_TEST_SUITE_P(FlowRange, VelocityFlowTest,
                         ::testing::Values(10.0, 100.0, 500.0, 1000.0, 5000.0, 15000.0));

} // namespace
} // namespace dsd::sizing::test
