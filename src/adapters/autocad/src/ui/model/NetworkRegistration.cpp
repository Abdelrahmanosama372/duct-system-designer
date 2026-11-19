#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include "autocad/ui/model/ApplicationSession.h"
#include <stdexcept>
#include <cmath>
#include <limits>

namespace {
CString wide(const std::string& text) {
    return CString(std::wstring(text.begin(), text.end()).c_str());
}
double number(const CString& text, const char* label) {
    wchar_t* end = nullptr;
    const double value = std::wcstod(text.GetString(), &end);
    if (end == text.GetString() || *end || !std::isfinite(value))
        throw std::invalid_argument(std::string("Invalid sizing setting: ") + label);
    return value;
}
int integer(const CString& text, const char* label) {
    const auto value = number(text, label);
    if (value < 1 || value > (std::numeric_limits<int>::max)() || std::floor(value) != value)
        throw std::invalid_argument(std::string("Expected positive integer: ") + label);
    return static_cast<int>(value);
}
dsd::application::NetworkSizingConfiguration sizing_config(const NetworkDraft& form) {
    using namespace dsd::application;
    NetworkSizingConfiguration config;
    config.minimum_mm = integer(form.minimum, "minimum");
    config.maximum_mm = integer(form.maximum, "maximum");
    config.increment_mm = integer(form.increment, "increment");
    config.aspect_ratio = number(form.aspect, "aspect ratio");
    config.roughness_mm = number(form.roughness, "roughness");
    config.method = form.sizingMethod == L"Velocity" ? NetworkSizingMethod::EqualVelocity
                                                         : NetworkSizingMethod::EqualFriction;
    if (config.method == NetworkSizingMethod::EqualVelocity)
        config.velocity_fpm = number(form.velocity, "velocity");
    else config.head_loss_in_wg_per_100ft = number(form.headLoss, "head loss");
    (void)config.to_sizer_config();
    return config;
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
    auto network = *session.network;
    network.sizing = sizing_config(form);
    NetworkActions::CommitDrawingChange([&] {
        DuctUi::Application().register_network(std::move(network));
        Drafts().swap(records);
    });
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
