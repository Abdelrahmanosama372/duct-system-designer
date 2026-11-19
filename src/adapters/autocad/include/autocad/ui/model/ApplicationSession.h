#pragma once
#include "application/application.hpp"
#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/ProjectInformation.h"

namespace DuctUi {
struct DrawingSession {
    dsd::application::DsdApplication application;
    std::vector<NetworkDraft> networks;
    NetworkDraftSession draft;
    int next_id = 1;
    std::array<CString,ProjectInfo::Count> project_draft{}, project_saved{};
    bool project_editing = true;
};
using DrawingKeyProvider = void* (*)();
void SetDrawingKeyProvider(DrawingKeyProvider provider);
DrawingSession& Session();
void ReleaseDrawingSession(void* key);
void ClearDrawingSessions();
dsd::application::DsdApplication& Application();
}
