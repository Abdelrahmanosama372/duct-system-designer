#include "autocad/ui/model/NetworkActions.h"
#include "autocad/ui/model/NetworkSession.h"

namespace {
NetworkActions::Handler handler = nullptr;
}
void NetworkActions::SetHandler(Handler value) { handler = value; }
bool NetworkActions::Request(Action action, HWND notifyWindow) {
    if (handler)
        return handler(action, notifyWindow);
    NetworkSession::Current().message = L"Drawing selection requires AutoCAD.";
    return false;
}
