#include "network_test_helpers.hpp"
#include <set>

namespace dsd::application::test {
namespace {
TEST(NetworkDiscoveryTest, TraversesEitherEndpointOnTheRootLayerWithoutInteriorCrossings) {
    const auto root = path("A", {{0, 0}, {1000, 0}});
    const std::vector<SourcePath> sources{root,
                                          path("B", {{2000, 0}, {1000, 0}}),
                                          path("C", {{-1000, 0}, {0, 0}}),
                                          path("OtherLayer", {{2000, 0}, {3000, 0}}, L"Other"),
                                          path("Crossing", {{500, -1000}, {500, 1000}}),
                                          path("InteriorT", {{700, 0}, {700, 1000}})};
    const auto result = NetworkDiscovery{}.discover(root, sources, {});
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].source_id, "A");
    EXPECT_EQ(result[1].source_id, "B");
    EXPECT_EQ(result[2].source_id, "C");
}
TEST(NetworkDiscoveryTest, EndpointToleranceIsAdjustable) {
    const auto root = path("A", {{0, 0}, {1000, 0}});
    const std::vector<SourcePath> sources{path("B", {{1004, 0}, {2000, 0}})};
    NetworkProcessingOptions options;
    EXPECT_EQ(NetworkDiscovery{}.discover(root, sources, options).size(), 2);
    options.connection_tolerance_mm = 3;
    EXPECT_EQ(NetworkDiscovery{}.discover(root, sources, options).size(), 1);
}
TEST(NetworkDiscoveryTest, ReturnsEachSourceOnceWhenALoopReturnsToTheRoot) {
    const auto root = path("A", {{0, 0}, {1000, 0}});
    const std::vector<SourcePath> sources{root, root, path("B", {{1000, 0}, {1000, 1000}}),
                                          path("C", {{1000, 1000}, {0, 1000}}),
                                          path("D", {{0, 1000}, {0, 0}})};
    const auto result = NetworkDiscovery{}.discover(root, sources, {});
    ASSERT_EQ(result.size(), 4);
    std::set<std::string> ids;
    for (const auto& source : result)
        EXPECT_TRUE(ids.insert(source.source_id).second);
}
TEST(NetworkDiscoveryTest, RejectsConnectedCurvesButIgnoresUnconnectedCurves) {
    const auto root = path("A", {{0, 0}, {1000, 0}});
    auto curved = path("B", {{5000, 0}, {6000, 0}});
    curved.has_curves = true;
    EXPECT_EQ(NetworkDiscovery{}.discover(root, {curved}, {}).size(), 1);
    curved.vertices = {{1000, 0}, {2000, 0}};
    EXPECT_THROW((void)NetworkDiscovery{}.discover(root, {curved}, {}), std::invalid_argument);
    EXPECT_THROW((void)NetworkDiscovery{}.discover(curved, {}, {}), std::invalid_argument);
}
TEST(NetworkDiscoveryTest, IgnoresAllSourceZCoordinates) {
    const auto root = path("A", {{0, 0, 500}, {1000, 0, 200}});
    const auto result =
        NetworkDiscovery{}.discover(root, {path("B", {{1000, 0, 9000}, {2000, 0, -30}})}, {});
    ASSERT_EQ(result.size(), 2);
    expect_xy(result[0].vertices[0], 0, 0);
    expect_xy(result[1].vertices[1], 2000, 0);
}
TEST(NetworkDiscoveryTest, RejectsInvalidTolerancesAndEmptyRoots) {
    NetworkProcessingOptions options;
    options.connection_tolerance_mm = 0;
    EXPECT_THROW((void)NetworkDiscovery{}.discover(path("A", {{0, 0}, {1000, 0}}), {}, options),
                 std::invalid_argument);
    EXPECT_THROW((void)NetworkDiscovery{}.discover(path("A", {}), {}, {}), std::invalid_argument);
}
} // namespace
} // namespace dsd::application::test
