#pragma once
#include "autocad/ui/platform/Mfc.h"

inline constexpr UINT WM_DSD_NETWORK_UPDATED = WM_APP + 84;
namespace NetworkActions {
enum class Action {
    PickRoot,
    PickAirTerminal,
    PickDamper,
    Refresh,
    RefreshLabels,
    FinishPropertyApply
};
using Handler = bool (*)(Action, HWND);
void SetHandler(Handler handler);
bool Request(Action action, HWND notifyWindow);
bool RefreshLabels(HWND notifyWindow = nullptr);
bool FinishPropertyApply();
} // namespace NetworkActions
