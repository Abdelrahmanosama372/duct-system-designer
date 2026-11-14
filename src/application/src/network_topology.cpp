#include "network_topology.hpp"
#include "network_geometry.hpp"
#include <deque>
#include <set>

namespace dsd::application::detail {
NetworkIdsTree build_network_ids_tree(const DuctNetwork& draft) {
    const double tolerance = draft.options.connection_tolerance_mm;
    NetworkIdsTree result;
    // Build an ID tree by visiting each item once. Loop back-edges are skipped.
    std::map<std::string, std::set<std::string>> adjacency;
    struct Port {
        std::vector<std::string> ducts, blocks;
    };
    std::vector<Point> points;
    std::vector<std::pair<std::string, bool>> items;
    const auto add_port = [&](const Point& point, const std::string& id, bool block) {
        points.push_back(point);
        items.emplace_back(id, block);
        adjacency[id];
    };
    for (const auto& info : draft.segmented_lines_infos) {
        add_port(info.raw_line.start, info.id, false);
        add_port(info.raw_line.end, info.id, false);
    }
    for (const auto& info : draft.air_terminal_blocks_infos) {
        add_port(info.raw_block.insertion, info.id, true);
    }
    for (const auto& info : draft.damper_blocks_infos) {
        add_port(info.raw_block.insertion, info.id, true);
    }
    const auto clusters = cluster_points(points, tolerance);
    std::vector<Port> network_ports(clusters.count);
    for (std::size_t i = 0; i < points.size(); ++i) {
        auto& port = network_ports[clusters.ids[i]];
        (items[i].second ? port.blocks : port.ducts).push_back(items[i].first);
    }
    const auto connect = [&](const std::string& a, const std::string& b) {
        if (a != b) {
            adjacency[a].insert(b);
            adjacency[b].insert(a);
        }
    };
    for (const auto& port : network_ports) {
        std::vector<std::string> ids = port.blocks;
        ids.insert(ids.end(), port.ducts.begin(), port.ducts.end());
        for (std::size_t i = 1; i < ids.size(); ++i)
            connect(ids[0], ids[i]);
    }
    const auto root_segment =
        std::find_if(draft.segmented_lines_infos.begin(), draft.segmented_lines_infos.end(),
                     [&](const auto& segment) {
                         return std::any_of(segment.sources.begin(), segment.sources.end(),
                                            [&](const auto& source) {
                                                return source.source_id == draft.root->source_id;
                                            });
                     });
    if (root_segment == draft.segmented_lines_infos.end())
        throw std::invalid_argument("Root segment is missing");
    const auto& root = root_segment->id;
    result.root_id = root;
    result.parents[root] = std::nullopt;
    std::deque<std::string> pending{root};
    while (!pending.empty()) {
        const auto id = pending.front();
        pending.pop_front();
        for (const auto& next : adjacency[id])
            if (!result.parents.contains(next)) {
                result.parents[next] = id;
                pending.push_back(next);
            }
    }
    if (result.parents.size() != adjacency.size())
        throw std::invalid_argument("Segmented items do not form a connected network");
    return result;
}
} // namespace dsd::application::detail
