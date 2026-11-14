#include "network_test_helpers.hpp"
#include "application/network_properties.hpp"

namespace dsd::application::test {
namespace {
DuctNetwork network_with_equipment() {
    auto input = draft({path("A", {{0, 0}, {3000, 0}})});
    input.air_terminal_block_name = L"Terminal";
    input.damper_block_name = L"Damper";
    return DsdApplication{}.prepare_network(
        input, {block("T", L"Terminal", {1000, 0}), block("D", L"Damper", {2000, 0})});
}
TEST(NetworkPropertiesTest, SavesEachItemTypeAndItsRectangleMetadataWithoutChangingOtherItems) {
    auto network = network_with_equipment();
    struct Case {
        NetworkItemType type;
        std::string id;
        Metadata values;
    };
    const std::vector<Case> cases{
        {NetworkItemType::Segment,
         "DUCT-001",
         {{"flow", "350.5"}, {"width", "425"}, {"height", "AC"}}},
        {NetworkItemType::AirTerminal,
         "AT-001",
         {{"flow", "250"}, {"width", "200"}, {"height", "150"}, {"face_size", "300 x 300"}}},
        {NetworkItemType::Damper,
         "VCD-001",
         {{"damper_type", "Fire Damper"},
          {"blade_arrangement", "Parallel Blade"},
          {"width", "AC"},
          {"height", "250"}}}};
    for (const auto& item : cases) {
        SCOPED_TRACE(item.id);
        update_network_item_metadata(network, item.type, item.id, item.values);
        EXPECT_EQ(read_network_item_metadata(network, item.type, item.id), item.values);
        const auto& rectangles = item.type == NetworkItemType::Segment
                                     ? network.segmented_lines_vis_rectangles
                                 : item.type == NetworkItemType::AirTerminal
                                     ? network.air_terminals_blocks_vis_rectangles
                                     : network.damper_blocks_vis_rectangles;
        EXPECT_EQ(rectangles.front().metadata, item.values);
    }
    EXPECT_EQ(network.segmented_lines_infos[1].metadata,
              (Metadata{{"flow", "0"}, {"width", "AC"}, {"height", "AC"}}));
}
TEST(NetworkPropertiesTest, DefaultsCanBeAppliedWithoutLosingAutomaticOrUnsetValues) {
    auto network = network_with_equipment();
    for (const auto& [type, id] : std::vector<std::pair<NetworkItemType, std::string>>{
             {NetworkItemType::Segment, "DUCT-001"},
             {NetworkItemType::AirTerminal, "AT-001"},
             {NetworkItemType::Damper, "VCD-001"}}) {
        const auto values = read_network_item_metadata(network, type, id);
        EXPECT_NO_THROW(update_network_item_metadata(network, type, id, values));
        EXPECT_EQ(read_network_item_metadata(network, type, id), values);
    }
}
TEST(NetworkPropertiesTest, InvalidEditsLeaveStoredItemAndRectangleUnchanged) {
    auto network = network_with_equipment();
    const auto segment = network.segmented_lines_infos.front().metadata;
    const auto terminal = network.air_terminal_blocks_infos.front().metadata;
    const auto damper = network.damper_blocks_infos.front().metadata;
    struct Case {
        NetworkItemType type;
        std::string id;
        Metadata changes;
    };
    for (const auto& edit : std::vector<Case>{
             {NetworkItemType::Segment, "DUCT-001", {{"flow", "-1"}, {"width", "200"}}},
             {NetworkItemType::Segment, "DUCT-001", {{"flow", "nan"}}},
             {NetworkItemType::Segment, "DUCT-001", {{"width", "0"}}},
             {NetworkItemType::Segment, "DUCT-001", {{"height", "250.5"}}},
             {NetworkItemType::AirTerminal, "AT-001", {{"face_size", "bad"}}},
             {NetworkItemType::Damper, "VCD-001", {{"damper_type", "Unknown"}}},
             {NetworkItemType::Damper, "VCD-001", {{"blade_arrangement", "Unknown"}}}}) {
        SCOPED_TRACE(edit.id);
        EXPECT_THROW(update_network_item_metadata(network, edit.type, edit.id, edit.changes),
                     std::invalid_argument);
    }
    EXPECT_EQ(network.segmented_lines_infos.front().metadata, segment);
    EXPECT_EQ(network.segmented_lines_vis_rectangles.front().metadata, segment);
    EXPECT_EQ(network.air_terminal_blocks_infos.front().metadata, terminal);
    EXPECT_EQ(network.air_terminals_blocks_vis_rectangles.front().metadata, terminal);
    EXPECT_EQ(network.damper_blocks_infos.front().metadata, damper);
    EXPECT_EQ(network.damper_blocks_vis_rectangles.front().metadata, damper);
}
TEST(NetworkPropertiesTest, RejectsMissingOrWrongTypeSelections) {
    auto network = network_with_equipment();
    EXPECT_THROW(read_network_item_metadata(network, NetworkItemType::Segment, "DUCT-999"),
                 std::invalid_argument);
    EXPECT_THROW(update_network_item_metadata(network, NetworkItemType::Damper, "AT-001",
                                              {{"width", "200"}}),
                 std::invalid_argument);
}
TEST(NetworkPropertiesTest, ReadingForAnEditorDoesNotSaveUnappliedChanges) {
    auto network = network_with_equipment();
    auto edited = read_network_item_metadata(network, NetworkItemType::Segment, "DUCT-001");
    edited["flow"] = "999";
    EXPECT_EQ(read_network_item_metadata(network, NetworkItemType::Segment, "DUCT-001").at("flow"),
              "0");
}
TEST(NetworkPropertiesTest, AppliedValuesSurviveRefreshAndPreserveUnrelatedMetadata) {
    auto network = network_with_equipment();
    network.segmented_lines_infos.front().metadata["custom"] = "retained";
    update_network_item_metadata(network, NetworkItemType::Segment, "DUCT-001", {{"flow", "400"}});
    update_network_item_metadata(network, NetworkItemType::AirTerminal, "AT-001",
                                 {{"width", "225"}, {"height", "175"}});
    update_network_item_metadata(network, NetworkItemType::Damper, "VCD-001", {{"width", "500"}});
    const auto refreshed = DsdApplication{}.prepare_network(
        network, {block("T", L"Terminal", {1000, 0}), block("D", L"Damper", {2000, 0})});
    EXPECT_EQ(refreshed.segmented_lines_infos.front().metadata.at("flow"), "400");
    EXPECT_EQ(refreshed.segmented_lines_infos.front().metadata.at("custom"), "retained");
    EXPECT_EQ(refreshed.air_terminal_blocks_infos.front().metadata.at("width"), "225");
    EXPECT_EQ(refreshed.air_terminal_blocks_infos.front().metadata.at("height"), "175");
    EXPECT_EQ(refreshed.damper_blocks_infos.front().metadata.at("width"), "500");
}
} // namespace
} // namespace dsd::application::test
