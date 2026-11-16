#include "autocad/ui/platform/ObjectArx.h"
#include "autocad/database/VisualizationSelection.h"
#include "autocad/database/NetworkInteraction.h"
#include "autocad/ui/Designer.h"
#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/PropertiesSession.h"
#include "acdocman.h"
#include "adscodes.h"
#include <stdexcept>

namespace {
constexpr UINT selectionChanged = WM_APP + 86;
constexpr UINT propertyApplyCompleted = WM_APP + 87;
struct ImpliedSelection {
    ads_name entities;
    bool acquired = false;
    bool Read() {
        acquired = acedSSGet(L"_I", nullptr, nullptr, nullptr, entities) == RTNORM;
        return acquired;
    }
    void Release() {
        if (acquired)
            acedSSFree(entities);
        acquired = false;
    }
    ~ImpliedSelection() { Release(); }
};
class SelectionWindow final : public CWnd {
    bool queued = false;
    AcApDocument* completionDocument = nullptr;

  public:
    void Schedule() {
        if (m_hWnd && !queued) {
            queued = true;
            PostMessage(selectionChanged);
        }
    }
    void CompleteApply() {
        completionDocument = acDocManager->mdiActiveDocument();
        ResumeCompletion();
    }
    void ResumeCompletion() {
        if (m_hWnd && completionDocument)
            PostMessage(propertyApplyCompleted);
    }

  protected:
    afx_msg LRESULT OnPropertyApplyCompleted(WPARAM, LPARAM) {
        auto* document = completionDocument;
        auto& session = NetworkSession::Current();
        if (!document || document != acDocManager->mdiActiveDocument() || session.busy) {
            completionDocument = nullptr;
            return 0;
        }
        resbuf active{};
        if (acedGetVar(L"CMDACTIVE", &active) != RTNORM) {
            completionDocument = nullptr;
            return 0;
        }
        // A nested command can keep AutoCAD active after this message is posted.
        // Its completion notification will schedule another attempt at idle.
        if (active.resval.rint != 0)
            return 0;
        completionDocument = nullptr;
        // This message runs after the queued drawing command, when clearing the
        // pickfirst set is permitted. Temporarily allow the palette to release focus.
        session.busy = true;
        try {
            if (acedSSSetFirst(nullptr, nullptr) != RTNORM)
                acutPrintf(L"\nCould not clear the drawing selection after Apply.");
            PropertiesSession::Clear();
            auto* view = acedGetAcadDwgView();
            if (view && ::IsWindow(view->GetSafeHwnd()))
                view->SetFocus();
        } catch (CException* error) {
            error->Delete();
        } catch (const std::exception& error) {
            acutPrintf(L"\n%hs", error.what());
        }
        session.busy = false;
        NetworkSession::Notify();
        return 0;
    }
    afx_msg LRESULT OnSelectionChanged(WPARAM, LPARAM) {
        queued = false;
        // The reactor only schedules work. Selection reading and UI navigation
        // happen after AutoCAD has finished modifying its selection set.
        if (NetworkSession::Current().busy)
            return 0;
        try {
            ImpliedSelection selection;
            if (!selection.Read()) {
                PropertiesSession::Clear();
                return 0;
            }
            Adesk::Int32 count = 0;
            std::vector<AcDbObjectId> ids;
            if (acedSSLength(selection.entities, &count) == RTNORM) {
                for (Adesk::Int32 index = 0; index < count; ++index) {
                    ads_name entity;
                    AcDbObjectId id;
                    if (acedSSName(selection.entities, index, entity) == RTNORM &&
                        acdbGetObjectId(id, entity) == Acad::eOk)
                        ids.push_back(id);
                }
            }
            selection.Release();
            if (ids.empty() || ids.size() != static_cast<std::size_t>(count)) {
                PropertiesSession::Clear();
                return 0;
            }
            const auto identity = Autocad::Network::ResolveCurrentVisualization(ids.front());
            if (!identity) {
                PropertiesSession::Clear();
                return 0;
            }
            // PICKSTYLE can select an associative hatch and its boundary together.
            // They are one editable item; selections spanning different items are ignored.
            for (const auto id : ids) {
                const auto other = Autocad::Network::ResolveCurrentVisualization(id);
                if (!other || other->visualization_id != identity->visualization_id) {
                    PropertiesSession::Clear();
                    return 0;
                }
            }
            using dsd::application::NetworkItemType;
            const auto type = identity->type == L"segment"        ? NetworkItemType::Segment
                              : identity->type == L"air_terminal" ? NetworkItemType::AirTerminal
                                                                  : NetworkItemType::Damper;
            PropertiesSession::Select(
                {std::string(identity->network_id.begin(), identity->network_id.end()),
                 std::string(identity->item_id.begin(), identity->item_id.end()), type});
            DuctUi::Show(DuctUi::Panel::Properties);
        } catch (const std::exception&) {
            PropertiesSession::Clear();
        } catch (CException* error) {
            error->Delete();
            PropertiesSession::Clear();
        }
        return 0;
    }
    DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(SelectionWindow, CWnd)
ON_MESSAGE(selectionChanged, OnSelectionChanged)
ON_MESSAGE(propertyApplyCompleted, OnPropertyApplyCompleted)
END_MESSAGE_MAP()
SelectionWindow window;
bool initialized = false;
class EditorReactor final : public AcEditorReactor {
    void pickfirstModified() override { window.Schedule(); }
    void commandEnded(const ACHAR*) override { window.ResumeCompletion(); }
    void commandCancelled(const ACHAR*) override { window.ResumeCompletion(); }
    void commandFailed(const ACHAR*) override { window.ResumeCompletion(); }
};
EditorReactor editorReactor;
class DocumentReactor final : public AcApDocManagerReactor {
    void documentBecameCurrent(AcApDocument*) override { window.Schedule(); }
};
DocumentReactor documentReactor;
} // namespace

void Autocad::VisualizationSelection::Initialize() {
    if (initialized)
        return;
    CAcModuleResourceOverride resources;
    if (!window.CreateEx(0, AfxRegisterWndClass(0), L"DSD visualization selection", 0, 0, 0, 0, 0,
                         HWND_MESSAGE, nullptr)) {
        acutPrintf(L"\nCould not create the DSD visualization selection listener.");
        return;
    }
    acedEditor->addReactor(&editorReactor);
    acDocManager->addReactor(&documentReactor);
    initialized = true;
}
void Autocad::VisualizationSelection::Shutdown() {
    if (!initialized)
        return;
    initialized = false;
    acedEditor->removeReactor(&editorReactor);
    acDocManager->removeReactor(&documentReactor);
    if (::IsWindow(window.m_hWnd))
        window.DestroyWindow();
    PropertiesSession::Clear();
}
void Autocad::VisualizationSelection::CompletePropertyApply() { window.CompleteApply(); }
