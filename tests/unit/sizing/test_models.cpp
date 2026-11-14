#include "sizing/models.hpp"

#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace dsd::sizing::test {
namespace {
static_assert(!std::is_assignable_v<decltype((std::declval<SizingContext&>().flow)), double>);
static_assert(!std::is_assignable_v<decltype((std::declval<DuctDimension&>().width)), int>);

class InvalidSizingInput : public ::testing::TestWithParam<double> {};

TEST_P(InvalidSizingInput, RejectsFlowAndPreferredWidth) {
    EXPECT_THROW((void)(SizingContext{GetParam()}), std::invalid_argument);
    EXPECT_THROW((void)(SizingContext{100, GetParam()}), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(NonPositiveOrNonFinite, InvalidSizingInput, ::testing::Values(
    0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()));

TEST(SizingModels, KeepsUnitsAndOptionalWidth) {
    const SizingContext root{1000};
    EXPECT_EQ(root.flow, 1000);
    EXPECT_FALSE(root.preferred_width);
    const SizingContext child{500, 450};
    EXPECT_EQ(child.preferred_width, 450);
    EXPECT_EQ((DuctDimension{450, 225}), (DuctDimension{450, 225}));
    EXPECT_NE((DuctDimension{450, 225}), (DuctDimension{225, 450}));
}
} // namespace
} // namespace dsd::sizing::test
