#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
class NetworkDiscovery {
  public:
    [[nodiscard]] std::vector<SourcePath> discover(const SourcePath& root,
                                                   const std::vector<SourcePath>& candidates,
                                                   const NetworkProcessingOptions& options) const;
};
} // namespace dsd::application
