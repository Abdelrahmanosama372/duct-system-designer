#pragma once
#include "application/network_properties.hpp"
#include "autocad/ui/platform/Mfc.h"

inline constexpr UINT WM_DSD_PROPERTIES_UPDATED = WM_APP + 85;
namespace PropertiesSession {
struct Selection {
    std::string network_id;
    std::vector<std::string> item_ids; // First item supplies the displayed values.
    dsd::application::NetworkItemType type;
};
struct Snapshot {
    Selection selection;
    std::wstring network_name;
    dsd::application::Metadata metadata;
};
void Attach(HWND window);
void Detach(HWND window);
void Select(Selection selection);
void Clear();
std::optional<Snapshot> Read();
void Apply(const Selection& expected, const dsd::application::Metadata& changes);
} // namespace PropertiesSession
