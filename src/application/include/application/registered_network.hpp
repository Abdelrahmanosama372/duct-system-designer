#pragma once
#include "application/network_topology_analyzer.hpp"
#include "application/duct_annotations.hpp"
#include "ductsystem/duct_system.hpp"
#include "geometry/geometric_representation.hpp"
#include <cstdint>
#include <memory>

namespace dsd::application {
struct NetworkGenerationResult {
    std::string network_id;
    std::uint64_t source_revision = 0;
    std::shared_ptr<const ductsystem::DuctSystem> system;
    NetworkTopology topology;
    geometry::GeometryRepresentations geometry;
    std::map<std::string, std::vector<int>> item_components;
    std::uint64_t geometry_revision = 0;
    std::optional<AnnotationGenerationResult> annotations;
};
struct RegisteredNetwork {
    DuctNetwork network;
    std::optional<NetworkGenerationResult> generation;
    std::uint64_t revision = 1;
    std::uint64_t geometry_revision = 0;
    std::optional<AnnotationSettings> annotation_settings = std::nullopt;
};
} // namespace dsd::application
