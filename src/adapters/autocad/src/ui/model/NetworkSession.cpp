#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/PropertiesSession.h"
#include "autocad/ui/model/ApplicationSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include <tuple>

namespace {
std::vector<NetworkDraft> registeredNetworks;
NetworkDraftSession current;
int nextId = 1;
std::vector<HWND> listeners;
auto settings(const NetworkDraft& form) {
    return std::tie(form.name, form.system, form.minimum, form.maximum, form.increment, form.aspect,
                    form.sizingMethod, form.headLoss, form.velocity, form.material, form.roughness,
                    form.segments, form.flow, form.bod, form.dimensions);
}
} // namespace
std::vector<NetworkDraft>& NetworkSession::Drafts() { return registeredNetworks; }
int& NetworkSession::NextId() { return nextId; }
NetworkDraftSession& NetworkSession::Current() { return current; }
void NetworkSession::Attach(HWND window) { listeners.push_back(window); }
void NetworkSession::Detach(HWND window) { std::erase(listeners, window); }
void NetworkSession::Notify() {
    const auto windows = listeners;
    for (const auto window : windows)
        if (::IsWindow(window))
            ::SendMessage(window, WM_DSD_NETWORK_UPDATED, 0, 0);
}
void NetworkSession::InvalidateAnalysis() {
    current.analysis.reset();
    current.form.status = L"Draft";
    current.message = L"The draft changed. Analyze the network before registering.";
    Notify();
}
bool NetworkSession::CanRegister() {
    return !current.busy && current.network && current.analysis && current.analysis->ok();
}
void NetworkSession::ResetDraft() {
    current = NetworkDraftSession{};
    PropertiesSession::Clear();
    Notify();
}
void NetworkSession::Reset() {
    registeredNetworks.clear();
    DuctUi::Application().clear_networks();
    ResetDraft();
    nextId = 1;
}
void NetworkSession::UpdateForm(const NetworkDraft& form) {
    auto updated = form;
    if (current.editing_network_id)
        updated.name = current.form.name;
    const bool changed = settings(current.form) != settings(updated);
    current.form = std::move(updated);
    if (changed)
        InvalidateAnalysis();
    if (!current.network)
        return;
    auto& network = *current.network;
    const auto& saved = current.form;
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
