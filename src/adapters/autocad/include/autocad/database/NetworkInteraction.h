#pragma once
#include "dbid.h"
#include <optional>
#include <string>
#include "application/network_persistence.hpp"
class AcDbDatabase;

namespace Autocad::Network {
struct VisualizationIdentity {
    std::wstring network_id, visualization_id, item_id, type;
};
void Initialize();
void Shutdown();
void ExecutePendingAction();
void IdentifyVisualization();
void RebindPersistentNetwork(AcDbDatabase& database,
    dsd::application::persistence::NetworkRecord& record);
std::optional<VisualizationIdentity> ResolveVisualization(AcDbObjectId entity);
// Only entities belonging to the currently owned draft may be edited.
std::optional<VisualizationIdentity> ResolveCurrentVisualization(AcDbObjectId entity);
} // namespace Autocad::Network
