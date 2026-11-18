#include "application/network_topology_analyzer.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "geometry/placement_context_manager.hpp"
#include "network_test_helpers.hpp"
#include "sizing/equal_velocity.hpp"
#include <numbers>

namespace dsd::application::test {
namespace {
using namespace ductsystem;
class NetworkGenerationTest : public ::testing::Test {
protected:
    DsdApplication application;
    FittingSizingConfig fittings{150, 300, 1.5};
    DuctNetwork network(std::vector<SourcePath> paths) {
        auto value = draft(std::move(paths));
        value.name = L"Supply";
        // Model registered segments explicitly: equipment/property boundaries can
        // preserve collinear pieces that discovery would otherwise merge.
        for (const auto &source : value.lines)
            for (std::size_t i = 1; i < source.vertices.size(); ++i) {
                const auto id = network_item_id("DUCT", value.segmented_lines_infos.size() + 1);
                value.segmented_lines_infos.push_back(
                    {id,
                     {source.vertices[i - 1], source.vertices[i]},
                     {{"flow", "1000"}, {"width", "400"}, {"height", "200"}},
                     {{source.source_id, i - 1}}});
                value.network_ids_tree.parents[id] = std::nullopt;
            }
        value.network_ids_tree.root_id = value.segmented_lines_infos.front().id;
        return value;
    }
    NetworkGenerationResult generate(DuctNetwork value) {
        application.register_network(std::move(value));
        return application.prepare_network_generation("NET-001", fittings);
    }
    template <class T> std::vector<const T *> components(const NetworkGenerationResult &result) {
        std::vector<const T *> values;
        for (const auto *node : result.system->preorder_traversal())
            if (const auto *c = dynamic_cast<const T *>(&node->component()))
                values.push_back(c);
        return values;
    }
};
TEST_F(NetworkGenerationTest, SizesAutomaticRootFromSavedVelocityAndKeepsInputAutomatic) {
    auto value = network({path("a", {{0, 0, 300}, {4000, 0, -900}})});
    value.sizing.method = NetworkSizingMethod::EqualVelocity;
    value.sizing.velocity_fpm = 800;
    value.segmented_lines_infos[0].metadata = {{"flow", "1000"}, {"width", "AC"}, {"height", "AC"}};
    const auto expected = sizing::EqualVelocityDuctSizer{value.sizing.to_sizer_config()}.size_duct(
        sizing::SizingContext{1000});
    const auto result = generate(value);
    const auto ducts = components<RectangularDuct>(result);
    ASSERT_EQ(ducts.size(), 1);
    EXPECT_EQ(ducts[0]->width, expected.width);
    EXPECT_EQ(ducts[0]->height, expected.height);
    EXPECT_EQ(
        application.registered_network(value.id).segmented_lines_infos[0].metadata.at("width"),
        "AC");
    EXPECT_FALSE(application.registered_network_record(value.id).generation);
    for (const auto &edge : result.geometry.at(0).primitives) {
        EXPECT_EQ(std::get<geometry::Line>(edge).start.z, 0);
        EXPECT_EQ(std::get<geometry::Line>(edge).end.z, 0);
    }
}
TEST_F(NetworkGenerationTest, PartialDimensionsAreSizedWithoutKeepingTheEnteredSide) {
    auto value = network({path("a", {{0, 0}, {4000, 0}})});
    value.segmented_lines_infos[0].metadata["height"] = "AC";
    value.segmented_lines_infos[0].metadata["width"] = "1777";
    const auto result = generate(value);
    EXPECT_NE(components<RectangularDuct>(result)[0]->width, 1777);
}
TEST_F(NetworkGenerationTest, PreservesExplicitDimensionsWithoutRequiringFlow) {
    auto value = network({path("a", {{0, 0}, {4000, 0}})});
    value.segmented_lines_infos[0].metadata["flow"] = "0";
    const auto result = generate(value);
    const auto *duct = components<RectangularDuct>(result)[0];
    EXPECT_EQ(duct->width, 400);
    EXPECT_EQ(duct->height, 200);
    EXPECT_DOUBLE_EQ(*duct->length, 4000);
}
TEST_F(NetworkGenerationTest, InsertsReducerAtFixedOffsetAndTrimsAdjacentDucts) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {7000, 0}})});
    ASSERT_EQ(value.segmented_lines_infos.size(), 2);
    value.segmented_lines_infos[1].metadata["width"] = "300";
    value.segmented_lines_infos[1].metadata["height"] = "150";
    const auto result = generate(value);
    const auto reducers = components<RectangularReducer>(result);
    ASSERT_EQ(reducers.size(), 1);
    EXPECT_EQ(reducers[0]->inlet_width, 400);
    EXPECT_EQ(reducers[0]->outlet_width, 300);
    EXPECT_EQ(reducers[0]->length, 300);
    const auto &skeleton = result.geometry.at(reducers[0]->id).layout_skeleton.front();
    expect_xy(skeleton.start, 3600, 0);
    expect_xy(skeleton.end, 3900, 0);
    const auto ducts = components<RectangularDuct>(result);
    EXPECT_EQ(ducts[0]->length, 3600);
    EXPECT_EQ(ducts[1]->length, 3100);
    const auto *root = &result.system->dftree().root();
    ASSERT_EQ(root->children().size(), 1);
    EXPECT_NE(dynamic_cast<const RectangularReducer *>(&root->children()[0]->component()), nullptr);
}
TEST_F(NetworkGenerationTest, RejectsReducerThatLeavesNoDownstreamLength) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {3700, 0}})});
    value.segmented_lines_infos[1].metadata["width"] = "300";
    application.register_network(value);
    EXPECT_THROW((void)application.prepare_network_generation(value.id, fittings),
                 geometry::PlacementError);
}
TEST_F(NetworkGenerationTest, BuildsPlanElbowAndValidatesEqualDimensions) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {3000, 3000}})});
    const auto result = generate(value);
    ASSERT_EQ(components<RectangularElbow>(result).size(), 1);
    EXPECT_NEAR(components<RectangularElbow>(result)[0]->angle, std::numbers::pi / 2, 1e-12);
    value.segmented_lines_infos[1].metadata["height"] = "150";
    application.register_network(value);
    EXPECT_THROW((void)application.prepare_network_generation(value.id, fittings),
                 std::invalid_argument);
}
TEST_F(NetworkGenerationTest, UsesParentWidthForAutomaticDownstreamDuct) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {7000, 0}})});
    value.sizing.method = NetworkSizingMethod::EqualVelocity;
    value.segmented_lines_infos[1].metadata = {{"flow", "500"}};
    const auto expected = sizing::EqualVelocityDuctSizer{value.sizing.to_sizer_config()}.size_duct(
        sizing::SizingContext{500, 400});
    const auto result = generate(value);
    const auto ducts = components<RectangularDuct>(result);
    EXPECT_EQ(ducts[1]->width, expected.width);
    EXPECT_EQ(ducts[1]->height, expected.height);
}
TEST_F(NetworkGenerationTest, BuildsPerpendicularShoesAndCapsOnlyMainWithoutContinuation) {
    const auto result =
        generate(network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {3000, 3000}}),
                          path("c", {{3000, 0}, {3000, -3000}})}));
    EXPECT_EQ(components<RectangularShoeTakeoff>(result).size(), 2);
    EXPECT_EQ(components<RectangularEndcap>(result).size(), 1);
    EXPECT_EQ(components<RectangularDuct>(result)[0]->length, 3800);
}
TEST_F(NetworkGenerationTest, StraightContinuationAtCrossingHasTwoShoesAndNoCap) {
    const auto result =
        generate(network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {7000, 0}}),
                          path("c", {{3000, -3000}, {3000, 0}, {3000, 3000}})}));
    EXPECT_EQ(components<RectangularShoeTakeoff>(result).size(), 2);
    EXPECT_TRUE(components<RectangularEndcap>(result).empty());
}
TEST_F(NetworkGenerationTest, RejectsSkewBranchJunction) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {7000, 0}}),
                          path("c", {{3000, 0}, {6000, 3000}})});
    application.register_network(value);
    EXPECT_THROW((void)application.prepare_network_generation(value.id, fittings),
                 std::invalid_argument);
}
TEST_F(NetworkGenerationTest, EndTerminalGetsDownwardElbowWithRadiusBasedOnHeight) {
    auto value = network({path("a", {{0, 0}, {0, 4000}})});
    value.air_terminal_blocks_infos.push_back(
        {"AT-001",
         block("t", L"Terminal", {0, 4000, 999}),
         {{"width", "200"}, {"height", "100"}, {"flow", "1000"}}});
    const auto result = generate(value);
    const auto elbows = components<RectangularElbow>(result);
    ASSERT_EQ(elbows.size(), 1);
    EXPECT_EQ(elbows[0]->orientation, ElbowOrientation::Downward);
    const auto &rectangle = result.geometry.at(elbows[0]->id);
    ASSERT_EQ(rectangle.primitives.size(), 4);
    const auto &line = std::get<geometry::Line>(rectangle.primitives[0]);
    expect_xy(line.start, 200, 3500);
    expect_xy(line.end, 200, 4000);
    EXPECT_EQ(components<RectangularDuct>(result)[0]->length, 3500);
    EXPECT_TRUE(result.item_components.contains("AT-001"));
}
TEST_F(NetworkGenerationTest, InlineEquipmentIsRetainedWithoutCreatingTerminalElbow) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {7000, 0}})});
    value.air_terminal_blocks_infos.push_back({"AT-001", block("t", L"Terminal", {3000, 0}), {}});
    value.damper_blocks_infos.push_back({"VCD-001", block("d", L"Damper", {3000, 0}), {}});
    const auto result = generate(value);
    EXPECT_TRUE(components<RectangularElbow>(result).empty());
    EXPECT_EQ(result.topology.air_terminals.size(), 1);
    EXPECT_EQ(result.topology.dampers.size(), 1);
}
TEST_F(NetworkGenerationTest, OrientsReversedSourcesFromFreeRootEndpointAndSnapsInXY) {
    auto value = network(
        {path("a", {{3000, 0, 99}, {0, 0, -99}}), path("b", {{7000, 0, 80}, {3003, 0, -80}})});
    const auto topology = TopologyAnalyzer{}.analyze(value);
    ASSERT_EQ(topology.ducts.size(), 2);
    expect_xy(topology.ducts[0].line.start, 0, 0);
    EXPECT_EQ(topology.ducts[0].line.end.x, topology.ducts[1].line.start.x);
    expect_xy(topology.ducts[1].line.end, 7000, 0);
}
TEST_F(NetworkGenerationTest, RejectsPhysicalLoopEvenWhenIdTreeSkippedBackEdges) {
    auto value = network({path("a", {{-3000, 0}, {0, 0}}), path("b", {{0, 0}, {3000, 0}}),
                          path("c", {{3000, 0}, {3000, 3000}}),
                          path("d", {{3000, 3000}, {0, 3000}, {0, 0}})});
    EXPECT_THROW((void)TopologyAnalyzer{}.analyze(value), std::invalid_argument);
}
TEST_F(NetworkGenerationTest, RejectsDisconnectedSegments) {
    auto value = network({path("a", {{0, 0}, {4000, 0}})});
    value.segmented_lines_infos.push_back(
        {"DUCT-002", {{8000, 0}, {12000, 0}}, {{"width", "400"}, {"height", "200"}}, {{"b", 0}}});
    EXPECT_THROW((void)TopologyAnalyzer{}.analyze(value), std::invalid_argument);
}
TEST_F(NetworkGenerationTest, PublishesOnlyOnCommitAndReregistrationInvalidatesResult) {
    auto value = network({path("a", {{0, 0}, {4000, 0}})});
    auto result = generate(value);
    application.commit_network_generation(std::move(result));
    ASSERT_TRUE(application.registered_network_record(value.id).generation);
    const auto system = application.registered_network_record(value.id).generation->system;
    EXPECT_THROW((void)application.prepare_network_generation("missing", fittings),
                 std::invalid_argument);
    EXPECT_EQ(application.registered_network_record(value.id).generation->system, system);
    application.register_network(value);
    EXPECT_FALSE(application.registered_network_record(value.id).generation);
}
TEST_F(NetworkGenerationTest, FailedRegenerationKeepsPreviouslyPublishedGeometry) {
    auto value = network({path("a", {{0, 0}, {3000, 0}}), path("b", {{3000, 0}, {4000, 0}})});
    value.segmented_lines_infos[1].metadata["width"] = "300";
    application.commit_network_generation(generate(value));
    const auto previous = application.registered_network_record(value.id).generation->system;
    EXPECT_THROW((void)application.prepare_network_generation(value.id, {150, 1000, 1.5}),
                 geometry::PlacementError);
    EXPECT_EQ(application.registered_network_record(value.id).generation->system, previous);
    application.clear_network_generation(value.id);
    EXPECT_FALSE(application.registered_network_record(value.id).generation);
}
TEST_F(NetworkGenerationTest, SavesSizingConfigurationAndConvertsRoughnessToMetres) {
    auto value = network({path("a", {{0, 0}, {4000, 0}})});
    value.sizing.roughness_mm = 0.2;
    value.sizing.velocity_fpm = 850;
    application.register_network(value);
    const auto &saved = application.registered_network(value.id).sizing;
    EXPECT_DOUBLE_EQ(saved.to_sizer_config().roughness, 0.0002);
    EXPECT_DOUBLE_EQ(saved.velocity_fpm, 850);
    value.sizing.increment_mm = 0;
    EXPECT_THROW(application.register_network(value), std::invalid_argument);
    EXPECT_DOUBLE_EQ(application.registered_network(value.id).sizing.velocity_fpm, 850);
}
TEST_F(NetworkGenerationTest, SupportsEveryUiMaterialAndExhaustSystem) {
    for (const auto material : {NetworkMaterial::GalvanizedSteel, NetworkMaterial::StainlessSteel,
                                NetworkMaterial::Aluminum, NetworkMaterial::CarbonSteel}) {
        auto value = network({path("a", {{0, 0}, {4000, 0}})});
        value.material = material;
        value.system_type = NetworkSystemType::Exhaust;
        const auto result = generate(value);
        EXPECT_EQ(result.system->config().system_type, SystemType::Exhaust);
    }
}
TEST_F(NetworkGenerationTest, CannotPublishGeometryPreparedBeforeNetworkModification) {
    auto value = network({path("a", {{0, 0}, {4000, 0}})});
    auto result = generate(value);
    value.segmented_lines_infos[0].metadata["width"] = "500";
    application.register_network(value);
    EXPECT_THROW(application.commit_network_generation(std::move(result)), std::invalid_argument);
    EXPECT_FALSE(application.registered_network_record(value.id).generation);
}
TEST_F(NetworkGenerationTest, GeneratesFromActualDiscoveryAndEquipmentSegmentation) {
    auto value = draft({path("a", {{0, 0}, {8000, 0}})});
    value.name = L"Discovered network";
    value.air_terminal_block_name = L"Terminal";
    value.damper_block_name = L"Damper";
    value = application.prepare_network(
        std::move(value), {block("t", L"Terminal", {8000, 0}), block("d", L"Damper", {4000, 0})});
    ASSERT_EQ(value.segmented_lines_infos.size(), 2);
    for (auto &segment : value.segmented_lines_infos)
        segment.metadata = {{"flow", "1000"}, {"width", "400"}, {"height", "200"}};
    const auto result = generate(value);
    EXPECT_EQ(components<RectangularDuct>(result).size(), 2);
    EXPECT_EQ(components<RectangularElbow>(result).size(), 1);
    EXPECT_EQ(result.topology.air_terminals.size(), 1);
    EXPECT_EQ(result.topology.dampers.size(), 1);
}
} // namespace
} // namespace dsd::application::test
