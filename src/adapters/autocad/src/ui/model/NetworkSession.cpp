#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/PropertiesSession.h"
#include "autocad/ui/model/ApplicationSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include <tuple>

namespace {
std::vector<HWND> listeners;
auto settings(const NetworkDraft& form) {
    return std::tie(form.name, form.system, form.minimum, form.maximum, form.increment, form.aspect,
                    form.sizingMethod, form.headLoss, form.velocity, form.material, form.roughness,
                    form.segments, form.flow, form.bod, form.dimensions);
}
} // namespace
std::vector<NetworkDraft>& NetworkSession::Drafts() { return DuctUi::Session().networks; }
int& NetworkSession::NextId() { return DuctUi::Session().next_id; }
NetworkDraftSession& NetworkSession::Current() { return DuctUi::Session().draft; }
void NetworkSession::Attach(HWND window) { listeners.push_back(window); }
void NetworkSession::Detach(HWND window) { std::erase(listeners, window); }
void NetworkSession::Notify(bool drawing_changed) {
    const auto windows = listeners;
    for (const auto window : windows)
        if (::IsWindow(window))
            ::SendMessage(window, WM_DSD_NETWORK_UPDATED, drawing_changed?1:0, 0);
}
void NetworkSession::InvalidateAnalysis() {
    Current().analysis.reset();
    Current().form.status = L"Draft";
    Current().message = L"The draft changed. Analyze the network before registering.";
    Notify();
}
bool NetworkSession::CanRegister() {
    return !Current().busy && Current().network && Current().analysis && Current().analysis->ok();
}
void NetworkSession::ResetDraft() {
    Current() = NetworkDraftSession{};
    PropertiesSession::Clear();
    Notify();
}
void NetworkSession::Reset() {
    Drafts().clear();
    DuctUi::Application().clear_networks();
    ResetDraft();
    NextId() = 1;
}
void NetworkSession::UpdateForm(const NetworkDraft& form) {
    auto updated = form;
    if (Current().editing_network_id)
        updated.name = Current().form.name;
    const bool changed = settings(Current().form) != settings(updated);
    Current().form = std::move(updated);
    if (changed)
        InvalidateAnalysis();
    if (!Current().network)
        return;
    auto& network = *Current().network;
    const auto& saved = Current().form;
    network.name = {saved.name.GetString(), static_cast<std::size_t>(saved.name.GetLength())};
    using namespace dsd::application;
    network.system_type = saved.system == L"Return Air"    ? NetworkSystemType::Return
                          : saved.system == L"Exhaust Air" ? NetworkSystemType::Exhaust
                                                           : NetworkSystemType::Supply;
    network.material = saved.material == L"Stainless Steel" ? NetworkMaterial::StainlessSteel
                       : saved.material == L"Aluminum"      ? NetworkMaterial::Aluminum
                       : saved.material == L"Carbon Steel"  ? NetworkMaterial::CarbonSteel
                                                            : NetworkMaterial::GalvanizedSteel;
}
