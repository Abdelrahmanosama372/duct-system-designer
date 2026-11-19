#include "application/network_persistence.hpp"
#include "network_test_helpers.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace dsd::application::test {
namespace {
using namespace persistence;
void update_checksum(std::string &record) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (std::size_t i = 0; i < record.size() - 8; ++i) {
        hash ^= static_cast<unsigned char>(record[i]);
        hash *= 1099511628211ULL;
    }
    for (int i = 0; i < 8; ++i)
        record[record.size() - 8 + i] = static_cast<char>((hash >> (8 * i)) & 255);
}
class NetworkPersistenceTest : public ::testing::Test {
  protected:
    DsdApplication app;
    NetworkRecord sample() {
        auto input = draft({path("A1", {{0, 0}, {3000, 0}}),
                            path("A2", {{3000, 0}, {3000, 3000}, {12000, 3000}}),
                            path("A3", {{3000, 0}, {3000, -3000}})});
        input.name = L"شبكة \U0001F600";
        input.document_key = "runtime-pointer:AA";
        input.air_terminal_block_name = L"Terminal";
        input.damper_block_name = L"Damper";
        auto network =
            app.prepare_network(std::move(input), {block("B1", L"Terminal", {12000, 3000}),
                                                   block("B2", L"Damper", {8000, 3000})});
        for (auto &segment : network.segmented_lines_infos) {
            segment.metadata = {{"flow", "1000"}, {"width", "400"}, {"height", "200"}};
            if (segment.raw_line.start.x >= 8000 && segment.raw_line.end.x >= 8000)
                segment.metadata["width"] = "300";
        }
        app.register_network(network);
        app.commit_network_generation(app.prepare_network_generation(network.id, {150, 300, 1.5}));
        app.set_geometry_output_settings(network.id, {true, L"Ducts", L"Axes", 150, 300, 1.5});
        app.commit_network_annotations(app.prepare_network_annotations(
            network.id,
            {true, true, true, 150, 25, AnnotationPlacement::Above, L"Notes", L"Office"}));
        NetworkRecord record;
        record.registered = app.registered_network_record(network.id);
        record.space_handle = "AA";
        record.visualization_fields = {true, true, true};
        record.visualization_visible = false;
        EntityDefinition outline;
        outline.handle = "C1";
        outline.group = OutputGroup::Visualization;
        outline.kind = EntityKind::Polyline;
        outline.layer.name = L"Visualization Rectangles";
        outline.layer.color = 11;
        outline.closed = true;
        outline.visible = false;
        outline.tag_app = L"DSD_NETWORK_VIS";
        outline.tag_strings = {L"NET-001", L"VIS-001", L"DUCT-001", L"segment"};
        outline.points = {{0, 0}, {100, 0}, {100, 200}, {0, 200}};
        outline.bulges = {0, 0, 0, 0};
        outline.transparency = 128;
        outline.transparency_by_layer = false;
        auto hatch = outline;
        hatch.handle = "C2";
        hatch.kind = EntityKind::Hatch;
        hatch.points.clear();
        hatch.bulges.clear();
        hatch.boundary_handle = "C1";
        EntityDefinition line;
        line.handle = "C3";
        line.group = OutputGroup::Geometry;
        line.kind = EntityKind::Line;
        line.layer.name = L"Ducts";
        line.layer.true_color = true;
        line.layer.rgb = 0x112233;
        line.layer.lineweight = 25;
        line.tag_app = L"DSD_GEOMETRY";
        line.tag_strings = {L"NET-001"};
        line.component_id = 0;
        line.points = {{0, 0}, {1000, 0}};
        line.linetype = L"CENTER";
        line.pattern_length = 50;
        line.dashes = {25, -10, 5, -10};
        EntityDefinition text;
        text.handle = "C4";
        text.group = OutputGroup::Annotation;
        text.kind = EntityKind::Text;
        text.layer.name = L"Notes";
        text.style.name = L"Office";
        text.style.font_file = L"arial.ttf";
        text.text_height = 150;
        text.points = {{500, 300}};
        text.text = L"\\pxqc;1000 cfm\\P400 x 200\\PBOD 0";
        text.attachment = 8;
        text.tag_app = L"DSD_DUCT_ANNOTATIONS";
        text.tag_strings = {L"NET-001", L"DUCT-001"};
        text.component_id = 0;
        record.entities = {outline, hatch, line, text};
        return record;
    }
};
TEST_F(NetworkPersistenceTest, RoundTripPreservesStableSegmentsModelsDefinitionsAndSettings) {
    const auto original = sample();
    const auto encoded = NetworkRecordCodec::encode_network(original);
    const auto restored = NetworkRecordCodec::decode_network(encoded);
    EXPECT_EQ(NetworkRecordCodec::encode_network(restored), encoded);
    EXPECT_EQ(restored.registered.network.name, L"شبكة \U0001F600");
    EXPECT_EQ(restored.registered.network.segmented_lines_infos.size(),
              original.registered.network.segmented_lines_infos.size());
    EXPECT_EQ(restored.registered.network.network_ids_tree.parents,
              original.registered.network.network_ids_tree.parents);
    ASSERT_TRUE(restored.registered.generation);
    EXPECT_EQ(restored.registered.generation->geometry.size(),
              original.registered.generation->geometry.size());
    EXPECT_EQ(restored.registered.generation->item_components,
              original.registered.generation->item_components);
    ASSERT_TRUE(restored.registered.generation->annotations);
    EXPECT_EQ(restored.registered.generation->annotations->settings.text_style, L"Office");
    ASSERT_TRUE(restored.registered.geometry_settings);
    EXPECT_TRUE(restored.registered.geometry_settings->centerlines);
    EXPECT_EQ(restored.registered.geometry_settings->centerline_layer, L"Axes");
    EXPECT_DOUBLE_EQ(restored.registered.geometry_settings->elbow_inside_radius_ratio, 1.5);
    const auto nodes = restored.registered.generation->system->preorder_traversal();
    std::set<int> ids;
    for (const auto *node : nodes)
        ids.insert(node->component().id);
    EXPECT_EQ(ids.size(), nodes.size());
    for (const auto *node : nodes) {
        if (auto parent = node->parent().lock()) {
            EXPECT_TRUE(ids.contains(parent->component().id));
        }
    }
    EXPECT_EQ(restored.entities[1].boundary_handle, "C1");
    EXPECT_EQ(restored.entities[0].transparency, 128);
    EXPECT_FALSE(restored.entities[0].transparency_by_layer);
    EXPECT_EQ(restored.entities[2].dashes, (std::vector<double>{25, -10, 5, -10}));
    EXPECT_EQ(restored.entities[3].text, original.entities[3].text);
    EXPECT_FALSE(restored.visualization_visible);
}
TEST_F(NetworkPersistenceTest,
       StoresSourceHandlesWithoutOriginalCoordinatesOrRuntimeDocumentIdentity) {
    auto original = sample();
    original.registered.network.lines[0].layer = L"PRIVATE SOURCE LAYER";
    original.registered.missing_source_handles = {"temporary diagnostic"};
    const auto restored =
        NetworkRecordCodec::decode_network(NetworkRecordCodec::encode_network(original));
    const auto &network = restored.registered.network;
    EXPECT_TRUE(network.document_key.empty());
    ASSERT_TRUE(network.root);
    EXPECT_EQ(network.root->source_id, "A1");
    EXPECT_TRUE(network.root->vertices.empty());
    for (const auto &line : network.lines) {
        EXPECT_FALSE(line.source_id.empty());
        EXPECT_TRUE(line.vertices.empty());
        EXPECT_TRUE(line.layer.empty());
    }
    ASSERT_EQ(network.air_terminal_blocks_infos.size(), 1);
    EXPECT_EQ(network.air_terminal_blocks_infos[0].raw_block.source_id, "B1");
    EXPECT_TRUE(network.air_terminal_blocks_infos[0].raw_block.name.empty());
    EXPECT_TRUE(restored.registered.consistent());
}
TEST_F(NetworkPersistenceTest, MissingSourcesBlockEveryOutputRecoveryButKeepTheSavedNetwork) {
    const auto record = sample();
    NetworkRecoveryService service;
    const auto plan = service.plan(
        record, [](const auto &handle, bool) { return handle != "B1"; },
        [](const auto &) { return false; });
    EXPECT_FALSE(plan.consistent());
    EXPECT_EQ(plan.missing_sources, (std::vector<std::string>{"B1"}));
    EXPECT_TRUE(plan.missing_entities.empty());
    auto inconsistent = record.registered;
    inconsistent.missing_source_handles = plan.missing_sources;
    app.restore_networks({inconsistent});
    EXPECT_EQ(app.registered_networks().size(), 1);
    EXPECT_THROW((void)app.prepare_network_generation("NET-001", {150, 300, 1.5}),
                 std::invalid_argument);
    EXPECT_THROW(
        (void)app.prepare_network_annotations("NET-001", *inconsistent.annotation_settings),
        std::invalid_argument);
}
TEST_F(NetworkPersistenceTest, PlansOnlyMissingEntitiesAndDistinguishesSourceTypes) {
    const auto record = sample();
    NetworkRecoveryService service;
    std::set<std::string> inspected;
    const auto plan = service.plan(
        record,
        [&](const auto &handle, bool block) {
            inspected.insert(handle);
            EXPECT_EQ(block, handle.starts_with("B"));
            return true;
        },
        [](const auto &entity) { return entity.handle != "C1" && entity.handle != "C4"; });
    EXPECT_TRUE(plan.consistent());
    EXPECT_EQ(plan.missing_entities, (std::vector<std::size_t>{0, 3}));
    EXPECT_EQ(inspected.size(), 5);
}
TEST_F(NetworkPersistenceTest, IntentionalRemovalIsRepresentedByAbsentDefinitions) {
    auto record = sample();
    std::erase_if(record.entities, [](const auto &entity) {
        return entity.group == OutputGroup::Geometry || entity.group == OutputGroup::Annotation;
    });
    const auto loaded =
        NetworkRecordCodec::decode_network(NetworkRecordCodec::encode_network(record));
    const auto plan = NetworkRecoveryService{}.plan(
        loaded, [](const auto &, bool) { return true; }, [](const auto &) { return false; });
    EXPECT_EQ(plan.missing_entities, (std::vector<std::size_t>{0, 1}));
}
TEST_F(NetworkPersistenceTest, EmptyOutputDoesNotGeneratePreviouslyRemovedEntities) {
    auto record = sample();
    record.entities.clear();
    EXPECT_TRUE(
        NetworkRecoveryService{}
            .plan(
                record, [](const auto &, bool) { return true; }, [](const auto &) { return false; })
            .missing_entities.empty());
}
TEST_F(NetworkPersistenceTest, ProjectRoundTripPreservesUnicodeBlankAndMultilineFields) {
    DrawingHeader header;
    header.next_network_id = 42;
    ProjectInformation info;
    info.project_name = L"مشروع \U0001F600";
    info.additional_information = L"First line\r\nSecond line";
    header.project = info;
    const auto restored =
        NetworkRecordCodec::decode_header(NetworkRecordCodec::encode_header(header));
    ASSERT_TRUE(restored.project);
    EXPECT_EQ(*restored.project, info);
    EXPECT_EQ(restored.next_network_id, 42);
    EXPECT_FALSE(NetworkRecordCodec::decode_header(NetworkRecordCodec::encode_header({})).project);
}
TEST_F(NetworkPersistenceTest, RejectsUnsupportedVersionTruncationTrailingDataAndOversizedLengths) {
    const auto encoded = NetworkRecordCodec::encode_network(sample());
    auto future = encoded;
    future[15] = 2;
    update_checksum(future);
    EXPECT_THROW((void)NetworkRecordCodec::decode_network(future), std::invalid_argument);
    for (std::size_t size : {std::size_t{0}, std::size_t{8}, std::size_t{16}, encoded.size() - 1})
        EXPECT_THROW(
            (void)NetworkRecordCodec::decode_network(std::string_view(encoded.data(), size)),
            std::invalid_argument);
    auto corrupted = encoded;
    corrupted[35] ^= 1;
    EXPECT_THROW((void)NetworkRecordCodec::decode_network(corrupted), std::invalid_argument);
    EXPECT_THROW((void)NetworkRecordCodec::decode_network(encoded + "extra"),
                 std::invalid_argument);
    auto invalid_length = encoded;
    for (std::size_t i = 23; i < 31; ++i)
        invalid_length[i] = static_cast<char>(255);
    update_checksum(invalid_length);
    EXPECT_THROW((void)NetworkRecordCodec::decode_network(invalid_length), std::invalid_argument);
}
TEST_F(NetworkPersistenceTest, RejectsForeignOwnershipInvalidGeometryAndNonFiniteNumbers) {
    auto record = sample();
    record.entities[0].tag_strings[0] = L"NET-999";
    EXPECT_THROW(
        (void)NetworkRecordCodec::decode_network(NetworkRecordCodec::encode_network(record)),
        std::invalid_argument);
    record = sample();
    record.entities[0].bulges.clear();
    EXPECT_THROW(
        (void)NetworkRecordCodec::decode_network(NetworkRecordCodec::encode_network(record)),
        std::invalid_argument);
    record = sample();
    record.registered.network.system_type = static_cast<NetworkSystemType>(99);
    EXPECT_THROW(
        (void)NetworkRecordCodec::decode_network(NetworkRecordCodec::encode_network(record)),
        std::invalid_argument);
    record = sample();
    record.entities[0].points[0].x = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((void)NetworkRecordCodec::encode_network(record), std::invalid_argument);
}
TEST_F(NetworkPersistenceTest, RestoresMultipleNetworksAtomicallyWithoutRegistrationAnalysis) {
    auto record = sample();
    auto second = record.registered;
    second.network.id = "NET-002";
    second.network.name = L"Other";
    second.missing_source_handles = {"B1"};
    app.restore_networks({record.registered, second});
    EXPECT_EQ(app.registered_networks().size(), 2);
    EXPECT_FALSE(app.registered_network_record("NET-002").consistent());
    EXPECT_THROW(app.restore_networks({record.registered, record.registered}),
                 std::invalid_argument);
    EXPECT_EQ(app.registered_networks().size(), 2);
}
} // namespace
} // namespace dsd::application::test
