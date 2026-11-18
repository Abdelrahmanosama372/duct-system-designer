#pragma once
#include "application/duct_system_builder.hpp"
#include "application/registered_network.hpp"
#include "ductsystem/duct_system_sizer.hpp"

namespace dsd::application {
class NetworkGeometryGenerator {
public:
    [[nodiscard]] NetworkGenerationResult
    generate(const DuctNetwork &network, const ductsystem::FittingSizingConfig &fittings) const;

private:
    TopologyAnalyzer topology_analyzer_;
    DuctSystemBuilder system_builder_;
    ductsystem::DuctSystemSizer system_sizer_;
};
} // namespace dsd::application
