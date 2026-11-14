#include "test_helpers.hpp"
#include <stdexcept>

namespace dsd::ductsystem::test {
namespace {

TEST(RectangularDuctTest, CanBeConstructedBeforeSizingAndGeometryAssignment) {
    const RectangularDuct duct{0, 0, pressure, material};
    EXPECT_FALSE(duct.width);
    EXPECT_FALSE(duct.height);
    EXPECT_FALSE(duct.length);
    EXPECT_FALSE(duct.ref_line_id);
}

TEST(RectangularDuctTest, RequiresWidthAndHeightTogether) {
    EXPECT_THROW((void)(RectangularDuct{0, 100, pressure, material, 200}),
                 std::invalid_argument);
    EXPECT_THROW((void)(RectangularDuct{0, 100, pressure, material, std::nullopt, 100}),
                 std::invalid_argument);
}

TEST(RectangularDuctTest, RetainsAssignedDimensionsAndSourceReference) {
    const RectangularDuct duct{7, 500, pressure, material, 400, 250, 1200, 42};
    EXPECT_EQ(duct.width, 400);
    EXPECT_EQ(duct.height, 250);
    EXPECT_EQ(duct.length, 1200);
    EXPECT_EQ(duct.ref_line_id, 42);
}

class InvalidDuctLengthTest : public ::testing::TestWithParam<double> {};

TEST_P(InvalidDuctLengthTest, RejectedDuringConstruction) {
    EXPECT_THROW((void)(RectangularDuct{
        0, 100, pressure, material, std::nullopt, std::nullopt, GetParam()}),
        std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(DuctValidation, InvalidDuctLengthTest,
    ::testing::Values(0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()), invalid_number_name);

} // namespace
} // namespace dsd::ductsystem::test
