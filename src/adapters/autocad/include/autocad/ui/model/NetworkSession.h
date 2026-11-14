#pragma once
#include "autocad/ui/platform/Mfc.h"

struct NetworkDraft
{
    CString name, system;
    CString minimum=L"100", maximum=L"2000", increment=L"25", aspect=L"2.0";
    CString sizingMethod=L"Equal Friction",headLoss=L"0.1",velocity=L"1000";
    CString material=L"Galvanized Steel",roughness=L"0.15";
    bool segments = true, flow = false, bod = false, dimensions = false;
    CString segmentCount = L"--", totalFlow = L"--", status = L"Configured";
    int id = 0;
};
// Shared session data; no drawing writes or persistence in the UI prototype.
namespace NetworkSession
{
    std::vector<NetworkDraft>& Drafts();
    int& NextId();
    void InitializeSamples();
    void Reset();
}
