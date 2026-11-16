#include "autocad/ui/model/PropertiesSession.h"
#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include <stdexcept>
#include <set>

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
    std::set<std::string> seen;
    std::erase_if(selection.item_ids, [&](const auto& id) { return !seen.insert(id).second; });
    if (selection.item_ids.empty()) {
        Clear();
        return;
    }
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
        dsd::application::Metadata first_values;
        for (std::size_t i = 0; i < selected->item_ids.size(); ++i) {
            auto values = dsd::application::read_network_item_metadata(*network, selected->type,
                                                                       selected->item_ids[i]);
            if (i == 0)
                first_values = std::move(values);
        }
        return Snapshot{*selected, network->name, std::move(first_values)};
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}
void PropertiesSession::Apply(const Selection& expected,
                              const dsd::application::Metadata& changes) {
    const auto target = expected;
    auto& session = NetworkSession::Current();
    if (session.busy || !selected || !session.network ||
        selected->network_id != target.network_id || selected->item_ids != target.item_ids ||
        selected->type != target.type || session.network->id != target.network_id)
        throw std::invalid_argument("The selection changed. Select a current visualization again");
    dsd::application::update_network_items_metadata(*session.network, target.type, target.item_ids,
                                                    changes);
    NetworkSession::InvalidateAnalysis();
    Clear();
    NetworkActions::FinishPropertyApply();
}
