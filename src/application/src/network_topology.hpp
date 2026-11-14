#pragma once
#include "application/duct_network.hpp"

namespace dsd::application::detail {
NetworkIdsTree build_network_ids_tree(const DuctNetwork& draft);
} // namespace dsd::application::detail
