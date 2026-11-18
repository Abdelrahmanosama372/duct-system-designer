#pragma once
#include "application/network_topology_analyzer.hpp"
#include "ductsystem/duct_system.hpp"

namespace dsd::application {
class DuctSystemBuilder {
public:
    [[nodiscard]] ductsystem::DuctSystem build(const std::string &name,
                                               const NetworkTopology &topology,
                                               const ductsystem::DuctSystemConfig &config) const;
};
} // namespace dsd::application
