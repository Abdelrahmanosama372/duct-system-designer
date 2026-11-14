#include "application/network_discovery.hpp"
#include "network_geometry.hpp"
#include <deque>
#include <set>

namespace dsd::application {
using namespace detail;
std::vector<SourcePath> NetworkDiscovery::discover(const SourcePath& root,
                                                   const std::vector<SourcePath>& candidates,
                                                   const NetworkProcessingOptions& options) const {
    validate(options);
    if (root.has_curves)
        throw std::invalid_argument("Curved polylines are not supported");
    std::vector<SourcePath> paths{root};
    std::set<std::string> ids{root.source_id};
    for (const auto& path : candidates) {
        if (path.layer == root.layer && ids.insert(path.source_id).second)
            paths.push_back(path);
    }
    const auto all = edges(paths);
    std::deque<std::size_t> pending;
    std::vector<bool> seen(all.size());
    for (std::size_t i = 0; i < all.size(); ++i)
        if (all[i].source.source_id == root.source_id) {
            seen[i] = true;
            pending.push_back(i);
        }
    if (pending.empty())
        throw std::invalid_argument("Root has no usable line segments");
    std::set<std::string> found;
    while (!pending.empty()) {
        const auto i = pending.front();
        pending.pop_front();
        found.insert(all[i].source.source_id);
        for (std::size_t j = 0; j < all.size(); ++j)
            if (!seen[j] && connected(all[i], all[j], options.connection_tolerance_mm)) {
                seen[j] = true;
                pending.push_back(j);
            }
    }
    std::vector<SourcePath> result;
    for (const auto& path : paths)
        if (found.contains(path.source_id)) {
            if (path.has_curves)
                throw std::invalid_argument("Connected network contains a curved polyline");
            auto flattened = path;
            for (auto& vertex : flattened.vertices)
                vertex = Point{vertex.x, vertex.y};
            result.push_back(std::move(flattened));
        }
    return result;
}

} // namespace dsd::application
