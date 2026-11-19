#include "application/network_persistence.hpp"
#include <set>

namespace dsd::application::persistence {
RecoveryPlan NetworkRecoveryService::plan(
    const NetworkRecord &record,
    const std::function<bool(const std::string &, bool)> &source_exists,
    const std::function<bool(const EntityDefinition &)> &output_exists) const {
    RecoveryPlan result;
    std::set<std::pair<std::string, bool>> sources;
    const auto &network = record.registered.network;
    if (network.root)
        sources.emplace(network.root->source_id, false);
    for (const auto &line : network.lines)
        sources.emplace(line.source_id, false);
    for (const auto &segment : network.segmented_lines_infos)
        for (const auto &source : segment.sources)
            sources.emplace(source.source_id, false);
    for (const auto &block : network.air_terminal_blocks_infos)
        sources.emplace(block.raw_block.source_id, true);
    for (const auto &block : network.damper_blocks_infos)
        sources.emplace(block.raw_block.source_id, true);
    if (!network.root)
        result.missing_sources.push_back("<root>");
    for (const auto &[handle, block] : sources)
        if (handle.empty() || !source_exists(handle, block))
            result.missing_sources.push_back(handle);
    if (!result.consistent())
        return result;
    for (std::size_t i = 0; i < record.entities.size(); ++i)
        if (!output_exists(record.entities[i]))
            result.missing_entities.push_back(i);
    return result;
}
} // namespace dsd::application::persistence
