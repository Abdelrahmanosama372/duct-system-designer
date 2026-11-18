#include "network_test_helpers.hpp"
#include <cmath>
#include <limits>
#include <numbers>

namespace dsd::application::test {
namespace {
class DuctAnnotationsTest : public ::testing::Test {
protected:
    DsdApplication app;
    AnnotationSettings settings{true,     true,       true, 150, 25, AnnotationPlacement::Above,
                                L"Notes", L"My Style"};
    DuctNetwork network(double angle = 0) {
        auto value =
            draft({path("root", {{0, 0}, {4000 * std::cos(angle), 4000 * std::sin(angle)}})});
        value.name = L"Supply";
        value = app.prepare_network(std::move(value), {});
        for (auto& segment : value.segmented_lines_infos)
            segment.metadata = {{"flow", "0"}, {"width", "400"}, {"height", "200"}};
        return value;
    }
    void generate(DuctNetwork value) {
        app.register_network(std::move(value));
        app.commit_network_generation(app.prepare_network_generation("NET-001", {150, 300, 1.5}));
    }
};
TEST_F(DuctAnnotationsTest, RequiresSuccessfulGeometryGeneration) {
    app.register_network(network());
    EXPECT_THROW((void)app.prepare_network_annotations("NET-001", settings), std::invalid_argument);
}
TEST_F(DuctAnnotationsTest, IncludesCheckedFieldsInOrderEvenWhenFlowIsZero) {
    generate(network());
    const auto result = app.prepare_network_annotations("NET-001", settings);
    ASSERT_EQ(result.annotations.size(), 1);
    EXPECT_EQ(result.annotations[0].lines,
              (std::vector<std::string>{"0 cfm", "400 x 200", "BOD 0"}));
    settings.flow = false;
    settings.bod = false;
    EXPECT_EQ(app.prepare_network_annotations("NET-001", settings).annotations[0].lines,
              (std::vector<std::string>{"400 x 200"}));
}
TEST_F(DuctAnnotationsTest, UsesCalculatedDimensionsRatherThanAutomaticInputMetadata) {
    auto value = network();
    value.segmented_lines_infos[0].metadata = {{"flow", "1000"}, {"width", "AC"}, {"height", "AC"}};
    generate(value);
    const auto result = app.prepare_network_annotations("NET-001", settings);
    EXPECT_EQ(result.annotations[0].lines[0], "1000 cfm");
    EXPECT_EQ(result.annotations[0].lines[1].find("AC"), std::string::npos);
    EXPECT_EQ(app.registered_network("NET-001").segmented_lines_infos[0].metadata.at("width"),
              "AC");
}
class AnnotationPositionTest
    : public DuctAnnotationsTest,
      public ::testing::WithParamInterface<std::tuple<double, AnnotationPlacement>> {};
TEST_P(AnnotationPositionTest, CentersTextRelativeToFinalDuctAndKeepsItReadable) {
    const auto [degrees, placement] = GetParam();
    const double angle = degrees * std::numbers::pi / 180;
    generate(network(angle));
    settings.placement = placement;
    const auto annotation =
        app.prepare_network_annotations("NET-001", settings).annotations.front();
    double distance = placement == AnnotationPlacement::Centered ? 25 : 225;
    if (placement == AnnotationPlacement::Below)
        distance = -distance;
    expect_xy(annotation.anchor, 2000 * std::cos(angle) - distance * std::sin(angle),
              2000 * std::sin(angle) + distance * std::cos(angle));
    EXPECT_GE(annotation.rotation, -std::numbers::pi / 2 - 1e-10);
    EXPECT_LE(annotation.rotation, std::numbers::pi / 2 + 1e-10);
    const bool flipped = std::abs(degrees) > 90;
    const auto expected = placement == AnnotationPlacement::Centered
                              ? AnnotationAttachment::MiddleCenter
                              : ((placement == AnnotationPlacement::Above) != flipped
                                     ? AnnotationAttachment::BottomCenter
                                     : AnnotationAttachment::TopCenter);
    EXPECT_EQ(annotation.attachment, expected);
    EXPECT_NEAR(std::abs(std::cos(angle - annotation.rotation)), 1, 1e-12);
}
INSTANTIATE_TEST_SUITE_P(AllDirectionsAndPlacements, AnnotationPositionTest,
                         ::testing::Combine(::testing::Values(0., 90., -90., 180., 135., -135.),
                                            ::testing::Values(AnnotationPlacement::Above,
                                                              AnnotationPlacement::Below,
                                                              AnnotationPlacement::Centered)));
TEST_F(DuctAnnotationsTest, CenteredWithZeroOffsetUsesTheDuctCenter) {
    generate(network());
    settings.placement = AnnotationPlacement::Centered;
    settings.offset_mm = 0;
    expect_xy(app.prepare_network_annotations("NET-001", settings).annotations[0].anchor, 2000, 0);
}
TEST_F(DuctAnnotationsTest, AnnotatesOnlyDuctsAndUsesTrimmedMidpoints) {
    auto value = network();
    value.segmented_lines_infos[0].raw_line.end = {3000, 0};
    value.segmented_lines_infos.push_back({"DUCT-002",
                                           {{3000, 0}, {7000, 0}},
                                           {{"flow", "0"}, {"width", "300"}, {"height", "150"}},
                                           {{"child", 0}}});
    value.air_terminal_blocks_infos.push_back({"AT-001", block("t", L"Terminal", {7000, 0}), {}});
    generate(value);
    const auto result = app.prepare_network_annotations("NET-001", settings);
    ASSERT_EQ(result.annotations.size(), 2);
    EXPECT_EQ(result.annotations[0].item_id, "DUCT-001");
    expect_xy(result.annotations[0].anchor, 1800, 225);
    // Child begins after the reducer at 3900 and ends before its downward elbow at 6625.
    expect_xy(result.annotations[1].anchor, (3900 + 6625) / 2., 175);
}
TEST_F(DuctAnnotationsTest, RegenerationRetainsSettingsAndRebuildsAnnotations) {
    generate(network());
    settings.flow = false;
    app.commit_network_annotations(app.prepare_network_annotations("NET-001", settings));
    auto next = app.prepare_network_generation("NET-001", {200, 400, 2});
    ASSERT_TRUE(next.annotations);
    EXPECT_EQ(next.annotations->settings.text_style, L"My Style");
    EXPECT_EQ(next.annotations->settings.layer, L"Notes");
    EXPECT_FALSE(next.annotations->settings.flow);
    EXPECT_EQ(next.annotations->geometry_revision, next.geometry_revision);
    app.commit_network_generation(std::move(next));
    EXPECT_TRUE(app.registered_network_record("NET-001").generation->annotations);
}
TEST_F(DuctAnnotationsTest, GeometryRegenerationRepositionsExistingAnnotations) {
    auto value = network();
    value.segmented_lines_infos[0].raw_line.end = {3000, 0};
    value.segmented_lines_infos.push_back({"DUCT-002",
                                           {{3000, 0}, {7000, 0}},
                                           {{"flow", "0"}, {"width", "300"}, {"height", "150"}},
                                           {{"child", 0}}});
    generate(value);
    app.commit_network_annotations(app.prepare_network_annotations("NET-001", settings));
    const auto next = app.prepare_network_generation("NET-001", {150, 600, 1.5});
    ASSERT_TRUE(next.annotations);
    expect_xy(next.annotations->annotations[1].anchor, (4200 + 7000) / 2., 175);
}
TEST_F(DuctAnnotationsTest, RemovalIsIndependentAndGeometryRemovalDropsAnnotations) {
    generate(network());
    app.commit_network_annotations(app.prepare_network_annotations("NET-001", settings));
    const auto system = app.registered_network_record("NET-001").generation->system;
    app.clear_network_annotations("NET-001");
    EXPECT_EQ(app.registered_network_record("NET-001").generation->system, system);
    EXPECT_FALSE(app.registered_network_record("NET-001").generation->annotations);
    EXPECT_FALSE(app.prepare_network_generation("NET-001", {150, 300, 1.5}).annotations);
    app.commit_network_annotations(app.prepare_network_annotations("NET-001", settings));
    app.clear_network_generation("NET-001");
    EXPECT_FALSE(app.registered_network_record("NET-001").generation);
    EXPECT_THROW((void)app.prepare_network_annotations("NET-001", settings), std::invalid_argument);
}
TEST_F(DuctAnnotationsTest, RejectsPreparedAnnotationsAfterGeometryOrInputChanges) {
    generate(network());
    auto stale = app.prepare_network_annotations("NET-001", settings);
    app.commit_network_generation(app.prepare_network_generation("NET-001", {150, 300, 1.5}));
    EXPECT_THROW(app.commit_network_annotations(stale), std::invalid_argument);
    stale = app.prepare_network_annotations("NET-001", settings);
    app.register_network(network());
    EXPECT_THROW(app.commit_network_annotations(stale), std::invalid_argument);
}
TEST_F(DuctAnnotationsTest, RejectsInvalidSettingsWithoutReplacingSavedAnnotations) {
    generate(network());
    app.commit_network_annotations(app.prepare_network_annotations("NET-001", settings));
    std::vector<AnnotationSettings> invalid(7, settings);
    invalid[0].flow = invalid[0].dimensions = invalid[0].bod = false;
    invalid[1].text_height_mm = 0;
    invalid[2].offset_mm = -1;
    invalid[3].offset_mm = std::numeric_limits<double>::quiet_NaN();
    invalid[4].layer.clear();
    invalid[5].text_style.clear();
    invalid[6].placement = static_cast<AnnotationPlacement>(99);
    for (const auto& value : invalid)
        EXPECT_THROW((void)app.prepare_network_annotations("NET-001", value),
                     std::invalid_argument);
    EXPECT_EQ(app.registered_network_record("NET-001").generation->annotations->settings.offset_mm,
              25);
}
TEST_F(DuctAnnotationsTest, KeepsAnnotationSettingsWhenRegisteredInputsAreModified) {
    generate(network());
    app.commit_network_annotations(app.prepare_network_annotations("NET-001", settings));
    auto value = network();
    value.segmented_lines_infos[0].metadata["width"] = "500";
    app.register_network(value);
    EXPECT_FALSE(app.registered_network_record("NET-001").generation);
    const auto result = app.prepare_network_generation("NET-001", {150, 300, 1.5});
    ASSERT_TRUE(result.annotations);
    EXPECT_EQ(result.annotations->annotations[0].lines[1], "500 x 200");
    expect_xy(result.annotations->annotations[0].anchor, 2000, 275);
}
} // namespace
} // namespace dsd::application::test
