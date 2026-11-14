#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
class VisualizationRectangleFactory {
  public:
    [[nodiscard]] VisualizationRectangle
    create_for_segment(const std::string& network_id, const LineInfo& segment,
                       const NetworkProcessingOptions& options) const;
    [[nodiscard]] VisualizationRectangle
    create_for_block(const std::string& network_id, const BlockInfo& block, NetworkItemType type,
                     const NetworkProcessingOptions& options) const;
};
} // namespace dsd::application
