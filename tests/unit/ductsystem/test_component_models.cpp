#include "test_helpers.hpp"
#include "ductsystem/models/rectangular/rectangular_duct_fitting.hpp"
#include <stdexcept>

namespace dsd::ductsystem::test {
namespace {

TEST(SheetMetalComponentTest, AllowsUnassignedAirflowAndZeroIdentifier) {
    const SheetMetalComponent component{0, 0, pressure, material};
    EXPECT_EQ(component.id, 0);
    EXPECT_EQ(component.flow, 0);
}

TEST(SheetMetalComponentTest, RejectsNegativeIdentifier) {
    EXPECT_THROW((void)(SheetMetalComponent{-1, 100, pressure, material}), std::invalid_argument);
}

TEST(SheetMetalComponentTest, RejectsInvalidMaterial) {
    EXPECT_THROW((void)(SheetMetalComponent{0, 100, pressure, static_cast<Material>(99)}),
                 std::invalid_argument);
}

TEST(SheetMetalComponentTest, RejectsInvalidPressureLevel) {
    EXPECT_THROW((void)(SheetMetalComponent{0, 100, static_cast<PressureLevel>(99), material}),
                 std::invalid_argument);
}

class InvalidFlowTest : public ::testing::TestWithParam<double> {};

TEST_P(InvalidFlowTest, RejectedDuringConstruction) {
    EXPECT_THROW((void)(SheetMetalComponent{0, GetParam(), pressure, material}),
                 std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ComponentValidation, InvalidFlowTest,
    ::testing::Values(-1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity(),
                     -std::numeric_limits<double>::infinity()), invalid_number_name);

class InvalidDimensionTest : public ::testing::TestWithParam<double> {};

TEST_P(InvalidDimensionTest, RejectsWidth) {
    EXPECT_THROW((void)(RectangularSheetMetalComponent{0, 100, pressure, material, GetParam()}),
                 std::invalid_argument);
}

TEST_P(InvalidDimensionTest, RejectsHeight) {
    EXPECT_THROW((void)(RectangularSheetMetalComponent{
        0, 100, pressure, material, std::nullopt, GetParam()}), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ComponentValidation, InvalidDimensionTest,
    ::testing::Values(0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()), invalid_number_name);

TEST(RectangularComponentTest, DimensionsCanRemainUnassigned) {
    const RectangularSheetMetalComponent component{0, 100, pressure, material};
    EXPECT_FALSE(component.width);
    EXPECT_FALSE(component.height);
}

TEST(RectangularComponentTest, FittingsMayReceiveDimensionsInSeparateStages) {
    const RectangularDuctFitting fitting{0, 100, pressure, material, 200};
    EXPECT_EQ(fitting.width, 200);
    EXPECT_FALSE(fitting.height);
}

} // namespace
} // namespace dsd::ductsystem::test
