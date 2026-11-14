#include "network_test_helpers.hpp"
#include <limits>
#include <set>

namespace dsd::application::test {
namespace {
TEST(NetworkProcessingTest, ProjectsAllSourceAndOutputGeometryToXYRegardlessOfZ) {
    const double ignored_z = std::numeric_limits<double>::quiet_NaN();
    auto input = draft({path("A", {{0, 0, 100}, {2000, 0, 800}}),
                        path("B", {{2000, 0, -900}, {3000, 0, 200}})});
    input.root->vertices.front().z = ignored_z;
    input.lines.front().vertices.front().z = ignored_z;
    input.lines.back().vertices.back().z = ignored_z;
    input.air_terminal_block_name = L"Terminal";
    auto terminal = block("T", L"Terminal", {1000, 0});
    terminal.insertion.z = ignored_z;
    terminal.bounds.minimum.z = ignored_z;
    terminal.bounds.maximum.z = ignored_z;
    const auto result = DsdApplication{}.prepare_network(input, {terminal});
    ASSERT_EQ(result.lines.size(), 2);
    ASSERT_EQ(result.segmented_lines_infos.size(), 2);
    ASSERT_EQ(result.air_terminal_blocks_infos.size(), 1);
    for (const auto& vertex : result.root->vertices)
        EXPECT_EQ(vertex.z, 0);
    for (const auto& source : result.lines)
        for (const auto& vertex : source.vertices)
            EXPECT_EQ(vertex.z, 0);
    for (const auto& segment : result.segmented_lines_infos) {
        EXPECT_EQ(segment.raw_line.start.z, 0);
        EXPECT_EQ(segment.raw_line.end.z, 0);
    }
    const auto& equipment = result.air_terminal_blocks_infos.front().raw_block;
    EXPECT_EQ(equipment.insertion.z, 0);
    EXPECT_EQ(equipment.bounds.minimum.z, 0);
    EXPECT_EQ(equipment.bounds.maximum.z, 0);
    for (const auto& rectangles :
         {result.segmented_lines_vis_rectangles, result.air_terminals_blocks_vis_rectangles})
        for (const auto& rectangle : rectangles)
            for (const auto& corner : rectangle.corners)
                EXPECT_EQ(corner.z, 0);
}

TEST(NetworkProcessingTest,
     ProducesEquipmentDefaultsSegmentRectanglesAndAnIdTreeWithoutOwningTheDraft) {
    auto input = draft({path("A", {{0, 0}, {2000, 0}})});
    input.air_terminal_block_name = L"Terminal";
    input.damper_block_name = L"Damper";
    const auto result = DsdApplication{}.prepare_network(
        input,
        {block("AT", L"Terminal", {500, 0}), block("VCD", L"Damper", {1500, 0}),
         block("Unmatched", L"Other", {1000, 0}), block("OffLine", L"Terminal", {1000, 1000})});
    EXPECT_TRUE(input.segmented_lines_infos.empty());
    ASSERT_EQ(result.segmented_lines_infos.size(), 3);
    ASSERT_EQ(result.air_terminal_blocks_infos.size(), 1);
    ASSERT_EQ(result.damper_blocks_infos.size(), 1);
    EXPECT_EQ(result.air_terminal_blocks_infos[0].id, "AT-001");
    EXPECT_EQ(result.damper_blocks_infos[0].id, "VCD-001");
    EXPECT_EQ(
        result.air_terminal_blocks_infos[0].metadata,
        (Metadata{{"flow", "0"}, {"width", "none"}, {"height", "none"}, {"face_size", "none"}}));
    EXPECT_EQ(result.damper_blocks_infos[0].metadata, (Metadata{{"damper_type", "none"},
                                                                {"blade_arrangement", "none"},
                                                                {"width", "AC"},
                                                                {"height", "AC"}}));
    EXPECT_EQ(result.network_ids_tree.root_id, "DUCT-001");
    EXPECT_EQ(result.network_ids_tree.parents.size(), 5);
    EXPECT_EQ(result.segmented_lines_vis_rectangles.size(), 3);
    EXPECT_EQ(result.air_terminals_blocks_vis_rectangles.size(), 1);
    EXPECT_EQ(result.damper_blocks_vis_rectangles.size(), 1);
}
TEST(NetworkProcessingTest, AllowsNoEquipmentSelectionsAndDoesNotInventMatchingBlocks) {
    const auto result = DsdApplication{}.prepare_network(draft({path("A", {{0, 0}, {1000, 0}})}),
                                                         {block("B", L"Terminal", {500, 0})});
    EXPECT_EQ(result.segmented_lines_infos.size(), 1);
    EXPECT_TRUE(result.air_terminal_blocks_infos.empty());
    EXPECT_TRUE(result.damper_blocks_infos.empty());
    EXPECT_EQ(result.network_ids_tree.parents.size(), 1);
}
TEST(NetworkProcessingTest, LoopGeometryAppearsOncePerItemInTheRootedTree) {
    const auto result = DsdApplication{}.prepare_network(
        draft({path("A", {{0, 0}, {1000, 0}}), path("B", {{1000, 0}, {1000, 1000}}),
               path("C", {{1000, 1000}, {0, 1000}}), path("D", {{0, 1000}, {0, 0}})}),
        {});
    ASSERT_EQ(result.segmented_lines_infos.size(), 4);
    ASSERT_EQ(result.network_ids_tree.parents.size(), 4);
    for (const auto& [id, parent] : result.network_ids_tree.parents) {
        std::set<std::string> ancestors{id};
        auto current = parent;
        while (current) {
            EXPECT_TRUE(ancestors.insert(*current).second);
            current = result.network_ids_tree.parents.at(*current);
        }
    }
}
TEST(NetworkProcessingTest, EquipmentIntersectionUsesInsertionPointAndItsAdjustableTolerance) {
    auto input = draft({path("A", {{0, 0}, {1000, 0}})});
    input.air_terminal_block_name = L"Terminal";
    const auto near = block("B", L"Terminal", {500, 4});
    EXPECT_EQ(DsdApplication{}.prepare_network(input, {near}).air_terminal_blocks_infos.size(), 1);
    input.options.connection_tolerance_mm = 3;
    EXPECT_TRUE(DsdApplication{}.prepare_network(input, {near}).air_terminal_blocks_infos.empty());
}
TEST(NetworkProcessingTest, KeepsEquipmentMetadataAndIdsWhenRediscoveringTheSameSource) {
    auto input = draft({path("A", {{0, 0}, {2000, 0}})});
    input.air_terminal_block_name = L"Terminal";
    const auto source = block("B", L"Terminal", {1000, 0});
    auto first = DsdApplication{}.prepare_network(input, {source});
    first.air_terminal_blocks_infos[0].metadata["width"] = "200";
    first.air_terminal_blocks_infos[0].metadata["height"] = "150";
    const auto refreshed = DsdApplication{}.prepare_network(first, {source});
    EXPECT_EQ(refreshed.air_terminal_blocks_infos[0].id, first.air_terminal_blocks_infos[0].id);
    EXPECT_EQ(refreshed.air_terminal_blocks_infos[0].metadata,
              first.air_terminal_blocks_infos[0].metadata);
}
} // namespace
} // namespace dsd::application::test
