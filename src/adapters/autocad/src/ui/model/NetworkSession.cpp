#include "autocad/ui/model/NetworkSession.h"

namespace {
std::vector<NetworkDraft> registeredNetworks;
NetworkDraftSession current;
int nextId = 1;
} // namespace
std::vector<NetworkDraft>& NetworkSession::Drafts() { return registeredNetworks; }
int& NetworkSession::NextId() { return nextId; }
NetworkDraftSession& NetworkSession::Current() { return current; }
void NetworkSession::ResetDraft() { current = NetworkDraftSession{}; }
void NetworkSession::Reset() {
    registeredNetworks.clear();
    ResetDraft();
    nextId = 1;
}
void NetworkSession::UpdateForm(const NetworkDraft& form) {
    current.form = form;
    if (!current.network)
        return;
    auto& network = *current.network;
    network.name = {form.name.GetString(), static_cast<std::size_t>(form.name.GetLength())};
    using namespace dsd::application;
    network.system_type = form.system == L"Return Air"    ? NetworkSystemType::Return
                          : form.system == L"Exhaust Air" ? NetworkSystemType::Exhaust
                                                          : NetworkSystemType::Supply;
    network.material = form.material == L"Stainless Steel" ? NetworkMaterial::StainlessSteel
                       : form.material == L"Aluminum"      ? NetworkMaterial::Aluminum
                       : form.material == L"Carbon Steel"  ? NetworkMaterial::CarbonSteel
                                                           : NetworkMaterial::GalvanizedSteel;
}
