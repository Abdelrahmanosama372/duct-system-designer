#pragma once
#include "autocad/ui/platform/Mfc.h"
#include "application/duct_annotations.hpp"
#include <string>
#include <vector>

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
    RemoveGeometry,
    GenerateAnnotations,
    RemoveAnnotations,
    SetVisualizationVisibility
};
enum class GeometryRepresentation { Outline, OutlineWithCenterlines };
struct GeometryRequest {
    std::string network_id;
    GeometryRepresentation representation = GeometryRepresentation::Outline;
    double reducer_length_mm = 300;
    double takeoff_length_mm = 150;
    double elbow_inside_radius_ratio = 1.5;
    std::wstring duct_layer = L"M-HVAC-DUCT", centerline_layer = L"M-HVAC-CENTERLINE";
    bool remove = false;
};
struct AnnotationRequest {
    std::string network_id;
    dsd::application::AnnotationSettings settings;
    bool remove = false;
};
struct VisibilityRequest {
    std::string network_id;
    bool visible = true;
};
using VisibilityHandler = bool (*)(const VisibilityRequest&, HWND);
void SetVisibilityHandler(VisibilityHandler handler);
bool RequestVisibility(const VisibilityRequest& request, HWND notifyWindow);
using LayerProvider = std::vector<std::wstring> (*)();
using AnnotationHandler = bool (*)(const AnnotationRequest&, HWND);
void SetAnnotationHandler(AnnotationHandler handler);
bool RequestAnnotations(const AnnotationRequest& request, HWND notifyWindow);
void SetTextStyleProvider(LayerProvider provider);
std::vector<std::wstring> DrawingTextStyles();
using GeometryHandler = bool (*)(const GeometryRequest&, HWND);
void SetLayerProvider(LayerProvider provider);
std::vector<std::wstring> DrawingLayers();
void SetGeometryHandler(GeometryHandler handler);
bool RequestGeometry(const GeometryRequest& request, HWND notifyWindow);
using Handler = bool (*)(Action, HWND);
void SetHandler(Handler handler);
bool Request(Action action, HWND notifyWindow);
bool RefreshLabels(HWND notifyWindow = nullptr);
bool FinishPropertyApply();
} // namespace NetworkActions
