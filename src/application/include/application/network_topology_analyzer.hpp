#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
struct DuctInfo {
    std::string id;
    int component_id;
    geometry::Line line; // Oriented inlet to outlet, snapped in XY.
    double flow;
    std::optional<double> width, height;
    std::optional<std::size_t> parent;
    std::vector<std::size_t> children;
};
struct EquipmentInfo {
    BlockInfo block;
    std::size_t upstream_duct;
    bool at_duct_end;
};
struct NetworkTopology {
    std::vector<DuctInfo> ducts; // Parent before child; the first duct is the root.
    std::vector<EquipmentInfo> air_terminals;
    std::vector<EquipmentInfo> dampers;
};
class TopologyAnalyzer {
public:
    [[nodiscard]] NetworkTopology analyze(const DuctNetwork &network) const;
};
} // namespace dsd::application
