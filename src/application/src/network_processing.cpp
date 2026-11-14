#include "application/application.hpp"
#include "network_equipment.hpp"
#include "network_topology.hpp"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace dsd::application {

std::string network_item_id(const std::string& prefix, std::size_t number) {
    std::ostringstream value;
    value << prefix << '-' << std::setfill('0') << std::setw(3) << number;
    return value.str();
}

std::vector<SourcePath>
DsdApplication::discover_connected_lines(const SourcePath& root,
                                         const std::vector<SourcePath>& candidates,
                                         const NetworkProcessingOptions& options) const {
    return network_discovery_.discover(root, candidates, options);
}

DuctNetwork DsdApplication::prepare_network(DuctNetwork draft,
                                            const std::vector<geometry::Block>& blocks) const {
    if (!draft.root || draft.id.empty())
        throw std::invalid_argument("Network requires an ID and root");
    if (draft.air_terminal_block_name && draft.damper_block_name &&
        *draft.air_terminal_block_name == *draft.damper_block_name)
        throw std::invalid_argument(
            "Air terminal and damper templates must have different original names");

    draft.lines = network_discovery_.discover(*draft.root, draft.lines, draft.options);
    // Discovery returns the root first, projected into the drawing's XY plane.
    draft.root = draft.lines.front();
    auto equipment = detail::match_equipment(draft, blocks);
    draft.segmented_lines_infos = network_segmentor_.segment(
        draft.lines, equipment.split_points, draft.options, draft.segmented_lines_infos);
    draft.air_terminal_blocks_infos = std::move(equipment.air_terminals);
    draft.damper_blocks_infos = std::move(equipment.dampers);
    draft.network_ids_tree = detail::build_network_ids_tree(draft);

    draft.segmented_lines_vis_rectangles.clear();
    draft.air_terminals_blocks_vis_rectangles.clear();
    draft.damper_blocks_vis_rectangles.clear();
    for (const auto& segment : draft.segmented_lines_infos)
        draft.segmented_lines_vis_rectangles.push_back(
            visualization_rectangle_factory_.create_for_segment(draft.id, segment, draft.options));
    for (const auto& terminal : draft.air_terminal_blocks_infos)
        draft.air_terminals_blocks_vis_rectangles.push_back(
            visualization_rectangle_factory_.create_for_block(
                draft.id, terminal, NetworkItemType::AirTerminal, draft.options));
    for (const auto& damper : draft.damper_blocks_infos)
        draft.damper_blocks_vis_rectangles.push_back(
            visualization_rectangle_factory_.create_for_block(
                draft.id, damper, NetworkItemType::Damper, draft.options));
    return draft;
}
} // namespace dsd::application
