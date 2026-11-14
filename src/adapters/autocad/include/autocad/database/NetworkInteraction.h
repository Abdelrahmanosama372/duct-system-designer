#pragma once
#include "dbid.h"
#include <optional>
#include <string>

namespace Autocad::Network {
struct VisualizationIdentity {
    std::wstring network_id, visualization_id, item_id, type;
};
void Initialize();
void Shutdown();
void ExecutePendingAction();
void IdentifyVisualization();
std::optional<VisualizationIdentity> ResolveVisualization(AcDbObjectId entity);
} // namespace Autocad::Network
