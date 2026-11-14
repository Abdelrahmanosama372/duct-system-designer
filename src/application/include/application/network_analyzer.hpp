#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
struct NetworkAnalysisIssue {
    std::string item_id;
    std::string message;
};
struct NetworkAnalysisResult {
    std::vector<NetworkAnalysisIssue> issues;
    [[nodiscard]] bool ok() const noexcept { return issues.empty(); }
};
class NetworkAnalyzer {
  public:
    [[nodiscard]] NetworkAnalysisResult analyze(const DuctNetwork& network) const;
};
// Total flow is the root segment's positive flow; dimensions alone imply zero.
[[nodiscard]] double network_total_flow(const DuctNetwork& network);
} // namespace dsd::application
