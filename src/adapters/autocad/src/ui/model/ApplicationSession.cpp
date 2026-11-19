#include "autocad/ui/model/ApplicationSession.h"
#include <map>

namespace {
DuctUi::DrawingKeyProvider keyProvider = nullptr;
std::map<void *, std::unique_ptr<DuctUi::DrawingSession>> sessions;
} // namespace
void DuctUi::SetDrawingKeyProvider(DrawingKeyProvider provider) { keyProvider = provider; }
DuctUi::DrawingSession &DuctUi::Session() {
    void *key = keyProvider ? keyProvider() : nullptr;
    auto &session = sessions[key];
    if (!session)
        session = std::make_unique<DrawingSession>();
    return *session;
}
dsd::application::DsdApplication &DuctUi::Application() { return Session().application; }
void DuctUi::ReleaseDrawingSession(void *key) { sessions.erase(key); }
void DuctUi::ClearDrawingSessions() { sessions.clear(); }
