#include "application/network_analyzer.hpp"
#include <algorithm>
#include <cmath>
#include <exception>

namespace dsd::application {
namespace {
double positive_value(const Metadata& values, const char* key) {
    const auto found = values.find(key);
    if (found == values.end())
        return 0;
    try {
        std::size_t used = 0;
        const double value = std::stod(found->second, &used);
        if (found->second.find_first_not_of(" \t\r\n", used) != std::string::npos ||
            !std::isfinite(value) || value <= 0)
            return 0;
        return value;
    } catch (const std::exception&) {
        return 0;
    }
}
auto root_segment(const DuctNetwork& network) {
    return std::find_if(
        network.segmented_lines_infos.begin(), network.segmented_lines_infos.end(),
        [&](const auto& segment) { return segment.id == network.network_ids_tree.root_id; });
}
} // namespace
NetworkAnalysisResult NetworkAnalyzer::analyze(const DuctNetwork& network) const {
    NetworkAnalysisResult result;
    if (network.segmented_lines_infos.empty()) {
        result.issues.push_back({network.id, "The network has no duct segments"});
        return result;
    }
    if (!network.root || root_segment(network) == network.segmented_lines_infos.end()) {
        result.issues.push_back({network.id, "The network requires a valid root segment"});
        return result;
    }
    for (const auto& segment : network.segmented_lines_infos) {
        const auto& values = segment.metadata;
        if (positive_value(values, "flow") == 0 &&
            (positive_value(values, "width") == 0 || positive_value(values, "height") == 0))
            result.issues.push_back(
                {segment.id, "Provide positive flow or both positive width and height"});
    }
    return result;
}
double network_total_flow(const DuctNetwork& network) {
    const auto root = root_segment(network);
    return root == network.segmented_lines_infos.end() ? 0 : positive_value(root->metadata, "flow");
}
} // namespace dsd::application
