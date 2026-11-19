#include "autocad/database/DrawingPersistence.h"
#include "PersistenceDatabase.h"
#include "acdocman.h"
#include "autocad/database/GeneratedGeometry.h"
#include "autocad/database/NetworkInteraction.h"
#include "autocad/database/NodNetworkRepository.h"
#include "autocad/database/PersistentDrawingEntities.h"
#include "autocad/ui/model/ApplicationSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include "autocad/ui/model/PropertiesSession.h"
#include <algorithm>
#include <cstdint>
#include <set>

namespace Autocad::Persistence {
using namespace detail;
using namespace dsd::application;
using namespace dsd::application::persistence;
namespace {
struct DrawingState {
    std::map<std::string, NetworkRecord> records;
    bool loaded = false, writable = true, recovery_queued = false;
};
std::map<AcDbDatabase *, DrawingState> drawings;
NodNetworkRepository repository;
PersistentDrawingEntities entities;
NetworkRecoveryService recovery;
bool updating = false;
void *current_key() {
    auto *doc = acDocManager->mdiActiveDocument();
    return doc ? doc->database() : nullptr;
}
struct Updating {
    Updating() {
        if (updating)
            throw std::runtime_error("Drawing persistence is already updating");
        updating = true;
    }
    ~Updating() { updating = false; }
};
struct DocumentLock {
    AcApDocument *document;
    bool locked = false;
    explicit DocumentLock(AcApDocument *doc) : document(doc) {
        if (!doc)
            throw std::runtime_error("Open a drawing before saving network data");
        if (doc->myLockMode() == AcAp::kNone) {
            check(acDocManager->lockDocument(doc, AcAp::kWrite, L"DSD_DATA_CHANGE",
                                             L"DSD data change", false),
                  "Could not lock drawing for persistence");
            locked = true;
        }
    }
    ~DocumentLock() {
        if (locked)
            acDocManager->unlockDocument(document);
    }
};
CString number(double value) {
    CString out;
    out.Format(L"%.12g", value);
    return out;
}
NetworkDraft form(const NetworkRecord &record) {
    const auto &r = record.registered;
    const auto &n = r.network;
    NetworkDraft out;
    if (!n.id.starts_with("NET-"))
        throw std::invalid_argument("Unknown saved network ID format");
    out.id = std::stoi(n.id.substr(4));
    if (out.id <= 0)
        throw std::invalid_argument("Invalid saved network ID");
    out.name = n.name.c_str();
    out.system = n.system_type == NetworkSystemType::Supply   ? L"Supply Air"
                 : n.system_type == NetworkSystemType::Return ? L"Return Air"
                                                              : L"Exhaust Air";
    out.material = n.material == NetworkMaterial::GalvanizedSteel  ? L"Galvanized Steel"
                   : n.material == NetworkMaterial::StainlessSteel ? L"Stainless Steel"
                   : n.material == NetworkMaterial::Aluminum       ? L"Aluminum"
                                                                   : L"Carbon Steel";
    out.minimum = number(n.sizing.minimum_mm);
    out.maximum = number(n.sizing.maximum_mm);
    out.increment = number(n.sizing.increment_mm);
    out.aspect = number(n.sizing.aspect_ratio);
    out.sizingMethod =
        n.sizing.method == NetworkSizingMethod::EqualFriction ? L"Equal Friction" : L"Velocity";
    out.headLoss = number(n.sizing.head_loss_in_wg_per_100ft);
    out.velocity = number(n.sizing.velocity_fpm);
    out.roughness = number(n.sizing.roughness_mm);
    out.segments = record.show_segments;
    out.flow = record.visualization_fields.flow;
    out.dimensions = record.visualization_fields.dimensions;
    out.bod = record.visualization_fields.bod;
    out.visualizationVisible = record.visualization_visible;
    out.segmentCount.Format(L"%zu", n.segmented_lines_infos.size());
    out.totalFlow.Format(L"%.12g CFM", network_total_flow(n));
    out.status = !r.consistent() ? L"Inconsistent" : r.generation ? L"Generated" : L"Registered";
    return out;
}
void classify(AcDbDatabase &db, NetworkRecord &record) {
    const auto space = resolve(db, record.space_handle);
    const auto plan = recovery.plan(
        record,
        [&](const auto &handle, bool block) {
            return !space.isNull() && entities.SourceExists(db, space, handle, block);
        },
        [&](const auto &definition) {
            return !space.isNull() && entities.OutputExists(db, space, definition);
        });
    record.registered.missing_source_handles = plan.missing_sources;
    if (space.isNull())
        record.registered.missing_source_handles.push_back("space:" + record.space_handle);
}
void publish(AcDbDatabase &db, DrawingState &state, bool reload_project) {
    std::vector<RegisteredNetwork> registered;
    std::vector<NetworkDraft> forms;
    for (auto &[id, record] : state.records) {
        (void)id;
        Network::RebindPersistentNetwork(db, record);
        forms.push_back(form(record));
        registered.push_back(record.registered);
        DuctUi::Session().next_id = (std::max)(DuctUi::Session().next_id, forms.back().id + 1);
    }
    DuctUi::Application().restore_networks(std::move(registered));
    NetworkSession::Drafts().swap(forms);
    if (reload_project)
        ProjectInfo::ReloadFromApplication();
    PropertiesSession::Clear();
    NetworkSession::Notify(true);
}
void queue_recovery(AcApDocument *document, DrawingState &state) {
    if (state.recovery_queued)
        return;
    for (const auto &[id, record] : state.records) {
        (void)id;
        if (!record.registered.consistent())
            continue;
        const auto space = resolve(*document->database(), record.space_handle);
        const auto plan = recovery.plan(
            record,
            [&](const auto &h, bool b) {
                return entities.SourceExists(*document->database(), space, h, b);
            },
            [&](const auto &d) { return entities.OutputExists(*document->database(), space, d); });
        if (!plan.missing_entities.empty() ||
            entities.SymbolsMissing(*document->database(), record)) {
            if (acDocManager->sendStringToExecute(document, L"_DSD_RECOVER_NETWORKS ", true, false,
                                                  false) == Acad::eOk)
                state.recovery_queued = true;
            break;
        }
    }
}
void load(AcApDocument *document, bool force, bool recover_missing) {
    if (!document || document != acDocManager->mdiActiveDocument() || updating)
        return;
    Updating guard;
    auto &db = *document->database();
    auto &state = drawings[&db];
    if (!force && state.loaded) {
        for (auto &[id, record] : state.records) {
            record.registered = DuctUi::Application().registered_network_record(id);
            classify(db, record);
        }
        publish(db, state, false);
        return;
    }
    const auto stored = repository.Read(db);
    DrawingState restored;
    restored.loaded = true;
    restored.writable = stored.writable;
    DuctUi::Session().next_id = (std::max)(1, stored.header.next_network_id);
    if (stored.header.project)
        DuctUi::Application().register_project_information(*stored.header.project);
    else
        DuctUi::Application().clear_project_information();
    for (auto record : stored.networks) {
        try {
            classify(db, record);
            (void)form(record);
            restored.records.emplace(record.registered.network.id, std::move(record));
        } catch (const std::exception &error) {
            acutPrintf(L"\nDSD network could not be loaded: %hs", error.what());
        }
    }
    state = std::move(restored);
    publish(db, state, true);
    if (!stored.issues.empty()) {
        NetworkSession::Current().message =
            L"Some persistent data could not be read; see the command line.";
        for (const auto &issue : stored.issues)
            acutPrintf(L"\nDSD persistence: %hs", issue.c_str());
        NetworkSession::Notify();
    }
    if (recover_missing)
        queue_recovery(document, state);
}
void capture(AcDbDatabase &db, DrawingState &state) {
    std::map<std::string, NetworkRecord> records;
    for (const auto &[id, registered] : DuctUi::Application().registered_networks()) {
        NetworkRecord record;
        if (auto old = state.records.find(id); old != state.records.end())
            record = old->second;
        const bool registration_changed = record.registered.network.id.empty() ||
                                          record.registered.revision != registered.revision;
        if (registration_changed)
            std::erase_if(record.entities,
                          [](const auto &e) { return e.group == OutputGroup::Visualization; });
        record.registered = registered;
        if (record.space_handle.empty()) {
            const auto separator = registered.network.document_key.rfind(':');
            if (separator == std::string::npos)
                throw std::runtime_error("Registered network has no drawing-space reference");
            record.space_handle = registered.network.document_key.substr(separator + 1);
        }
        const auto space = resolve(db, record.space_handle);
        if (space.isNull())
            throw std::runtime_error("Network drawing space is missing");
        const auto ui =
            std::find_if(NetworkSession::Drafts().begin(), NetworkSession::Drafts().end(),
                         [&](const auto &f) { return network_item_id("NET", f.id) == id; });
        if (ui != NetworkSession::Drafts().end()) {
            record.visualization_fields = {ui->flow, ui->dimensions, ui->bod};
            record.visualization_visible = ui->visualizationVisible;
            record.show_segments = ui->segments;
        }
        auto captured = entities.Capture(db, space, id, record.entities);
        if (!registration_changed && NetworkSession::Current().editing_network_id == id)
            std::erase_if(captured,
                          [](const auto &e) { return e.group == OutputGroup::Visualization; });
        std::set<std::string> handles;
        for (const auto &output : captured)
            handles.insert(output.handle);
        // Retain definitions for user-deleted entities. Intentional removal explicitly forgets its
        // group.
        for (const auto &old : record.entities)
            if (!handles.contains(old.handle))
                captured.push_back(old);
        record.entities = std::move(captured);
        classify(db, record);
        DuctUi::Application().update_source_consistency(id,
                                                        record.registered.missing_source_handles);
        if (ui != NetworkSession::Drafts().end())
            ui->status = !record.registered.consistent() ? L"Inconsistent"
                         : record.registered.generation  ? L"Generated"
                                                         : L"Registered";
        records.emplace(id, std::move(record));
    }
    std::vector<NetworkRecord> data;
    for (const auto &[id, record] : records) {
        (void)id;
        data.push_back(record);
    }
    repository.Write(db, {DuctUi::Application().project_information(), NetworkSession::NextId()},
                     data);
    state.records.swap(records);
}
void commit_change(const std::function<void()> &change) {
    auto *document = acDocManager->mdiActiveDocument();
    if (!document)
        throw std::runtime_error("Open a drawing before saving network data");
    if (!drawings[document->database()].loaded)
        load(document, false, false);
    auto &state = drawings[document->database()];
    if (!state.writable)
        throw std::runtime_error(
            "Unsupported or damaged DSD persistence header; existing data has been preserved");
    DocumentLock lock(document);
    Updating guard;
    auto old_application = DuctUi::Application();
    auto old_forms = NetworkSession::Drafts();
    auto old_state = state;
    const auto old_form = NetworkSession::Current().form;
    const int old_next_id = NetworkSession::NextId();
    const auto old_project_draft = ProjectInfo::Draft(), old_project_saved = ProjectInfo::Saved();
    const bool old_editing = ProjectInfo::Editing();
    try {
        Transaction transaction(*document->database());
        change();
        capture(*document->database(), state);
        // Binding can fail too; keep it within the transaction so rollback remains possible.
        for (auto &[id, record] : state.records) {
            (void)id;
            Network::RebindPersistentNetwork(*document->database(), record);
        }
        transaction.commit();
    } catch (...) {
        DuctUi::Application() = std::move(old_application);
        NetworkSession::Drafts() = std::move(old_forms);
        state = std::move(old_state);
        NetworkSession::Current().form = old_form;
        NetworkSession::NextId() = old_next_id;
        ProjectInfo::Draft() = old_project_draft;
        ProjectInfo::Saved() = old_project_saved;
        ProjectInfo::Editing() = old_editing;
        for (auto &[id, record] : state.records) {
            (void)id;
            Network::RebindPersistentNetwork(*document->database(), record);
        }
        throw;
    }
}
void refresh_consistency() {
    auto *doc = acDocManager->mdiActiveDocument();
    if (!doc || updating)
        return;
    auto found = drawings.find(doc->database());
    if (found == drawings.end() || !found->second.loaded)
        return;
    Updating guard;
    bool changed = false;
    for (auto &[id, record] : found->second.records) {
        const auto old = record.registered.missing_source_handles;
        record.registered = DuctUi::Application().registered_network_record(id);
        classify(*doc->database(), record);
        changed = changed || old != record.registered.missing_source_handles;
    }
    if (changed)
        publish(*doc->database(), found->second, false);
}
class Documents final : public AcApDocManagerReactor {
    void documentBecameCurrent(AcApDocument *doc) override {
        try {
            load(doc, false, true);
        } catch (const std::exception &e) {
            acutPrintf(L"\nDSD load: %hs", e.what());
        }
    }
    void documentToBeDestroyed(AcApDocument *doc) override {
        drawings.erase(doc->database());
        DuctUi::ReleaseDrawingSession(doc->database());
    }
};
Documents documents;
class Editor final : public AcEditorReactor {
    void commandEnded(const ACHAR *command) override {
        if (updating)
            return;
        try {
            CString name(command);
            name.MakeUpper();
            name.TrimLeft(L"_.");
            if (name == L"U" || name == L"UNDO" || name == L"REDO" || name == L"MREDO")
                load(acDocManager->mdiActiveDocument(), true, false);
            else
                refresh_consistency();
        } catch (const std::exception &e) {
            acutPrintf(L"\nDSD state refresh: %hs", e.what());
        }
    }
};
Editor editor;
} // namespace
void Initialize() {
    DuctUi::SetDrawingKeyProvider(current_key);
    NetworkActions::SetDrawingChangeHandler(commit_change);
    acDocManager->addReactor(&documents);
    acedEditor->addReactor(&editor);
    try {
        load(acDocManager->mdiActiveDocument(), false, true);
    } catch (const std::exception &e) {
        acutPrintf(L"\nDSD persistence load: %hs", e.what());
    }
}
void Shutdown() {
    NetworkActions::SetDrawingChangeHandler(nullptr);
    acDocManager->removeReactor(&documents);
    acedEditor->removeReactor(&editor);
    drawings.clear();
    DuctUi::SetDrawingKeyProvider(nullptr);
}
void RequireConsistentSources(const std::string &id) {
    auto *doc = acDocManager->mdiActiveDocument();
    if (!doc)
        throw std::runtime_error("Open the source drawing");
    NetworkRecord record;
    record.registered = DuctUi::Application().registered_network_record(id);
    const auto separator = record.registered.network.document_key.rfind(':');
    if (separator != std::string::npos)
        record.space_handle = record.registered.network.document_key.substr(separator + 1);
    classify(*doc->database(), record);
    const auto previous =
        DuctUi::Application().registered_network_record(id).missing_source_handles;
    DuctUi::Application().update_source_consistency(id, record.registered.missing_source_handles);
    auto &state = drawings[doc->database()];
    if (auto saved = state.records.find(id); saved != state.records.end())
        saved->second.registered.missing_source_handles = record.registered.missing_source_handles;
    for (auto &ui : NetworkSession::Drafts())
        if (network_item_id("NET", ui.id) == id)
            ui.status = !record.registered.consistent() ? L"Inconsistent"
                        : record.registered.generation  ? L"Generated"
                                                        : L"Registered";
    if (previous != record.registered.missing_source_handles)
        NetworkSession::Notify(true);
    if (!record.registered.consistent())
        throw std::invalid_argument(
            "Network is inconsistent: one or more source entities are missing");
}
void ForgetOutputs(const std::string &id, OutputGroup group) {
    auto *doc = acDocManager->mdiActiveDocument();
    if (!doc)
        return;
    auto &state = drawings[doc->database()];
    auto found = state.records.find(id);
    if (found == state.records.end())
        return;
    std::erase_if(found->second.entities, [&](const auto &e) { return e.group == group; });
}
void RecoverCurrentDrawing() try {
    auto *doc = acDocManager->mdiActiveDocument();
    if (!doc)
        return;
    if (!drawings[doc->database()].loaded)
        load(doc, false, false);
    auto &state = drawings[doc->database()];
    state.recovery_queued = false;
    if (!state.writable)
        return;
    DocumentLock lock(doc);
    Updating guard;
    for (auto &[id, record] : state.records) {
        (void)id;
        classify(*doc->database(), record);
        if (!record.registered.consistent())
            continue;
        auto candidate = record;
        try {
            const auto space = resolve(*doc->database(), record.space_handle);
            const auto plan = recovery.plan(
                record,
                [&](const auto &h, bool b) {
                    return entities.SourceExists(*doc->database(), space, h, b);
                },
                [&](const auto &d) { return entities.OutputExists(*doc->database(), space, d); });
            if (plan.missing_entities.empty() && !entities.SymbolsMissing(*doc->database(), record))
                continue;
            Transaction transaction(*doc->database());
            entities.Recover(*doc->database(), space, candidate);
            repository.Write(
                *doc->database(),
                {DuctUi::Application().project_information(), NetworkSession::NextId()},
                {candidate});
            transaction.commit();
            record = std::move(candidate);
        } catch (const std::exception &e) {
            acutPrintf(L"\nDSD recovery pending for %hs: %hs", record.registered.network.id.c_str(),
                       e.what());
        }
    }
    publish(*doc->database(), state, false);
    acedUpdateDisplay();
} catch (CException *error) {
    error->Delete();
    acutPrintf(L"\nDSD recovery could not complete; saved network data is retained.");
} catch (const std::exception &error) {
    acutPrintf(L"\nDSD recovery could not complete: %hs", error.what());
}
} // namespace Autocad::Persistence
