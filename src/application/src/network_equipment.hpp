#pragma once
#include "application/duct_network.hpp"

namespace dsd::application::detail {
struct MatchedEquipment {
    std::vector<BlockInfo> air_terminals;
    std::vector<BlockInfo> dampers;
    std::vector<geometry::Point> split_points;
};
MatchedEquipment match_equipment(const DuctNetwork& draft,
                                 const std::vector<geometry::Block>& blocks);
} // namespace dsd::application::detail
