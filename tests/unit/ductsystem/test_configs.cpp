#include "test_helpers.hpp"
#include "ductsystem/duct_system_config.hpp"
#include <stdexcept>
#include <tuple>

namespace dsd::ductsystem::test {
namespace {

class InvalidFittingSettingTest : public ::testing::TestWithParam<double> {};

TEST_P(InvalidFittingSettingTest, RejectsTakeoffLength) {
    EXPECT_THROW((void)(FittingSizingConfig{GetParam(), 300, 1.5}), std::invalid_argument);
}
TEST_P(InvalidFittingSettingTest, RejectsReducerLength) {
    EXPECT_THROW((void)(FittingSizingConfig{150, GetParam(), 1.5}), std::invalid_argument);
}
TEST_P(InvalidFittingSettingTest, RejectsInsideRadiusRatio) {
    EXPECT_THROW((void)(FittingSizingConfig{150, 300, GetParam()}), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(ConfigValidation, InvalidFittingSettingTest,
    ::testing::Values(0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()), invalid_number_name);

class SystemClassificationTest : public ::testing::TestWithParam<
    std::tuple<Material, PressureLevel>> {};

TEST_P(SystemClassificationTest, AcceptsSupportedClassification) {
    const auto [chosen_material, chosen_pressure] = GetParam();
    const DuctSystemConfig config{chosen_material, chosen_pressure, {150, 300, 1.5}, SystemType::Supply};
    EXPECT_EQ(config.material, chosen_material);
    EXPECT_EQ(config.pressure_level, chosen_pressure);
}

INSTANTIATE_TEST_SUITE_P(SupportedValues, SystemClassificationTest,
    ::testing::Combine(::testing::Values(Material::GalvanizedSteel, Material::Aluminum),
                       ::testing::Values(PressureLevel::LowPressure, PressureLevel::MediumPressure,
                                         PressureLevel::HighPressure)));

TEST(DuctSystemConfigTest, RejectsInvalidMaterial) {
    EXPECT_THROW((void)(DuctSystemConfig{static_cast<Material>(99), pressure, {150, 300, 1.5}, SystemType::Supply}),
                 std::invalid_argument);
}
TEST(DuctSystemConfigTest, RejectsInvalidPressureLevel) {
    EXPECT_THROW((void)(DuctSystemConfig{material, static_cast<PressureLevel>(99), {150, 300, 1.5}, SystemType::Supply}),
                 std::invalid_argument);
}

TEST(DuctSystemConfigTest, RejectsInvalidSystemType) {
    EXPECT_THROW((void)(DuctSystemConfig{
        material, pressure, {150, 300, 1.5}, static_cast<SystemType>(99)}),
        std::invalid_argument);
}

} // namespace
} // namespace dsd::ductsystem::test
