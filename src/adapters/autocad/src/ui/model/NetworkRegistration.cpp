#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/ApplicationSession.h"
#include <stdexcept>

namespace {
CString wide(const std::string& text) {
    return CString(std::wstring(text.begin(), text.end()).c_str());
}
void summarize(NetworkDraft& form, const dsd::application::DuctNetwork& network) {
    form.segmentCount.Format(L"%zu", network.segmented_lines_infos.size());
    form.totalFlow.Format(L"%.12g CFM", dsd::application::network_total_flow(network));
}
} // namespace
void NetworkSession::AnalyzeCurrent() {
    auto& session = Current();
    if (session.busy || !session.network)
        throw std::invalid_argument("Select a root and finish drawing selection before analysis");
    session.analysis = DuctUi::Application().analyze_network(*session.network);
    summarize(session.form, *session.network);
    if (session.analysis->ok()) {
        session.form.status = L"Analyzed";
        session.message = L"Analysis passed. The network can now be registered.";
    } else {
        session.form.status = L"Needs analysis";
        session.message = L"Analysis failed: ";
        for (const auto& issue : session.analysis->issues)
            session.message += wide(issue.item_id) + L": " + wide(issue.message) + L". ";
    }
    Notify();
}
void NetworkSession::RegisterCurrent() {
    auto& session = Current();
    if (!CanRegister())
        throw std::invalid_argument("Analyze the current draft successfully before registering");
    auto form = session.form;
    summarize(form, *session.network);
    form.status = L"Registered";
    auto records = Drafts();
    const auto existing = std::find_if(records.begin(), records.end(),
                                       [&](const auto& record) { return record.id == form.id; });
    if (existing == records.end())
        records.push_back(form);
    else
        *existing = form;
    // Revalidate in the application before accepting the complete snapshot.
    DuctUi::Application().register_network(*session.network);
    Drafts().swap(records);
    ResetDraft();
    session.message = L"Network registered. Select a root to start another network.";
    Notify();
}
void NetworkSession::EditRegistered(size_t index) {
    if (Current().busy)
        throw std::invalid_argument("Finish drawing selection before modifying a network");
    const auto form = Drafts().at(index);
    const auto id = dsd::application::network_item_id("NET", static_cast<std::size_t>(form.id));
    auto copy = std::make_unique<dsd::application::DuctNetwork>(
        DuctUi::Application().registered_network(id));
    ResetDraft();
    auto& session = Current();
    session.form = form;
    session.network = std::move(copy);
    session.editing_network_id = id;
    InvalidateAnalysis();
    session.message =
        L"Modifying a copy of the registered network. Analyze before registering changes.";
    Notify();
}
