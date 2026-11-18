#include "autocad/ui/model/NetworkActions.h"
#include "autocad/ui/model/NetworkSession.h"

namespace {
NetworkActions::Handler handler = nullptr;
NetworkActions::GeometryHandler geometryHandler = nullptr;
NetworkActions::LayerProvider layerProvider = nullptr;
}
void NetworkActions::SetHandler(Handler value) { handler = value; }
void NetworkActions::SetLayerProvider(LayerProvider value) { layerProvider = value; }
std::vector<std::wstring> NetworkActions::DrawingLayers() {
    return layerProvider ? layerProvider() : std::vector<std::wstring>{};
}
void NetworkActions::SetGeometryHandler(GeometryHandler value) { geometryHandler = value; }
bool NetworkActions::RequestGeometry(const GeometryRequest& request, HWND window) {
    if (geometryHandler) return geometryHandler(request, window);
    NetworkSession::Current().message = L"Geometry drawing requires AutoCAD.";
    return false;
}
bool NetworkActions::RefreshLabels(HWND notifyWindow) {
    return handler && handler(Action::RefreshLabels, notifyWindow);
}
bool NetworkActions::FinishPropertyApply() {
    return handler && handler(Action::FinishPropertyApply, nullptr);
}
bool NetworkActions::Request(Action action, HWND notifyWindow) {
    if (handler)
        return handler(action, notifyWindow);
    NetworkSession::Current().message = L"Drawing selection requires AutoCAD.";
    return false;
}
