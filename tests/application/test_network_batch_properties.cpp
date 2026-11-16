#include "network_test_helpers.hpp"
#include "application/network_properties.hpp"

namespace dsd::application::test {
namespace {
DuctNetwork batch_network() {
    auto input = draft({path("A", {{0, 0}, {10000, 0}})});
    input.air_terminal_block_name = L"Terminal";
    input.damper_block_name = L"Damper";
    return DsdApplication{}.prepare_network(
        input, {block("T1", L"Terminal", {2000, 0}), block("T2", L"Terminal", {4000, 0}),
                block("D1", L"Damper", {6000, 0}), block("D2", L"Damper", {8000, 0})});
}
const auto& item_rectangles(const DuctNetwork& network, NetworkItemType type) {
    return type == NetworkItemType::Segment       ? network.segmented_lines_vis_rectangles
           : type == NetworkItemType::AirTerminal ? network.air_terminals_blocks_vis_rectangles
                                                  : network.damper_blocks_vis_rectangles;
}
TEST(NetworkBatchPropertiesTest, CopiesDisplayedEditableValuesToEverySelectedItemOfEachType) {
    struct Case {
        NetworkItemType type;
        std::string first, second;
        Metadata displayed, different;
    };
    const std::vector<Case> cases{
        {NetworkItemType::Segment,
         "DUCT-002",
         "DUCT-001",
         {{"flow", "400"}, {"width", "500"}, {"height", "250"}},
         {{"flow", "100"}, {"width", "200"}, {"height", "150"}}},
        {NetworkItemType::AirTerminal,
         "AT-002",
         "AT-001",
         {{"flow", "300"}, {"width", "225"}, {"height", "175"}, {"face_size", "400 x 400"}},
         {{"flow", "150"}, {"width", "200"}, {"height", "150"}, {"face_size", "300 x 300"}}},
        {NetworkItemType::Damper,
         "VCD-002",
         "VCD-001",
         {{"damper_type", "Smoke Damper"},
          {"blade_arrangement", "Opposed Blade"},
          {"width", "600"},
          {"height", "350"}},
         {{"damper_type", "Fire Damper"},
          {"blade_arrangement", "Parallel Blade"},
          {"width", "400"},
          {"height", "250"}}}};
    for (const auto& item : cases) {
        SCOPED_TRACE(item.first);
        auto network = batch_network();
        update_network_item_metadata(network, item.type, item.first, item.displayed);
        update_network_item_metadata(network, item.type, item.second, item.different);
        // Only the fields shown by the editor are copied, preserving item-specific metadata.
        update_network_item_metadata(network, item.type, item.first, {{"custom", "first"}});
        update_network_item_metadata(network, item.type, item.second, {{"custom", "second"}});
        update_network_items_metadata(network, item.type, {item.first, item.second},
                                      item.displayed);
        for (const auto& id : {item.first, item.second}) {
            const auto saved = read_network_item_metadata(network, item.type, id);
            for (const auto& [key, value] : item.displayed)
                EXPECT_EQ(saved.at(key), value);
            EXPECT_EQ(saved.at("custom"), id == item.first ? "first" : "second");
            const auto& rectangles = item_rectangles(network, item.type);
            const auto rectangle =
                std::find_if(rectangles.begin(), rectangles.end(),
                             [&](const auto& value) { return value.item_id == id; });
            ASSERT_NE(rectangle, rectangles.end());
            EXPECT_EQ(rectangle->metadata, saved);
        }
        EXPECT_EQ(network.segmented_lines_infos[2].metadata.at("flow"), "0");
    }
}
TEST(NetworkBatchPropertiesTest, AStaleOrWrongTypeMemberCannotPartiallyUpdateEarlierMembers) {
    for (const std::string missing : {"DUCT-999", "AT-001"}) {
        SCOPED_TRACE(missing);
        auto network = batch_network();
        const auto original = network.segmented_lines_infos.front().metadata;
        EXPECT_THROW(update_network_items_metadata(network, NetworkItemType::Segment,
                                                   {"DUCT-001", missing}, {{"flow", "600"}}),
                     std::invalid_argument);
        EXPECT_EQ(network.segmented_lines_infos.front().metadata, original);
        EXPECT_EQ(network.segmented_lines_vis_rectangles.front().metadata, original);
    }
}
TEST(NetworkBatchPropertiesTest,
     InvalidValuesOrInvalidLaterMergedMetadataLeaveTheWholeBatchUnchanged) {
    auto network = batch_network();
    const auto first = network.air_terminal_blocks_infos[0].metadata;
    const auto second = network.air_terminal_blocks_infos[1].metadata;
    EXPECT_THROW(update_network_items_metadata(network, NetworkItemType::AirTerminal,
                                               {"AT-001", "AT-002"},
                                               {{"flow", "-1"}, {"width", "200"}}),
                 std::invalid_argument);
    EXPECT_EQ(network.air_terminal_blocks_infos[0].metadata, first);
    EXPECT_EQ(network.air_terminal_blocks_infos[1].metadata, second);
    EXPECT_EQ(network.air_terminals_blocks_vis_rectangles[0].metadata, first);
    EXPECT_EQ(network.air_terminals_blocks_vis_rectangles[1].metadata, second);
    // If an imported item's other metadata is invalid, staging a valid earlier item
    // still must not mutate it before validation reaches the invalid item.
    network.air_terminal_blocks_infos[1].metadata["face_size"] = "invalid";
    EXPECT_THROW(update_network_items_metadata(network, NetworkItemType::AirTerminal,
                                               {"AT-001", "AT-002"}, {{"flow", "100"}}),
                 std::invalid_argument);
    EXPECT_EQ(network.air_terminal_blocks_infos[0].metadata, first);
    EXPECT_EQ(network.air_terminals_blocks_vis_rectangles[0].metadata, first);
    EXPECT_EQ(network.air_terminal_blocks_infos[1].metadata.at("flow"), "0");
}
TEST(NetworkBatchPropertiesTest, RejectsAnEmptySelectionAndDeduplicatesRepeatedItemIds) {
    auto network = batch_network();
    EXPECT_THROW(
        update_network_items_metadata(network, NetworkItemType::Segment, {}, {{"flow", "100"}}),
        std::invalid_argument);
    const auto before = network.segmented_lines_infos.size();
    update_network_items_metadata(network, NetworkItemType::Segment,
                                  {"DUCT-002", "DUCT-001", "DUCT-002", "DUCT-001"},
                                  {{"flow", "500"}});
    EXPECT_EQ(network.segmented_lines_infos.size(), before);
    EXPECT_EQ(network.segmented_lines_infos[0].metadata.at("flow"), "500");
    EXPECT_EQ(network.segmented_lines_infos[1].metadata.at("flow"), "500");
    EXPECT_EQ(network.segmented_lines_infos[2].metadata.at("flow"), "0");
    EXPECT_EQ(network.segmented_lines_vis_rectangles[0].metadata.at("flow"), "500");
    EXPECT_EQ(network.segmented_lines_vis_rectangles[1].metadata.at("flow"), "500");
}
} // namespace
} // namespace dsd::application::test
