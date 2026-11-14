#include "network_equipment.hpp"
#include "network_geometry.hpp"
#include "network_identity.hpp"
#include <set>

namespace dsd::application::detail {
MatchedEquipment match_equipment(const DuctNetwork& draft,
                                 const std::vector<geometry::Block>& blocks) {
    MatchedEquipment result;
    const auto raw = edges(draft.lines);
    const double tolerance = draft.options.connection_tolerance_mm;
    auto next_terminal = next_item_number(draft.air_terminal_blocks_infos);
    auto next_damper = next_item_number(draft.damper_blocks_infos);
    auto sorted_blocks = blocks;
    std::sort(sorted_blocks.begin(), sorted_blocks.end(),
              [](const auto& a, const auto& b) { return a.source_id < b.source_id; });
    std::set<std::string> seen_blocks;
    for (const auto& block : sorted_blocks) {
        const bool terminal =
            draft.air_terminal_block_name && block.name == *draft.air_terminal_block_name;
        const bool damper = draft.damper_block_name && block.name == *draft.damper_block_name;
        if ((!terminal && !damper) || !seen_blocks.insert(block.source_id).second)
            continue;
        if (std::none_of(raw.begin(), raw.end(), [&](const auto& edge) {
                return projection(block.insertion, edge, tolerance).has_value();
            }))
            continue;
        auto& destination = terminal ? result.air_terminals : result.dampers;
        const auto& previous =
            terminal ? draft.air_terminal_blocks_infos : draft.damper_blocks_infos;
        Metadata metadata = terminal ? Metadata{{"flow", "0"},
                                                {"width", "none"},
                                                {"height", "none"},
                                                {"face_size", "none"}}
                                     : Metadata{{"damper_type", "none"},
                                                {"blade_arrangement", "none"},
                                                {"width", "AC"},
                                                {"height", "AC"}};
        const auto old = std::find_if(previous.begin(), previous.end(), [&](const auto& info) {
            return info.raw_block.source_id == block.source_id;
        });
        if (old != previous.end())
            metadata = old->metadata;
        auto flattened = block;
        flattened.insertion = Point{block.insertion.x, block.insertion.y};
        flattened.bounds = {Point{block.bounds.minimum.x, block.bounds.minimum.y},
                            Point{block.bounds.maximum.x, block.bounds.maximum.y}};
        const auto id = old != previous.end()
                            ? old->id
                            : network_item_id(terminal ? "AT" : "VCD",
                                              terminal ? next_terminal++ : next_damper++);
        destination.push_back({id, std::move(flattened), std::move(metadata)});
        result.split_points.push_back(Point{block.insertion.x, block.insertion.y});
    }
    return result;
}
} // namespace dsd::application::detail
