#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
class NetworkSegmentor {
  public:
    [[nodiscard]] std::vector<LineInfo>
    segment(const std::vector<SourcePath>& lines, const std::vector<geometry::Point>& split_points,
            const NetworkProcessingOptions& options,
            const std::vector<LineInfo>& previous_segments = {}) const;
};
} // namespace dsd::application
