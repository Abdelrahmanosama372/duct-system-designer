#pragma once

#include "application/duct_network.hpp"
#include "application/network_discovery.hpp"
#include "application/network_segmentor.hpp"
#include "application/project_information.hpp"
#include "application/visualization_rectangle_factory.hpp"
#include <optional>

namespace dsd::application {

class DsdApplication {
  public:
    void register_project_information(ProjectInformation information);
    [[nodiscard]] const std::optional<ProjectInformation>& project_information() const noexcept;
    void clear_project_information() noexcept;

    [[nodiscard]] std::vector<SourcePath>
    discover_connected_lines(const SourcePath& root, const std::vector<SourcePath>& candidates,
                             const NetworkProcessingOptions& options = {}) const;
    [[nodiscard]] DuctNetwork prepare_network(DuctNetwork draft,
                                              const std::vector<geometry::Block>& blocks) const;

  private:
    NetworkDiscovery network_discovery_;
    NetworkSegmentor network_segmentor_;
    VisualizationRectangleFactory visualization_rectangle_factory_;
    std::optional<ProjectInformation> project_information_;
};

} // namespace dsd::application
