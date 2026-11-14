#pragma once
#include "application/duct_network.hpp"
#include "autocad/ui/platform/Mfc.h"

struct NetworkDraft {
    CString name, system = L"Supply Air";
    CString minimum = L"100", maximum = L"2000", increment = L"25", aspect = L"2.0";
    CString sizingMethod = L"Equal Friction", headLoss = L"0.1", velocity = L"1000";
    CString material = L"Galvanized Steel", roughness = L"0.15";
    bool segments = true, flow = false, bod = false, dimensions = false;
    CString segmentCount = L"--", totalFlow = L"--", status = L"Configured";
    int id = 0;
};
struct NetworkDraftSession {
    NetworkDraft form;
    std::unique_ptr<dsd::application::DuctNetwork> network;
    bool busy = false;
    CString message = L"Select a root line or straight polyline to discover a network.";
};
namespace NetworkSession {
std::vector<NetworkDraft>& Drafts();
int& NextId();
NetworkDraftSession& Current();
void UpdateForm(const NetworkDraft& form);
void ResetDraft();
void Reset();
} // namespace NetworkSession
