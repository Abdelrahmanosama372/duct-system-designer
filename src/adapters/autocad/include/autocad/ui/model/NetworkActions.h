#pragma once
#include "autocad/ui/platform/Mfc.h"
#include <string>

inline constexpr UINT WM_DSD_NETWORK_UPDATED = WM_APP + 84;
namespace NetworkActions {
enum class Action {
    PickRoot,
    PickAirTerminal,
    PickDamper,
    Refresh,
    RefreshLabels,
    FinishPropertyApply,
    GenerateGeometry,
    RemoveGeometry
};
enum class GeometryRepresentation { Hatch, Outline, Centerline };
struct GeometryRequest {
    std::string network_id;
    GeometryRepresentation representation = GeometryRepresentation::Outline;
    double reducer_length_mm = 300;
    double takeoff_length_mm = 150;
    double elbow_inside_radius_ratio = 1.5;
    std::wstring duct_layer = L"M-HVAC-DUCT", fitting_layer = L"M-HVAC-FITTINGS";
    bool remove = false;
};
using GeometryHandler = bool (*)(const GeometryRequest&, HWND);
void SetGeometryHandler(GeometryHandler handler);
bool RequestGeometry(const GeometryRequest& request, HWND notifyWindow);
using Handler = bool (*)(Action, HWND);
void SetHandler(Handler handler);
bool Request(Action action, HWND notifyWindow);
bool RefreshLabels(HWND notifyWindow = nullptr);
bool FinishPropertyApply();
} // namespace NetworkActions
