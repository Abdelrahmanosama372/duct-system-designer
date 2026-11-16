#include "autocad/ui/panels/PropertiesPanel.h"
#ifndef DUCT_UI_NATIVE_TESTS
#include "autocad/ui/PalettePage.h"
#endif
#include <stdexcept>

namespace {
using namespace dsd::application;
CString wide(const std::string& value) {
    return CString(std::wstring(value.begin(), value.end()).c_str());
}
CString selected_ids(const PropertiesSession::Selection& selection) {
    CString result;
    for (const auto& id : selection.item_ids) {
        if (!result.IsEmpty())
            result += L" - ";
        result += wide(id);
    }
    return result;
}
CString selection_note(const PropertiesSession::Selection& selection) {
    if (selection.item_ids.size() == 1)
        return L"Changes are saved to the current network draft when you click Apply Changes.";
    CString note;
    note.Format(
        L"Showing values from %s. Apply uses all displayed values for all %zu selected items.",
        wide(selection.item_ids.front()).GetString(), selection.item_ids.size());
    return note;
}
CString text(CWnd& page, UINT id) {
    CString value;
    page.GetDlgItemText(id, value);
    value.Trim();
    return value;
}
std::string narrow(const CString& value) {
    std::string result;
    for (int i = 0; i < value.GetLength(); ++i) {
        if (value[i] > 127)
            throw std::invalid_argument("Use numbers and the listed property values");
        result.push_back(static_cast<char>(value[i]));
    }
    return result;
}
void set_combo(CWnd& page, UINT id, const CString& value) {
    auto* combo = static_cast<CComboBox*>(page.GetDlgItem(id));
    const int row = combo->FindStringExact(-1, value);
    if (row >= 0)
        combo->SetCurSel(row);
    else {
        combo->SetCurSel(-1);
        combo->SetWindowText(value);
    }
}
void dimensions(CComboBox& box, LPCTSTR automatic) {
    box.AddString(automatic);
    for (int size = 100; size <= 2000; size += 25) {
        CString value;
        value.Format(L"%d", size);
        box.AddString(value);
    }
}
void sizes(CComboBox& box) {
    for (auto value : {L"none", L"150 x 150", L"200 x 200", L"250 x 250", L"300 x 300",
                       L"400 x 400", L"600 x 600"})
        box.AddString(value);
}
std::pair<std::string, std::string> neck(const CString& value) {
    if (value == L"none")
        return {"none", "none"};
    const int separator = value.Find(L'x');
    if (separator < 0)
        throw std::invalid_argument("Neck size must be none or width x height in millimetres");
    auto width = value.Left(separator), height = value.Mid(separator + 1);
    width.Trim();
    height.Trim();
    return {narrow(width), narrow(height)};
}
constexpr const wchar_t* emptySelection =
    L"Select current visualization boundaries or hatches of one type in the drawing.";
} // namespace

void SegmentPropertiesPage::LayoutContent() {
    Sections({{124, 352}, {364, 680}});
    const int width = PageWidth(), margin = 28, full = width - 56;
    Place(PC_TITLE, margin, 20, full, 44);
    Place(PC_SUBTITLE, margin, 76, full, 42);
    Place(PC_SELECTION_SECTION, margin, 138, full, 28);
    Place(PC_SEGMENT_LABEL, margin, 182, full, 24);
    Place(PC_SEGMENT, margin, 214, full, 36);
    Place(PC_NETWORK_LABEL, margin, 270, full, 24);
    Place(PC_NETWORK, margin, 302, full, 36);
    Place(PC_DATA_SECTION, margin, 378, full, 28);
    Place(PC_FLOW_LABEL, margin, 422, full, 24);
    Place(PC_FLOW, margin, 454, full, 36);
    Place(PC_WIDTH_LABEL, margin, 510, full, 24);
    Place(PC_WIDTH, margin, 542, full, 240);
    Place(PC_HEIGHT_LABEL, margin, 598, full, 24);
    Place(PC_HEIGHT, margin, 630, full, 240);
    const int buttonWidth = min(240, full);
    Place(PC_APPLY, (width - buttonWidth) / 2, 708, buttonWidth, 40);
    Place(PC_STATUS, margin, 770, full, 60);
}
int SegmentPropertiesPage::OnCreate(LPCREATESTRUCT cs) {
    UiStyle::SetVisualStyle(*this, UiStyle::ProjectReferenceStyle);
    if (ScrollPage::OnCreate(cs) == -1)
        return -1;
    ReferenceTypography();
    Label(L"Selection", PC_SELECTION_SECTION, true);
    Label(L"Properties", PC_DATA_SECTION, true);
    static_cast<UiHeading*>(GetDlgItem(PC_SELECTION_SECTION))->SetNumber(1);
    static_cast<UiHeading*>(GetDlgItem(PC_DATA_SECTION))->SetNumber(2);
    Label(L"Segment properties", PC_TITLE, true).SetFont(&m_title);
    static_cast<UiHeading*>(GetDlgItem(PC_TITLE))->SetPlainTitle();
    Label(L"Adjust flow and dimensions for the selected duct segments.", PC_SUBTITLE);
    UiStyle::TextColor(*GetDlgItem(PC_SUBTITLE), UiStyle::ProjectReferenceStyle.muted);
    Label(L"Selected segments:", PC_SEGMENT_LABEL);
    Control<CEdit>(L"", ES_READONLY | ES_AUTOHSCROLL, PC_SEGMENT);
    Label(L"Network name:", PC_NETWORK_LABEL);
    Control<CEdit>(L"", ES_READONLY | ES_AUTOHSCROLL, PC_NETWORK);
    Label(L"Flow (CFM):", PC_FLOW_LABEL);
    Control<CEdit>(L"", ES_AUTOHSCROLL, PC_FLOW).SetLimitText(16);
    Label(L"Width (mm):", PC_WIDTH_LABEL);
    dimensions(Control<CComboBox>(L"", CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL, PC_WIDTH),
               L"AC");
    Label(L"Height (mm):", PC_HEIGHT_LABEL);
    dimensions(Control<CComboBox>(L"", CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL, PC_HEIGHT),
               L"AC");
    Control<CButton>(L"Apply Changes", BS_PUSHBUTTON, PC_APPLY);
    static_cast<UiButton*>(GetDlgItem(PC_APPLY))->SetPrimary(true);
    Label(emptySelection, PC_STATUS);
    UiStyle::TextColor(*GetDlgItem(PC_STATUS), UiStyle::ProjectReferenceStyle.muted);
    m_contentHeight = 856;
    m_layoutReady = true;
    Relayout();
    LoadSelection();
    return 0;
}
void SegmentPropertiesPage::LoadSelection() {
    const auto snapshot = PropertiesSession::Read();
    const bool active = snapshot && snapshot->selection.type == NetworkItemType::Segment;
    m_selection = active ? std::optional{snapshot->selection} : std::nullopt;
    SetDlgItemText(PC_SEGMENT, active ? selected_ids(snapshot->selection) : CString());
    SetDlgItemText(PC_NETWORK, active ? CString(snapshot->network_name.c_str()) : CString());
    SetDlgItemText(PC_FLOW, active ? wide(snapshot->metadata.at("flow")) : CString());
    set_combo(*this, PC_WIDTH, active ? wide(snapshot->metadata.at("width")) : CString());
    set_combo(*this, PC_HEIGHT, active ? wide(snapshot->metadata.at("height")) : CString());
    for (UINT id : {PC_FLOW, PC_WIDTH, PC_HEIGHT, PC_APPLY})
        GetDlgItem(id)->EnableWindow(active);
    SetDlgItemText(PC_STATUS,
                   active ? selection_note(snapshot->selection) : CString(emptySelection));
}
BOOL SegmentPropertiesPage::OnCommand(WPARAM wp, LPARAM lp) {
    if (LOWORD(wp) == PC_APPLY && HIWORD(wp) == BN_CLICKED) {
        if (!m_selection)
            return TRUE;
        try {
            PropertiesSession::Apply(*m_selection, {{"flow", narrow(text(*this, PC_FLOW))},
                                                    {"width", narrow(text(*this, PC_WIDTH))},
                                                    {"height", narrow(text(*this, PC_HEIGHT))}});
            SetDlgItemText(PC_STATUS, L"Changes saved to the current network draft.");
        } catch (const std::exception& error) {
            SetDlgItemText(PC_STATUS, CString(error.what()));
        }
        return TRUE;
    }
    return ScrollPage::OnCommand(wp, lp);
}

void EquipmentPropertiesPage::LayoutContent() {
    Sections({{124, 352}, {364, m_damper ? 768 : 680}});
    const int width = PageWidth(), margin = 28, full = width - 56;
    Place(EP_TITLE, margin, 20, full, 44);
    Place(EP_SUBTITLE, margin, 76, full, 42);
    Place(EP_SELECTION_SECTION, margin, 138, full, 28);
    Place(EP_SELECTED_LABEL, margin, 182, full, 24);
    Place(EP_SELECTED, margin, 214, full, 36);
    Place(EP_NETWORK_LABEL, margin, 270, full, 24);
    Place(EP_NETWORK, margin, 302, full, 36);
    Place(EP_DATA_SECTION, margin, 378, full, 28);
    const UINT labels[] = {EP_FIRST_LABEL, EP_SECOND_LABEL, EP_THIRD_LABEL, EP_FOURTH_LABEL};
    const UINT fields[] = {EP_FIRST, EP_SECOND, EP_THIRD, EP_FOURTH};
    for (int i = 0; i < (m_damper ? 4 : 3); ++i) {
        Place(labels[i], margin, 422 + i * 88, full, 24);
        Place(fields[i], margin, 454 + i * 88, full,
              ((m_damper && i < 2) || (!m_damper && i > 0)) ? 240 : 36);
    }
    const int buttonWidth = min(240, full), buttonY = m_damper ? 796 : 708;
    Place(EP_APPLY, (width - buttonWidth) / 2, buttonY, buttonWidth, 40);
    Place(EP_STATUS, margin, buttonY + 62, full, 60);
}
int EquipmentPropertiesPage::OnCreate(LPCREATESTRUCT cs) {
    UiStyle::SetVisualStyle(*this, UiStyle::ProjectReferenceStyle);
    if (ScrollPage::OnCreate(cs) == -1)
        return -1;
    ReferenceTypography();
    Label(L"Selection", EP_SELECTION_SECTION, true);
    Label(L"Properties", EP_DATA_SECTION, true);
    static_cast<UiHeading*>(GetDlgItem(EP_SELECTION_SECTION))->SetNumber(1);
    static_cast<UiHeading*>(GetDlgItem(EP_DATA_SECTION))->SetNumber(2);
    Label(m_damper ? L"Damper properties" : L"Air terminal properties", EP_TITLE, true)
        .SetFont(&m_title);
    static_cast<UiHeading*>(GetDlgItem(EP_TITLE))->SetPlainTitle();
    Label(m_damper ? L"Adjust details for the selected dampers."
                   : L"Adjust flow and sizes for the selected air terminals.",
          EP_SUBTITLE);
    UiStyle::TextColor(*GetDlgItem(EP_SUBTITLE), UiStyle::ProjectReferenceStyle.muted);
    Label(m_damper ? L"Selected dampers:" : L"Selected air terminals:", EP_SELECTED_LABEL);
    Control<CEdit>(L"", ES_READONLY | ES_AUTOHSCROLL, EP_SELECTED);
    Label(L"Network name:", EP_NETWORK_LABEL);
    Control<CEdit>(L"", ES_READONLY | ES_AUTOHSCROLL, EP_NETWORK);
    if (m_damper) {
        Label(L"Damper type:", EP_FIRST_LABEL);
        auto& types = Control<CComboBox>(L"", CBS_DROPDOWNLIST | WS_VSCROLL, EP_FIRST);
        for (auto value : {L"none", L"Volume Control Damper", L"Fire Damper", L"Smoke Damper",
                           L"Fire/Smoke Damper", L"Backdraft Damper"})
            types.AddString(value);
        Label(L"Blade arrangement:", EP_SECOND_LABEL);
        auto& blades = Control<CComboBox>(L"", CBS_DROPDOWNLIST | WS_VSCROLL, EP_SECOND);
        for (auto value : {L"none", L"Opposed Blade", L"Parallel Blade"})
            blades.AddString(value);
        Label(L"Width (mm):", EP_THIRD_LABEL);
        Control<CEdit>(L"", ES_AUTOHSCROLL, EP_THIRD).SetLimitText(8);
        Label(L"Height (mm):", EP_FOURTH_LABEL);
        Control<CEdit>(L"", ES_AUTOHSCROLL, EP_FOURTH).SetLimitText(8);
    } else {
        Label(L"Flow (CFM):", EP_FIRST_LABEL);
        Control<CEdit>(L"", ES_AUTOHSCROLL, EP_FIRST).SetLimitText(16);
        Label(L"Neck size (mm):", EP_SECOND_LABEL);
        sizes(Control<CComboBox>(L"", CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL, EP_SECOND));
        Label(L"Face size (mm):", EP_THIRD_LABEL);
        sizes(Control<CComboBox>(L"", CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL, EP_THIRD));
    }
    Control<CButton>(L"Apply Changes", BS_PUSHBUTTON, EP_APPLY);
    static_cast<UiButton*>(GetDlgItem(EP_APPLY))->SetPrimary(true);
    Label(emptySelection, EP_STATUS);
    UiStyle::TextColor(*GetDlgItem(EP_STATUS), UiStyle::ProjectReferenceStyle.muted);
    m_contentHeight = m_damper ? 944 : 856;
    m_layoutReady = true;
    Relayout();
    LoadSelection();
    return 0;
}
void EquipmentPropertiesPage::LoadSelection() {
    const auto snapshot = PropertiesSession::Read();
    const auto type = m_damper ? NetworkItemType::Damper : NetworkItemType::AirTerminal;
    const bool active = snapshot && snapshot->selection.type == type;
    m_selection = active ? std::optional{snapshot->selection} : std::nullopt;
    SetDlgItemText(EP_SELECTED, active ? selected_ids(snapshot->selection) : CString());
    SetDlgItemText(EP_NETWORK, active ? CString(snapshot->network_name.c_str()) : CString());
    if (m_damper) {
        set_combo(*this, EP_FIRST, active ? wide(snapshot->metadata.at("damper_type")) : CString());
        set_combo(*this, EP_SECOND,
                  active ? wide(snapshot->metadata.at("blade_arrangement")) : CString());
        SetDlgItemText(EP_THIRD, active ? wide(snapshot->metadata.at("width")) : CString());
        SetDlgItemText(EP_FOURTH, active ? wide(snapshot->metadata.at("height")) : CString());
    } else {
        SetDlgItemText(EP_FIRST, active ? wide(snapshot->metadata.at("flow")) : CString());
        CString neckValue;
        if (active) {
            const auto& values = snapshot->metadata;
            neckValue = values.at("width") == "none" && values.at("height") == "none"
                            ? CString(L"none")
                            : wide(values.at("width") + " x " + values.at("height"));
        }
        set_combo(*this, EP_SECOND, neckValue);
        set_combo(*this, EP_THIRD, active ? wide(snapshot->metadata.at("face_size")) : CString());
    }
    for (UINT id : {EP_FIRST, EP_SECOND, EP_THIRD, EP_APPLY})
        GetDlgItem(id)->EnableWindow(active);
    if (m_damper)
        GetDlgItem(EP_FOURTH)->EnableWindow(active);
    SetDlgItemText(EP_STATUS,
                   active ? selection_note(snapshot->selection) : CString(emptySelection));
}
BOOL EquipmentPropertiesPage::OnCommand(WPARAM wp, LPARAM lp) {
    if (LOWORD(wp) == EP_APPLY && HIWORD(wp) == BN_CLICKED) {
        if (!m_selection)
            return TRUE;
        try {
            Metadata changes;
            if (m_damper) {
                changes = {{"damper_type", narrow(text(*this, EP_FIRST))},
                           {"blade_arrangement", narrow(text(*this, EP_SECOND))},
                           {"width", narrow(text(*this, EP_THIRD))},
                           {"height", narrow(text(*this, EP_FOURTH))}};
            } else {
                const auto [width, height] = neck(text(*this, EP_SECOND));
                changes = {{"flow", narrow(text(*this, EP_FIRST))},
                           {"width", width},
                           {"height", height},
                           {"face_size", narrow(text(*this, EP_THIRD))}};
            }
            PropertiesSession::Apply(*m_selection, changes);
            SetDlgItemText(EP_STATUS, L"Changes saved to the current network draft.");
        } catch (const std::exception& error) {
            SetDlgItemText(EP_STATUS, CString(error.what()));
        }
        return TRUE;
    }
    return ScrollPage::OnCommand(wp, lp);
}
int PropertiesContent::OnCreate(LPCREATESTRUCT cs) {
    if (CWnd::OnCreate(cs) == -1)
        return -1;
    if (!CreateHeader(L"Properties", L"ELEMENT PROPERTIES", 3700, 3701, 3699))
        return -1;
    if (!m_segments.CreatePage(this, 3801) || !m_terminals.CreatePage(this, 3802) ||
        !m_dampers.CreatePage(this, 3803))
        return -1;
    AttachPages({&m_segments, &m_terminals, &m_dampers},
                {L"Segments", L"Air Terminals", L"Dampers"});
    PropertiesSession::Attach(m_hWnd);
    OnPropertiesUpdated(0, 0);
    return 0;
}
void PropertiesContent::OnDestroy() {
    PropertiesSession::Detach(m_hWnd);
    TabbedContent::OnDestroy();
}
LRESULT PropertiesContent::OnPropertiesUpdated(WPARAM, LPARAM) {
    m_segments.LoadSelection();
    m_terminals.LoadSelection();
    m_dampers.LoadSelection();
    if (const auto selected = PropertiesSession::Read()) {
        const auto type = selected->selection.type;
        SelectView(type == NetworkItemType::Segment       ? 0
                   : type == NetworkItemType::AirTerminal ? 1
                                                          : 2);
    }
    return 0;
}
BEGIN_MESSAGE_MAP(SegmentPropertiesPage, ScrollPage)
ON_WM_CREATE()
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(EquipmentPropertiesPage, ScrollPage)
ON_WM_CREATE()
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(PropertiesContent, TabbedContent)
ON_WM_CREATE()
ON_WM_DESTROY()
ON_MESSAGE(WM_DSD_PROPERTIES_UPDATED, OnPropertiesUpdated)
END_MESSAGE_MAP()
#ifndef DUCT_UI_NATIVE_TESTS
std::unique_ptr<CAdUiPalette> MakePropertiesPanel() {
    return std::make_unique<ContentPalette<PropertiesContent, 3300, true>>();
}
#endif
