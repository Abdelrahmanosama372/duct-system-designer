#include "network_test_helpers.hpp"

namespace dsd::application::test {
namespace {
TEST(VisualizationRectangleFactoryTest, SegmentPreviewUsesTwoHundredWidthAndTwoHundredFiftyEndInsets) {
    const LineInfo segment{
        "DUCT-001", geometry::Line{{0, 0, 20}, {1000, 0, 900}}, {{"flow", "0"}}, {{"A", 0}}};
    const auto result = VisualizationRectangleFactory{}.create_for_segment("NET-001", segment, {});
    expect_xy(result.corners[0], 250, 100);
    expect_xy(result.corners[1], 750, 100);
    expect_xy(result.corners[2], 750, -100);
    expect_xy(result.corners[3], 250, -100);
    EXPECT_EQ(result.id, "NET-001:VIS:DUCT-001");
    EXPECT_EQ(result.item_id, "DUCT-001");
    EXPECT_EQ(result.metadata, segment.metadata);
}
TEST(VisualizationRectangleFactoryTest, SegmentPreviewFollowsItsLineDirectionInXY) {
    const LineInfo segment{"DUCT-001", geometry::Line{{0, 0}, {0, 1000}}, {}, {{"A", 0}}};
    const auto result = VisualizationRectangleFactory{}.create_for_segment("NET-001", segment, {});
    expect_xy(result.corners[0], -100, 250);
    expect_xy(result.corners[1], -100, 750);
    expect_xy(result.corners[2], 100, 750);
    expect_xy(result.corners[3], 100, 250);
}
TEST(VisualizationRectangleFactoryTest, EquipmentPreviewOffsetsBoundingRectangleByFifty) {
    const BlockInfo info{"AT-001", block("B", L"Terminal", {0, 0, 800}), {{"width", "none"}}};
    const auto result = VisualizationRectangleFactory{}.create_for_block(
        "NET-001", info, NetworkItemType::AirTerminal, {});
    expect_xy(result.corners[0], -70, -60);
    expect_xy(result.corners[1], 70, -60);
    expect_xy(result.corners[2], 70, 60);
    expect_xy(result.corners[3], -70, 60);
    EXPECT_EQ(result.type, NetworkItemType::AirTerminal);
    EXPECT_EQ(result.metadata, info.metadata);
}
TEST(VisualizationRectangleFactoryTest, NetworkIdScopesOtherwiseIdenticalLocalVisualizationIds) {
    const LineInfo segment{"DUCT-001", geometry::Line{{0, 0}, {1000, 0}}, {}, {{"A", 0}}};
    const VisualizationRectangleFactory factory;
    EXPECT_NE(factory.create_for_segment("NET-001", segment, {}).id,
              factory.create_for_segment("NET-002", segment, {}).id);
}
} // namespace
} // namespace dsd::application::test
