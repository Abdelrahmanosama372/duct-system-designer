#include "network_test_helpers.hpp"
#include "application/network_properties.hpp"

namespace dsd::application::test {
namespace {
DuctNetwork network_for_registration() {
    auto network = DsdApplication{}.prepare_network(
        draft({path("A", {{0, 0}, {1000, 0}}), path("B", {{1000, 0}, {1000, 1000}})}), {});
    network.name = L"Supply Network";
    return network;
}
DuctNetwork valid_network() {
    auto network = network_for_registration();
    for (auto& segment : network.segmented_lines_infos)
        segment.metadata["flow"] = "100";
    return network;
}
TEST(NetworkAnalyzerTest, DefaultsFailWithTheIdsOfEveryUnspecifiedSegment) {
    const auto network = network_for_registration();
    const auto result = NetworkAnalyzer{}.analyze(network);
    ASSERT_FALSE(result.ok());
    ASSERT_EQ(result.issues.size(), 2);
    EXPECT_EQ(result.issues[0].item_id, "DUCT-001");
    EXPECT_EQ(result.issues[1].item_id, "DUCT-002");
}
TEST(NetworkAnalyzerTest, RequiresPositiveFiniteFlowOrBothPositiveNumericDimensions) {
    struct Case {
        const char* flow;
        const char* width;
        const char* height;
        bool valid;
    };
    const std::vector<Case> cases{
        {"1.5", "AC", "AC", true},       {"0", "250.5", "200", true},  {"-10", "250", "200", true},
        {"invalid", "250", "200", true}, {"0", "250", "AC", false},    {"0", "AC", "200", false},
        {"0", "250", "0", false},        {"-1", "AC", "AC", false},    {"nan", "AC", "AC", false},
        {"inf", "AC", "AC", false},      {"10cfm", "AC", "AC", false}, {"0", "nan", "200", false},
        {"", "250", "-1", false},        {"0", "inf", "200", false}};
    for (const auto& item : cases) {
        SCOPED_TRACE(std::string(item.flow) + "," + item.width + "," + item.height);
        auto network = valid_network();
        network.segmented_lines_infos[0].metadata = {
            {"flow", item.flow}, {"width", item.width}, {"height", item.height}};
        EXPECT_EQ(NetworkAnalyzer{}.analyze(network).ok(), item.valid);
    }
}
TEST(NetworkAnalyzerTest, AllowsFlowOnOneSegmentAndDimensionsOnAnotherWithoutChangingMetadata) {
    auto network = valid_network();
    auto& second = network.segmented_lines_infos[1].metadata;
    second = {{"width", "250"}, {"height", "200"}};
    const auto original = second;
    EXPECT_TRUE(NetworkAnalyzer{}.analyze(network).ok());
    EXPECT_EQ(network.segmented_lines_infos[1].metadata, original);
}
TEST(NetworkAnalyzerTest, RejectsEmptyNetworksAndMissingRootSegments) {
    EXPECT_FALSE(NetworkAnalyzer{}.analyze(DuctNetwork{}).ok());
    auto network = valid_network();
    network.network_ids_tree.root_id = "DUCT-missing";
    EXPECT_FALSE(NetworkAnalyzer{}.analyze(network).ok());
    network = valid_network();
    network.root.reset();
    EXPECT_FALSE(NetworkAnalyzer{}.analyze(network).ok());
}
TEST(NetworkAnalyzerTest, TotalFlowUsesOnlyTheRootAndReturnsZeroWhenOnlyRootDimensionsExist) {
    auto network = valid_network();
    network.segmented_lines_infos[0].metadata["flow"] = "125.5";
    network.segmented_lines_infos[1].metadata["flow"] = "700";
    EXPECT_DOUBLE_EQ(network_total_flow(network), 125.5);
    network.segmented_lines_infos[0].metadata = {{"width", "300"}, {"height", "250"}};
    EXPECT_TRUE(NetworkAnalyzer{}.analyze(network).ok());
    EXPECT_DOUBLE_EQ(network_total_flow(network), 0);
}
TEST(NetworkRegistryTest, OwnsRegisteredDataAndModificationsRemainIsolatedUntilReregistration) {
    DsdApplication application;
    auto original = valid_network();
    EXPECT_TRUE(application.registered_networks().empty());
    application.register_network(original);
    original.segmented_lines_infos[0].metadata["flow"] = "999";
    EXPECT_DOUBLE_EQ(network_total_flow(application.registered_network("NET-001")), 100);
    auto editing = application.registered_network("NET-001");
    update_network_item_metadata(editing, NetworkItemType::Segment, "DUCT-001", {{"flow", "450"}});
    EXPECT_DOUBLE_EQ(network_total_flow(application.registered_network("NET-001")), 100);
    application.register_network(std::move(editing));
    EXPECT_EQ(application.registered_networks().size(), 1);
    EXPECT_DOUBLE_EQ(network_total_flow(application.registered_network("NET-001")), 450);
}
TEST(NetworkRegistryTest, RejectsInvalidRegistrationAndPreservesThePreviousSavedNetwork) {
    DsdApplication application;
    EXPECT_THROW(application.register_network(network_for_registration()), std::invalid_argument);
    EXPECT_TRUE(application.registered_networks().empty());
    application.register_network(valid_network());
    auto editing = application.registered_network("NET-001");
    editing.segmented_lines_infos[0].metadata["flow"] = "0";
    EXPECT_THROW(application.register_network(editing), std::invalid_argument);
    EXPECT_DOUBLE_EQ(network_total_flow(application.registered_network("NET-001")), 100);
}
TEST(NetworkRegistryTest, LocksRegisteredNamesAndRejectsDuplicateNamesForDifferentIds) {
    DsdApplication application;
    application.register_network(valid_network());
    auto editing = application.registered_network("NET-001");
    editing.name = L"Renamed";
    EXPECT_THROW(application.register_network(editing), std::invalid_argument);
    auto duplicate = valid_network();
    duplicate.id = "NET-002";
    duplicate.name = L" supply NETWORK ";
    EXPECT_THROW(application.register_network(duplicate), std::invalid_argument);
    EXPECT_EQ(application.registered_networks().size(), 1);
    EXPECT_EQ(application.registered_network("NET-001").name, L"Supply Network");
    duplicate.name = L"Return Network";
    EXPECT_NO_THROW(application.register_network(duplicate));
    EXPECT_EQ(application.registered_networks().size(), 2);
}
TEST(NetworkRegistryTest, RejectsMissingIdentityAndBlankNames) {
    DsdApplication application;
    auto network = valid_network();
    network.id.clear();
    EXPECT_THROW(application.register_network(network), std::invalid_argument);
    network = valid_network();
    network.name = L" \t ";
    EXPECT_THROW(application.register_network(network), std::invalid_argument);
    EXPECT_THROW((void)application.registered_network("missing"), std::invalid_argument);
}
TEST(NetworkRegistryTest, ClearingTheRegistryKeepsProjectInformationAndOtherApplicationInstances) {
    DsdApplication first, second;
    first.register_project_information({});
    first.register_network(valid_network());
    second.register_network(valid_network());
    first.clear_networks();
    EXPECT_TRUE(first.registered_networks().empty());
    EXPECT_TRUE(first.project_information());
    EXPECT_EQ(second.registered_networks().size(), 1);
}
} // namespace
} // namespace dsd::application::test
