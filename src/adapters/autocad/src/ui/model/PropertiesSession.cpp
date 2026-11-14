#include "autocad/ui/model/PropertiesSession.h"
#include "autocad/ui/model/NetworkSession.h"
#include <stdexcept>

namespace {
std::optional<PropertiesSession::Selection> selected;
std::vector<HWND> listeners;
void notify() {
    // Synchronous: a selection change must discard pending edits before Apply can run.
    const auto windows = listeners;
    for (const auto window : windows)
        if (::IsWindow(window))
            ::SendMessage(window, WM_DSD_PROPERTIES_UPDATED, 0, 0);
}
} // namespace
void PropertiesSession::Attach(HWND window) { listeners.push_back(window); }
void PropertiesSession::Detach(HWND window) { std::erase(listeners, window); }
void PropertiesSession::Select(Selection selection) {
    selected = std::move(selection);
    notify();
}
void PropertiesSession::Clear() {
    selected.reset();
    notify();
}
std::optional<PropertiesSession::Snapshot> PropertiesSession::Read() {
    const auto* network = NetworkSession::Current().network.get();
    if (!selected || !network || network->id != selected->network_id)
        return std::nullopt;
    try {
        return Snapshot{*selected, network->name,
                        dsd::application::read_network_item_metadata(*network, selected->type,
                                                                     selected->item_id)};
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}
void PropertiesSession::Apply(const Selection& expected,
                              const dsd::application::Metadata& changes) {
    const auto target = expected;
    auto& session = NetworkSession::Current();
    if (session.busy || !selected || !session.network ||
        selected->network_id != target.network_id || selected->item_id != target.item_id ||
        selected->type != target.type || session.network->id != target.network_id)
        throw std::invalid_argument("The selection changed. Select a current visualization again");
    dsd::application::update_network_item_metadata(*session.network, target.type, target.item_id,
                                                   changes);
    NetworkSession::InvalidateAnalysis();
    notify();
}
