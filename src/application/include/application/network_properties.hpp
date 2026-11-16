#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
// Properties are stored on the UI-owned network, independently of drawing entities.
Metadata read_network_item_metadata(const DuctNetwork& network, NetworkItemType type,
                                    const std::string& item_id);
void update_network_item_metadata(DuctNetwork& network, NetworkItemType type,
                                  const std::string& item_id, const Metadata& changes);
// Validate all selected items before updating any item or rectangle metadata.
void update_network_items_metadata(DuctNetwork& network, NetworkItemType type,
                                   const std::vector<std::string>& item_ids,
                                   const Metadata& changes);
} // namespace dsd::application
