#include "network_test_helpers.hpp"
#include <cmath>
#include <numbers>

namespace dsd::application::test {
namespace {
TEST(NetworkSegmentorTest, MergesSmallTurnsIntoAnEndpointToEndpointLine) {
    const double angle = 4 * std::numbers::pi / 180;
    const auto source =
        path("A", {{0, 0}, {1000, 0}, {1000 + 1000 * std::cos(angle), 1000 * std::sin(angle)}});
    const auto result = NetworkSegmentor{}.segment({source}, {}, {});
    ASSERT_EQ(result.size(), 1);
    expect_xy(result[0].raw_line.start, 0, 0);
    expect_xy(result[0].raw_line.end, source.vertices.back().x, source.vertices.back().y);
}
TEST(NetworkSegmentorTest, SplitsOnlyWhenTurnExceedsFiveDegrees) {
    for (double degrees : {5.0, 6.0}) {
        SCOPED_TRACE(degrees);
        const double angle = degrees * std::numbers::pi / 180;
        const auto result = NetworkSegmentor{}.segment(
            {path("A",
                  {{0, 0}, {1000, 0}, {1000 + 1000 * std::cos(angle), 1000 * std::sin(angle)}})},
            {}, {});
        EXPECT_EQ(result.size(), degrees == 5 ? 1 : 2);
    }
}
TEST(NetworkSegmentorTest, DoesNotMergeThroughBranchJunctions) {
    const auto result = NetworkSegmentor{}.segment({path("A", {{0, 0}, {1000, 0}}),
                                                    path("B", {{1000, 0}, {2000, 0}}),
                                                    path("C", {{1000, 0}, {1000, 1000}})},
                                                   {}, {});
    EXPECT_EQ(result.size(), 3);
}
TEST(NetworkSegmentorTest, SplitsAtEquipmentInsertionPointsAndDoesNotDuplicateCuts) {
    const auto result = NetworkSegmentor{}.segment(
        {path("A", {{0, 0}, {2000, 0}})}, {{500, 3, 900}, {500, 3, 1000}, {1500, 0, 800}}, {});
    ASSERT_EQ(result.size(), 3);
    EXPECT_NEAR(result[0].raw_line.length(), 500, 1e-7);
    EXPECT_NEAR(result[1].raw_line.length(), 1000, 1e-7);
    EXPECT_NEAR(result[2].raw_line.length(), 500, 1e-7);
    EXPECT_EQ(result[0].metadata, (Metadata{{"flow", "0"}, {"width", "AC"}, {"height", "AC"}}));
}
TEST(NetworkSegmentorTest, EquipmentWithinEndpointToleranceDoesNotCreateATinyDuct) {
    const auto result = NetworkSegmentor{}.segment({path("A", {{0, 0}, {1000, 0}})}, {{4, 0}}, {});
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].raw_line.length(), 1000);
}
TEST(NetworkSegmentorTest, RejectsFinalDuctsAtOrBelowOneHundredMillimetres) {
    EXPECT_THROW((void)NetworkSegmentor{}.segment({path("A", {{0, 0}, {100, 0}})}, {}, {}),
                 std::invalid_argument);
    EXPECT_THROW((void)NetworkSegmentor{}.segment({path("A", {{0, 0}, {1000, 0}})}, {{50, 0}}, {}),
                 std::invalid_argument);
}
TEST(NetworkSegmentorTest, ShortRawPolylineSectionsCanMergeIntoAValidDuct) {
    const auto result =
        NetworkSegmentor{}.segment({path("A", {{0, 0}, {50, 0}, {1000, 0}})}, {}, {});
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].raw_line.length(), 1000);
}
TEST(NetworkSegmentorTest, PreservesIdsAndUserMetadataWhenRefreshingUnchangedSegments) {
    const auto source = path("A", {{0, 0}, {2000, 0}});
    auto first = NetworkSegmentor{}.segment({source}, {{1000, 0}}, {});
    first[0].metadata["flow"] = "450";
    const auto second = NetworkSegmentor{}.segment({source}, {{1000, 0}}, {}, first);
    ASSERT_EQ(second.size(), 2);
    EXPECT_EQ(second[0].id, first[0].id);
    EXPECT_EQ(second[1].id, first[1].id);
    EXPECT_EQ(second[0].metadata.at("flow"), "450");
}
TEST(NetworkSegmentorTest, EndpointToleranceHandlesTransitivelyConnectedEndpoints) {
    const auto result = NetworkSegmentor{}.segment({path("A", {{0, 0}, {1000, 0}}),
                                                    path("B", {{1004, 0}, {1004, 1000}}),
                                                    path("C", {{1008, 0}, {2000, 0}})},
                                                   {}, {});
    EXPECT_EQ(result.size(), 3);
}
} // namespace
} // namespace dsd::application::test
