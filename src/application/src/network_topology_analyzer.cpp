#include "application/network_topology_analyzer.hpp"
#include "network_geometry.hpp"
#include <deque>
#include <set>

namespace dsd::application {
namespace {
std::optional<double> positive(const Metadata &metadata, const char *key) {
    const auto found = metadata.find(key);
    if (found == metadata.end())
        return {};
    try {
        std::size_t used;
        const auto value = std::stod(found->second, &used);
        if (std::isfinite(value) && value > 0 &&
            found->second.find_first_not_of(" \t\r\n", used) == std::string::npos)
            return value;
    } catch (const std::exception &) {
    }
    return {};
}
} // namespace
NetworkTopology TopologyAnalyzer::analyze(const DuctNetwork &network) const {
    detail::validate(network.options);
    const auto &segments = network.segmented_lines_infos;
    if (!network.root || segments.empty())
        throw std::invalid_argument("Topology requires a root and duct segments");
    std::vector<geometry::Point> points;
    std::set<std::string> ids;
    for (const auto &segment : segments) {
        if (segment.id.empty() || !ids.insert(segment.id).second)
            throw std::invalid_argument("Topology requires unique duct IDs");
        points.emplace_back(segment.raw_line.start.x, segment.raw_line.start.y);
        points.emplace_back(segment.raw_line.end.x, segment.raw_line.end.y);
    }
    const auto clusters = detail::cluster_points(points, network.options.connection_tolerance_mm);
    std::vector<geometry::Point> junctions(clusters.count);
    std::vector<std::vector<std::size_t>> incident(clusters.count);
    for (std::size_t i = 0; i < points.size(); ++i) {
        junctions[clusters.ids[i]] = points[i];
        incident[clusters.ids[i]].push_back(i / 2);
    }
    std::set<std::size_t> inlet_candidates;
    const auto has_equipment = [&](const geometry::Point& point) {
        const auto matches = [&](const auto& blocks) {
            return std::any_of(blocks.begin(), blocks.end(), [&](const auto& block) {
                return detail::distance(point, block.raw_block.insertion) <=
                       network.options.connection_tolerance_mm;
            });
        };
        return matches(network.air_terminal_blocks_infos) || matches(network.damper_blocks_infos);
    };
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const bool is_root = std::any_of(
            segments[i].sources.begin(), segments[i].sources.end(),
            [&](const auto &piece) { return piece.source_id == network.root->source_id; });
        if (!is_root)
            continue;
        for (std::size_t end = 0; end < 2; ++end) {
            const auto port = clusters.ids[2 * i + end];
            if (incident[port].size() == 1 && !has_equipment(junctions[port]))
                inlet_candidates.insert(port);
        }
    }
    if (inlet_candidates.empty())
        throw std::invalid_argument("Root source requires a free endpoint");
    std::optional<std::size_t> inlet;
    // Equipment may split one root source into several ducts. Its terminal end
    // is occupied; for two free ends retain the source's start-to-end direction.
    if (!network.root->vertices.empty()) {
        for (const auto port : inlet_candidates)
            if (detail::distance(junctions[port], network.root->vertices.front()) <=
                network.options.connection_tolerance_mm) {
                inlet = port;
                break;
            }
    }
    if (!inlet && inlet_candidates.size() == 1)
        inlet = *inlet_candidates.begin();
    if (!inlet)
        throw std::invalid_argument("Root source has more than one possible inlet");
    NetworkTopology result;
    struct Visit {
        std::size_t port;
        std::optional<std::size_t> parent;
    };
    std::deque<Visit> pending{{*inlet, {}}};
    std::vector<bool> visited_ports(clusters.count), visited_segments(segments.size());
    std::vector<std::optional<std::size_t>> incoming(clusters.count);
    while (!pending.empty()) {
        const auto [port, parent] = pending.front();
        pending.pop_front();
        if (visited_ports[port])
            throw std::invalid_argument("Network contains a physical loop");
        visited_ports[port] = true;
        incoming[port] = parent;
        for (const auto index : incident[port]) {
            if (visited_segments[index])
                continue;
            visited_segments[index] = true;
            const auto other = clusters.ids[2 * index] == port ? clusters.ids[2 * index + 1]
                                                               : clusters.ids[2 * index];
            const auto &segment = segments[index];
            const auto id = static_cast<int>(result.ducts.size());
            geometry::Line line{junctions[port], junctions[other], geometry::LineType::CenterLine,
                                id};
            if (line.length() <= network.options.minimum_segment_length_mm)
                throw std::invalid_argument(segment.id +
                                            ": duct is too short after endpoint snapping");
            auto width = positive(segment.metadata, "width"),
                 height = positive(segment.metadata, "height");
            if (!width || !height) {
                width.reset();
                height.reset();
            }
            const double flow = positive(segment.metadata, "flow").value_or(0);
            if (!width && flow <= 0)
                throw std::invalid_argument(segment.id + ": unsized duct requires positive flow");
            const auto next = result.ducts.size();
            result.ducts.push_back({segment.id, id, line, flow, width, height, parent, {}});
            if (parent)
                result.ducts[*parent].children.push_back(next);
            pending.push_back({other, next});
        }
    }
    if (result.ducts.size() != segments.size())
        throw std::invalid_argument("Network contains disconnected segments");
    const auto equipment = [&](const auto &blocks, auto &output) {
        for (const auto &block : blocks) {
            if (block.id.empty() || !ids.insert(block.id).second)
                throw std::invalid_argument("Topology requires unique equipment IDs");
            std::optional<std::size_t> port;
            double nearest = network.options.connection_tolerance_mm;
            for (std::size_t j = 0; j < junctions.size(); ++j) {
                const auto distance = detail::distance(block.raw_block.insertion, junctions[j]);
                if (distance <= nearest) {
                    nearest = distance;
                    port = j;
                }
            }
            if (!port || !incoming[*port])
                throw std::invalid_argument(block.id + ": equipment has no upstream duct");
            const auto duct = *incoming[*port];
            output.push_back({block, duct, result.ducts[duct].children.empty()});
        }
    };
    equipment(network.air_terminal_blocks_infos, result.air_terminals);
    equipment(network.damper_blocks_infos, result.dampers);
    return result;
}
} // namespace dsd::application
