#pragma once
#include "application/duct_network.hpp"
#include "application/network_analyzer.hpp"
#include "autocad/ui/platform/Mfc.h"

struct NetworkDraft {
    CString name, system = L"Supply Air";
    CString minimum = L"100", maximum = L"2000", increment = L"25", aspect = L"2.0";
    CString sizingMethod = L"Equal Friction", headLoss = L"0.1", velocity = L"1000";
    CString material = L"Galvanized Steel", roughness = L"0.15";
    bool segments = true, flow = false, bod = false, dimensions = false;
    bool visualizationVisible = true;
    CString segmentCount = L"--", totalFlow = L"--", status = L"Configured";
    int id = 0;
};
struct NetworkDraftSession {
    NetworkDraft form;
    std::unique_ptr<dsd::application::DuctNetwork> network;
    std::optional<dsd::application::NetworkAnalysisResult> analysis;
    std::optional<std::string> editing_network_id;
    bool busy = false;
    CString message = L"Select a root line or straight polyline to discover a network.";
};
namespace NetworkSession {
std::vector<NetworkDraft>& Drafts();
int& NextId();
NetworkDraftSession& Current();
void UpdateForm(const NetworkDraft& form);
void Attach(HWND window);
void Detach(HWND window);
void Notify(bool drawing_changed = false);
void InvalidateAnalysis();
bool CanRegister();
void AnalyzeCurrent();
void RegisterCurrent();
void EditRegistered(size_t index);
void ResetDraft();
void Reset();
} // namespace NetworkSession
