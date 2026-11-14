#include "network_test_helpers.hpp"
#include "application/network_properties.hpp"
#include <cmath>
#include <limits>
#include <numbers>

namespace dsd::application::test {
namespace {
DuctNetwork network_with_labels() {
    auto input = draft({path("A", {{0, 0}, {2000, 0}})});
    input.air_terminal_block_name = L"Terminal";
    input.damper_block_name = L"Damper";
    return DsdApplication{}.prepare_network(
        input, {block("T", L"Terminal", {2000, 0}), block("D", L"Damper", {0, 0})});
}
TEST(VisualizationTextFactoryTest, IncludesOnlyCheckedFieldsInOrderAndNeverLabelsDampers) {
    const auto network = network_with_labels();
    for (int mask = 0; mask < 8; ++mask) {
        SCOPED_TRACE(mask);
        const VisualizationTextFields fields{(mask & 1) != 0, (mask & 2) != 0, (mask & 4) != 0};
        const auto texts = DsdApplication{}.create_visualization_texts(network, fields);
        ASSERT_EQ(texts.size(), mask == 0 ? 0 : 2);
        for (const auto& text : texts) {
            EXPECT_NE(text.type, NetworkItemType::Damper);
            std::vector<std::string> expected;
            if (fields.flow)
                expected.push_back("0 cfm");
            if (fields.dimensions)
                expected.push_back(text.type == NetworkItemType::Segment ? "AC x AC"
                                                                         : "none x none");
            if (fields.bod)
                expected.push_back("BOD 0");
            EXPECT_EQ(text.lines, expected);
        }
    }
}
TEST(VisualizationTextFactoryTest, SegmentTextIsCenteredAboveItsRectangleAndFollowsXYDirection) {
    for (double angle : {0.0, std::numbers::pi / 2, -std::numbers::pi / 2, std::numbers::pi / 4,
                         std::numbers::pi}) {
        SCOPED_TRACE(angle);
        const double dx = 2000 * std::cos(angle), dy = 2000 * std::sin(angle);
        auto network =
            DsdApplication{}.prepare_network(draft({path("A", {{0, 0, 100}, {dx, dy, 900}})}), {});
        // No input Z value is consulted while laying out text.
        network.segmented_lines_infos.front().raw_line.end.z =
            std::numeric_limits<double>::quiet_NaN();
        for (auto& corner : network.segmented_lines_vis_rectangles.front().corners)
            corner.z = std::numeric_limits<double>::quiet_NaN();
        const auto texts = VisualizationTextFactory{}.create(network, {true, false, false});
        ASSERT_EQ(texts.size(), 1);
        const auto& text = texts.front();
        expect_xy(text.bottom_center, dx / 2 - 150 * std::sin(angle),
                  dy / 2 + 150 * std::cos(angle));
        EXPECT_NEAR(text.rotation, angle, 1e-7);
        EXPECT_EQ(text.height_mm, 35);
        EXPECT_EQ(text.id, "NET-001:TEXT:DUCT-001");
    }
}
TEST(VisualizationTextFactoryTest, AirTerminalUsesBlockRotationAndClearsEveryRectangleCorner) {
    auto network = network_with_labels();
    for (double angle : {0.0, std::numbers::pi / 2, std::numbers::pi / 4, std::numbers::pi}) {
        SCOPED_TRACE(angle);
        auto terminal = block("T", L"Terminal", {2000, 0});
        terminal.rotation = angle;
        const auto refreshed =
            DsdApplication{}.prepare_network(network, {terminal, block("D", L"Damper", {0, 0})});
        const auto texts = VisualizationTextFactory{}.create(refreshed, {true, true, true});
        ASSERT_EQ(texts.size(), 2);
        const auto& text = texts[1];
        EXPECT_EQ(text.type, NetworkItemType::AirTerminal);
        EXPECT_EQ(text.rotation, angle);
        const double nx = -std::sin(angle), ny = std::cos(angle);
        const auto& rectangle = network.air_terminals_blocks_vis_rectangles.front();
        double nearest = std::numeric_limits<double>::infinity();
        for (const auto& corner : rectangle.corners) {
            const double clearance =
                (text.bottom_center.x - corner.x) * nx + (text.bottom_center.y - corner.y) * ny;
            EXPECT_GE(clearance, 50 - 1e-7);
            nearest = std::min(nearest, clearance);
        }
        EXPECT_NEAR(nearest, 50, 1e-7);
        // The anchor remains centered along the rotated text's horizontal axis.
        EXPECT_NEAR((text.bottom_center.x - 2000) * std::cos(angle) +
                        text.bottom_center.y * std::sin(angle),
                    0, 1e-7);
    }
}
TEST(VisualizationTextFactoryTest, TextHeightAndClearanceAreAdjustableInNetworkOptions) {
    auto network = network_with_labels();
    network.options.visualization_text_height_mm = 60;
    network.options.visualization_text_offset_mm = 80;
    const auto texts = VisualizationTextFactory{}.create(network, {false, false, true});
    ASSERT_EQ(texts.size(), 2);
    expect_xy(texts[0].bottom_center, 1000, 180);
    expect_xy(texts[1].bottom_center, 2000, 140);
    EXPECT_EQ(texts[0].height_mm, 60);
    EXPECT_EQ(texts[1].height_mm, 60);
}
TEST(VisualizationTextFactoryTest, RefreshReadsCurrentMetadataAndUsesTerminalNeckDimensions) {
    auto network = network_with_labels();
    const VisualizationTextFactory factory;
    const auto before = factory.create(network, {true, true, true});
    update_network_item_metadata(network, NetworkItemType::Segment, "DUCT-001",
                                 {{"flow", "425.5"}, {"width", "400"}, {"height", "250"}});
    update_network_item_metadata(
        network, NetworkItemType::AirTerminal, "AT-001",
        {{"flow", "200"}, {"width", "225"}, {"height", "175"}, {"face_size", "600 x 600"}});
    const auto refreshed = factory.create(network, {true, true, false});
    ASSERT_EQ(refreshed.size(), 2);
    EXPECT_EQ(refreshed[0].lines, (std::vector<std::string>{"425.5 cfm", "400 x 250"}));
    EXPECT_EQ(refreshed[1].lines, (std::vector<std::string>{"200 cfm", "225 x 175"}));
    EXPECT_EQ(refreshed[0].id, before[0].id);
    EXPECT_EQ(before[0].lines.front(), "0 cfm");
    EXPECT_TRUE(factory.create(network, {}).empty());
}
TEST(VisualizationTextFactoryTest, RejectsInvalidTextDimensionsAndNonfiniteBlockRotation) {
    const VisualizationTextFactory factory;
    auto network = network_with_labels();
    network.options.visualization_text_height_mm = 0;
    EXPECT_THROW((void)factory.create(network, {true, false, false}), std::invalid_argument);
    network.options.visualization_text_height_mm = 35;
    network.options.visualization_text_offset_mm = -1;
    EXPECT_THROW((void)factory.create(network, {true, false, false}), std::invalid_argument);
    network.options.visualization_text_offset_mm = 50;
    network.air_terminal_blocks_infos.front().raw_block.rotation =
        std::numeric_limits<double>::infinity();
    EXPECT_THROW((void)factory.create(network, {true, false, false}), std::invalid_argument);
}
} // namespace
} // namespace dsd::application::test
