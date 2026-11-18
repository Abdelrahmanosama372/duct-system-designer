#pragma once

#include "application/duct_network.hpp"
#include "application/network_discovery.hpp"
#include "application/network_analyzer.hpp"
#include "application/network_segmentor.hpp"
#include "application/network_geometry_generator.hpp"
#include "application/project_information.hpp"
#include "application/visualization_rectangle_factory.hpp"
#include "application/visualization_text_factory.hpp"
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
    [[nodiscard]] NetworkAnalysisResult analyze_network(const DuctNetwork& network) const;
    [[nodiscard]] std::vector<VisualizationText>
    create_visualization_texts(const DuctNetwork& network,
                               const VisualizationTextFields& fields) const;
    void register_network(DuctNetwork network);
    [[nodiscard]] const std::map<std::string, RegisteredNetwork>& registered_networks() const noexcept;
    [[nodiscard]] const DuctNetwork& registered_network(const std::string& id) const;
    [[nodiscard]] const RegisteredNetwork& registered_network_record(const std::string& id) const;
    [[nodiscard]] NetworkGenerationResult prepare_network_generation(const std::string& id,
        const ductsystem::FittingSizingConfig& fittings) const;
    void commit_network_generation(NetworkGenerationResult result);
    void clear_network_generation(const std::string& id);
    void clear_networks() noexcept;

  private:
    NetworkDiscovery network_discovery_;
    NetworkAnalyzer network_analyzer_;
    NetworkSegmentor network_segmentor_;
    VisualizationRectangleFactory visualization_rectangle_factory_;
    VisualizationTextFactory visualization_text_factory_;
    std::optional<ProjectInformation> project_information_;
    NetworkGeometryGenerator network_geometry_generator_;
    std::map<std::string, RegisteredNetwork> registered_networks_;
};

} // namespace dsd::application
