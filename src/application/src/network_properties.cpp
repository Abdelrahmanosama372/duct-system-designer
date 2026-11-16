#include "application/network_properties.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace dsd::application {
namespace {
template <class Items> auto& find_item(Items& items, const std::string& id) {
    const auto item =
        std::find_if(items.begin(), items.end(), [&](const auto& value) { return value.id == id; });
    if (item == items.end())
        throw std::invalid_argument("The selected item no longer belongs to this network");
    return item->metadata;
}
template <class Network>
auto& metadata(Network& network, NetworkItemType type, const std::string& id) {
    switch (type) {
    case NetworkItemType::Segment:
        return find_item(network.segmented_lines_infos, id);
    case NetworkItemType::AirTerminal:
        return find_item(network.air_terminal_blocks_infos, id);
    case NetworkItemType::Damper:
        return find_item(network.damper_blocks_infos, id);
    }
    throw std::invalid_argument("Unknown network item type");
}
auto& rectangles(DuctNetwork& network, NetworkItemType type) {
    switch (type) {
    case NetworkItemType::Segment:
        return network.segmented_lines_vis_rectangles;
    case NetworkItemType::AirTerminal:
        return network.air_terminals_blocks_vis_rectangles;
    case NetworkItemType::Damper:
        return network.damper_blocks_vis_rectangles;
    }
    throw std::invalid_argument("Unknown network item type");
}
bool number(const std::string& input, bool dimension) {
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return false;
    const auto text = input.substr(first, input.find_last_not_of(" \t\r\n") - first + 1);
    std::size_t used = 0;
    try {
        const double value = std::stod(text, &used);
        return used == text.size() && std::isfinite(value) &&
               (dimension ? value > 0 && std::floor(value) == value : value >= 0);
    } catch (const std::exception&) {
        return false;
    }
}
void validate(NetworkItemType type, const Metadata& values) {
    if (type != NetworkItemType::Damper && !number(values.at("flow"), false))
        throw std::invalid_argument("Enter a valid non-negative flow in CFM");
    const auto dimension = [&](const char* key) {
        const auto& value = values.at(key);
        const auto automatic = type == NetworkItemType::AirTerminal ? "none" : "AC";
        if (value != automatic && !number(value, true))
            throw std::invalid_argument("Dimensions must be positive whole millimetres, or AC "
                                        "(duct/damper) or none (neck)");
    };
    dimension("width");
    dimension("height");
    if (type == NetworkItemType::AirTerminal) {
        const auto& face = values.at("face_size");
        const auto separator = face.find('x');
        if (face != "none" &&
            (separator == std::string::npos || !number(face.substr(0, separator), true) ||
             !number(face.substr(separator + 1), true)))
            throw std::invalid_argument("Face size must be none or width x height in millimetres");
    }
    if (type == NetworkItemType::Damper) {
        const auto& damper = values.at("damper_type");
        const auto& blades = values.at("blade_arrangement");
        const std::vector<std::string> types{
            "none",         "Volume Control Damper", "Fire Damper",
            "Smoke Damper", "Fire/Smoke Damper",     "Backdraft Damper"};
        if (std::find(types.begin(), types.end(), damper) == types.end() ||
            (blades != "none" && blades != "Opposed Blade" && blades != "Parallel Blade"))
            throw std::invalid_argument("Select a valid damper type and blade arrangement");
    }
}
} // namespace
Metadata read_network_item_metadata(const DuctNetwork& network, NetworkItemType type,
                                    const std::string& item_id) {
    return metadata(network, type, item_id);
}
void update_network_item_metadata(DuctNetwork& network, NetworkItemType type,
                                  const std::string& item_id, const Metadata& changes) {
    update_network_items_metadata(network, type, {item_id}, changes);
}
void update_network_items_metadata(DuctNetwork& network, NetworkItemType type,
                                   const std::vector<std::string>& item_ids,
                                   const Metadata& changes) {
    if (item_ids.empty())
        throw std::invalid_argument("Select at least one network item");
    struct Update {
        Metadata* stored;
        Metadata replacement;
    };
    std::map<std::string, Update> updates;
    for (const auto& id : item_ids) {
        if (updates.contains(id))
            continue;
        auto& stored = metadata(network, type, id);
        auto replacement = stored;
        for (const auto& [key, value] : changes)
            replacement[key] = value;
        validate(type, replacement);
        updates.emplace(id, Update{&stored, std::move(replacement)});
    }
    // Prepare all allocating work before changing the network.
    auto updated_rectangles = rectangles(network, type);
    for (auto& rectangle : updated_rectangles) {
        const auto found = updates.find(rectangle.item_id);
        if (found != updates.end())
            rectangle.metadata = found->second.replacement;
    }
    for (auto& [id, update] : updates)
        update.stored->swap(update.replacement);
    rectangles(network, type).swap(updated_rectangles);
}
} // namespace dsd::application
