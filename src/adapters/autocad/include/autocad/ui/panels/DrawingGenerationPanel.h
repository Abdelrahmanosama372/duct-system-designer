#pragma once
#include "autocad/ui/layout/TabbedContent.h"
#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include "autocad/ui/controls/SearchableComboBox.h"
#include <array>
#include <cmath>
#include <cwchar>
#include <exception>

namespace DrawingPreview { inline int networkId=1; }
enum DrawingControl : UINT
{
    DG_TITLE=6100,DG_SUBTITLE,DG_NETWORK_LABEL,DG_NETWORK,DG_SECTION_ONE,DG_SECTION_TWO,DG_SECTION_THREE,
    DG_REP_LABEL,DG_REP,DG_REDUCER_LABEL,DG_REDUCER,DG_TAKEOFF_LABEL,DG_TAKEOFF,DG_ELBOW_LABEL,DG_ELBOW,
    DG_DUCT_LAYER_LABEL,DG_DUCT_LAYER,DG_CENTERLINE_LAYER_LABEL,DG_CENTERLINE_LAYER,
    DG_DIM_LABEL,DG_DIM,DG_FLOW_LABEL,DG_FLOW,DG_BOD_LABEL,DG_BOD,
    DG_TEXT_HEIGHT_LABEL,DG_TEXT_HEIGHT,DG_TEXT_STYLE_LABEL,DG_TEXT_STYLE,DG_TEXT_LAYER_LABEL,DG_TEXT_LAYER,
    DG_PLACEMENT_LABEL,DG_PLACEMENT,DG_OFFSET_LABEL,DG_OFFSET,DG_GENERATE,DG_REMOVE,DG_STATUS
};
class DrawingGenerationPage : public ScrollPage
{
    bool m_annotations;
    SearchableComboBox& DrawingChoice(UINT id) {
        return *static_cast<SearchableComboBox*>(GetDlgItem(id));
    }
    void CreateDrawingChoice(UINT id) {
        auto& combo=Control<SearchableComboBox>(L"",CBS_DROPDOWN|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,id);
        combo.ConfigureSpacing();
    }
    void RefreshDrawingChoices() {
        try {
            const auto layers=NetworkActions::DrawingLayers();
            if(m_annotations) {
                DrawingChoice(DG_TEXT_LAYER).SetItems(layers,L"M-HVAC-TEXT");
                DrawingChoice(DG_TEXT_STYLE).SetItems(NetworkActions::DrawingTextStyles(),L"Standard");
            } else {
                DrawingChoice(DG_DUCT_LAYER).SetItems(layers,L"M-HVAC-DUCT");
                DrawingChoice(DG_CENTERLINE_LAYER).SetItems(layers,L"M-HVAC-CENTERLINE");
            }
        } catch(const std::exception& error) {
            DrawingChoice(m_annotations?DG_TEXT_LAYER:DG_DUCT_LAYER).SetItems({},L"");
            DrawingChoice(m_annotations?DG_TEXT_STYLE:DG_CENTERLINE_LAYER).SetItems({},L"");
            SetDlgItemText(DG_STATUS,CString(error.what()));
        }
    }
    void Combo(UINT id,std::initializer_list<LPCTSTR> items)
    {
        auto& combo=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,id);
        for(auto text:items)combo.AddString(text);combo.SetCurSel(0);
    }
    void Edit(UINT id,LPCTSTR value)
    {auto& edit=Control<CEdit>(L"",ES_AUTOHSCROLL,id);edit.SetLimitText(12);edit.SetWindowText(value);}
    void Row(UINT label,UINT field,int y,bool combo=true)
    {
        const int full=PageWidth()-56,labelWidth=210;
        Place(label,28,y, labelWidth-12,34);
        const int dropHeight=(field==DG_DUCT_LAYER || field==DG_CENTERLINE_LAYER || field==DG_TEXT_LAYER || field==DG_TEXT_STYLE)?220:240;
        Place(field,28+labelWidth,y,full-labelWidth,combo?dropHeight:34);
    }
    bool WithCenterlines() const {
        return !m_annotations && static_cast<CComboBox*>(GetDlgItem(DG_REP))->GetCurSel()==1;
    }
    void UpdateCenterlineControls() {
        const int show=WithCenterlines()?SW_SHOW:SW_HIDE;
        GetDlgItem(DG_CENTERLINE_LAYER_LABEL)->ShowWindow(show);
        GetDlgItem(DG_CENTERLINE_LAYER)->ShowWindow(show);
    }
    bool Valid(UINT id,bool allowZero)
    {
        CString text;GetDlgItemText(id,text);text.Trim();wchar_t* end=nullptr;
        const double value=std::wcstod(text.GetString(),&end);
        if(text.IsEmpty() || end==text.GetString() || *end!=L'\0' || !std::isfinite(value) || (allowZero?value<0:value<=0))
        {
            SetDlgItemText(DG_STATUS,allowZero?L"Enter a valid non-negative value in millimeters.":L"Enter a valid positive value in millimeters.");
            GetDlgItem(id)->SetFocus();EnsureVisible(GetDlgItem(id));return false;
        }
        return true;
    }
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        if(m_annotations)Sections({{120,212},{220,508},{512,734},{742,904}});
        else Sections({{120,212},{220,362},{370,592},{600,762}});
        const int full=PageWidth()-56;
        Place(DG_TITLE,28,20,full,44);Place(DG_SUBTITLE,28,76,full,42);
        Place(DG_NETWORK_LABEL,28,134,full,24);Place(DG_NETWORK,28,166,full,240);
        Place(DG_SECTION_ONE,28,232,full,28);
        if(!m_annotations)
        {
            Place(DG_REP_LABEL,28,280,full,24);Place(DG_REP,28,312,full,240);
            Place(DG_SECTION_TWO,28,382,full,28);
            Row(DG_REDUCER_LABEL,DG_REDUCER,430,false);Row(DG_TAKEOFF_LABEL,DG_TAKEOFF,486,false);
            Row(DG_ELBOW_LABEL,DG_ELBOW,542);
            Place(DG_SECTION_THREE,28,612,full,28);
            Row(DG_DUCT_LAYER_LABEL,DG_DUCT_LAYER,660);Row(DG_CENTERLINE_LAYER_LABEL,DG_CENTERLINE_LAYER,716);
        }
        else
        {
            Place(DG_DIM_LABEL,28,280,full,24);Place(DG_DIM,36,310,full-12,30);
            Place(DG_FLOW_LABEL,28,356,full,24);Place(DG_FLOW,36,386,full-12,30);
            Place(DG_BOD_LABEL,28,432,full,24);Place(DG_BOD,36,462,full-12,30);
            Place(DG_SECTION_TWO,28,524,full,28);
            Row(DG_TEXT_HEIGHT_LABEL,DG_TEXT_HEIGHT,572,false);Row(DG_TEXT_STYLE_LABEL,DG_TEXT_STYLE,628);
            Row(DG_TEXT_LAYER_LABEL,DG_TEXT_LAYER,684);
            Place(DG_SECTION_THREE,28,754,full,28);
            Row(DG_PLACEMENT_LABEL,DG_PLACEMENT,802);Row(DG_OFFSET_LABEL,DG_OFFSET,858,false);
        }
        const int actionY=m_annotations?940:798;
        Place(DG_GENERATE,28,actionY,full,38);Place(DG_REMOVE,28,actionY+52,full,38);
        Place(DG_STATUS,28,actionY+114,full,64);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(m_annotations?L"Annotations":L"Geometry",DG_TITLE,true).SetFont(&m_title);
        static_cast<UiHeading*>(GetDlgItem(DG_TITLE))->SetPlainTitle();
        Label(m_annotations?L"Choose annotation content, appearance and placement.":L"Configure duct geometry, fittings and output layers.",DG_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(DG_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(L"Network",DG_NETWORK_LABEL,true);
        static_cast<UiHeading*>(GetDlgItem(DG_NETWORK_LABEL))->SetNumber(1);
        Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,DG_NETWORK);
        Label(m_annotations?L"Annotation content":L"Duct geometry",DG_SECTION_ONE,true);
        Label(m_annotations?L"Annotation appearance":L"Fitting configuration",DG_SECTION_TWO,true);
        Label(m_annotations?L"Annotation placement":L"Output layers",DG_SECTION_THREE,true);
        const UINT headings[]={DG_SECTION_ONE,DG_SECTION_TWO,DG_SECTION_THREE};
        for(int i=0;i<3;++i)static_cast<UiHeading*>(GetDlgItem(headings[i]))->SetNumber(i+2);
        if(!m_annotations)
        {
            Label(L"Duct representation",DG_REP_LABEL);Combo(DG_REP,{L"Duct lines",L"Duct lines with centerlines"});
            Label(L"Reducer length (mm)",DG_REDUCER_LABEL);Edit(DG_REDUCER,L"300");
            Label(L"Takeoff length (mm)",DG_TAKEOFF_LABEL);Edit(DG_TAKEOFF,L"150");
            Label(L"Elbow inside-radius ratio",DG_ELBOW_LABEL);Combo(DG_ELBOW,{L"1",L"1.5",L"2"});
            static_cast<CComboBox*>(GetDlgItem(DG_ELBOW))->SetCurSel(1);
            Label(L"Duct geometry layer",DG_DUCT_LAYER_LABEL);CreateDrawingChoice(DG_DUCT_LAYER);
            Label(L"Duct centerline layer",DG_CENTERLINE_LAYER_LABEL);CreateDrawingChoice(DG_CENTERLINE_LAYER);
            UpdateCenterlineControls();
        }
        else
        {
            Label(L"Duct dimensions",DG_DIM_LABEL);Control<CButton>(L"Width x Height",BS_AUTOCHECKBOX,DG_DIM).SetCheck(BST_CHECKED);
            Label(L"Airflow",DG_FLOW_LABEL);Control<CButton>(L"Flow (CFM)",BS_AUTOCHECKBOX,DG_FLOW).SetCheck(BST_CHECKED);
            Label(L"Bottom of duct",DG_BOD_LABEL);Control<CButton>(L"BOD",BS_AUTOCHECKBOX,DG_BOD).SetCheck(BST_CHECKED);
            Label(L"Text height (mm)",DG_TEXT_HEIGHT_LABEL);Edit(DG_TEXT_HEIGHT,L"150");
            Label(L"Text style",DG_TEXT_STYLE_LABEL);CreateDrawingChoice(DG_TEXT_STYLE);
            Label(L"Text layer",DG_TEXT_LAYER_LABEL);CreateDrawingChoice(DG_TEXT_LAYER);
            Label(L"Placement",DG_PLACEMENT_LABEL);Combo(DG_PLACEMENT,{L"Above Duct",L"Below Duct",L"Centered on Duct"});
            Label(L"Offset from duct (mm)",DG_OFFSET_LABEL);Edit(DG_OFFSET,L"100");
        }
        Control<CButton>(m_annotations?L"Generate / Regenerate Annotations":L"Generate / Regenerate Geometry",BS_PUSHBUTTON,DG_GENERATE);
        static_cast<UiButton*>(GetDlgItem(DG_GENERATE))->SetRole(UiStyle::ButtonRole::Generation);
        Control<CButton>(m_annotations?L"Remove Generated Annotations":L"Remove Generated Geometry",BS_PUSHBUTTON,DG_REMOVE);
        static_cast<UiButton*>(GetDlgItem(DG_REMOVE))->SetRole(UiStyle::ButtonRole::Removal);
        Label(m_annotations?L"Generate network geometry before generating annotations.":L"Select a registered network to generate geometry.",DG_STATUS);
        UiStyle::TextColor(*GetDlgItem(DG_STATUS),UiStyle::ProjectReferenceStyle.muted);
        NetworkSession::Attach(GetSafeHwnd());
        RefreshNetworks();m_contentHeight=m_annotations?1144:1002;m_layoutReady=true;Relayout();return 0;
    }
    afx_msg void OnDestroy(){NetworkSession::Detach(GetSafeHwnd());ScrollPage::OnDestroy();}
    afx_msg LRESULT OnNetworkUpdated(WPARAM,LPARAM) {
        SetDlgItemText(DG_STATUS,NetworkSession::Current().message);
        return 0;
    }
    afx_msg BOOL OnEraseBkgnd(CDC* dc)
    {
        ScrollPage::OnEraseBkgnd(dc);
        dc->FillSolidRect(Px(24),Px(LayoutY(m_annotations?916:774))-m_scroll,Px(PageWidth()-56),1,UiStyle::ProjectReferenceStyle.border);return TRUE;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if((!m_annotations && (id==DG_DUCT_LAYER || id==DG_CENTERLINE_LAYER)) ||
           (m_annotations && (id==DG_TEXT_LAYER || id==DG_TEXT_STYLE))) {
            if(code==CBN_EDITCHANGE){DrawingChoice(id).FilterEdit();return TRUE;}
            if(code==CBN_SETFOCUS){RefreshDrawingChoices();return TRUE;}
        }
        if(id==DG_REP && code==CBN_SELCHANGE && !m_annotations) {
            UpdateCenterlineControls();return TRUE;
        }
        if(id==DG_NETWORK && code==CBN_SELCHANGE)
        {
            auto* combo=static_cast<CComboBox*>(GetDlgItem(DG_NETWORK));const int row=combo->GetCurSel();
            if(row>=0)DrawingPreview::networkId=static_cast<int>(combo->GetItemData(row));return TRUE;
        }
        if(code==BN_CLICKED && (id==DG_GENERATE || id==DG_REMOVE))
        {
            auto* combo=static_cast<CComboBox*>(GetDlgItem(DG_NETWORK));
            if(combo->GetCurSel()<0){SetDlgItemText(DG_STATUS,L"Select a registered network first.");return TRUE;}
            if(id==DG_GENERATE)
            {
                if(m_annotations)
                {
                    if(!IsDlgButtonChecked(DG_DIM) && !IsDlgButtonChecked(DG_FLOW) && !IsDlgButtonChecked(DG_BOD))
                    {SetDlgItemText(DG_STATUS,L"Select at least one annotation content option.");return TRUE;}
                    if(!Valid(DG_TEXT_HEIGHT,false) || !Valid(DG_OFFSET,true))return TRUE;
                }
                else if(!Valid(DG_REDUCER,false) || !Valid(DG_TAKEOFF,false))return TRUE;
            }
            if(!m_annotations) {
                NetworkActions::GeometryRequest request;
                const auto selected=static_cast<int>(combo->GetItemData(combo->GetCurSel()));
                request.network_id=dsd::application::network_item_id("NET",selected);
                request.remove=id==DG_REMOVE;
                if(!request.remove) {
                    CString text;
                    GetDlgItemText(DG_REDUCER,text);request.reducer_length_mm=std::wcstod(text.GetString(),nullptr);
                    GetDlgItemText(DG_TAKEOFF,text);request.takeoff_length_mm=std::wcstod(text.GetString(),nullptr);
                    GetDlgItemText(DG_ELBOW,text);request.elbow_inside_radius_ratio=std::wcstod(text.GetString(),nullptr);
                    if(!DrawingChoice(DG_DUCT_LAYER).SelectedValue(request.duct_layer)) {
                        SetDlgItemText(DG_STATUS,L"Select an existing duct geometry layer from the list.");return TRUE;
                    }
                    request.representation=WithCenterlines()?NetworkActions::GeometryRepresentation::OutlineWithCenterlines:
                        NetworkActions::GeometryRepresentation::Outline;
                    if(WithCenterlines()) {
                        if(!DrawingChoice(DG_CENTERLINE_LAYER).SelectedValue(request.centerline_layer)) {
                            SetDlgItemText(DG_STATUS,L"Select an existing duct centerline layer from the list.");return TRUE;
                        }
                    }
                }
                NetworkActions::RequestGeometry(request,GetSafeHwnd());
                SetDlgItemText(DG_STATUS,NetworkSession::Current().message);
                return TRUE;
            }
            NetworkActions::AnnotationRequest request;
            request.network_id=dsd::application::network_item_id("NET",static_cast<int>(combo->GetItemData(combo->GetCurSel())));
            request.remove=id==DG_REMOVE;
            if(!request.remove) {
                auto& settings=request.settings;
                settings.flow=IsDlgButtonChecked(DG_FLOW)==BST_CHECKED;
                settings.dimensions=IsDlgButtonChecked(DG_DIM)==BST_CHECKED;
                settings.bod=IsDlgButtonChecked(DG_BOD)==BST_CHECKED;
                CString text;
                GetDlgItemText(DG_TEXT_HEIGHT,text);settings.text_height_mm=std::wcstod(text.GetString(),nullptr);
                GetDlgItemText(DG_OFFSET,text);settings.offset_mm=std::wcstod(text.GetString(),nullptr);
                const int placement=static_cast<CComboBox*>(GetDlgItem(DG_PLACEMENT))->GetCurSel();
                using dsd::application::AnnotationPlacement;
                settings.placement=placement==0?AnnotationPlacement::Above:placement==1?AnnotationPlacement::Below:AnnotationPlacement::Centered;
                if(!DrawingChoice(DG_TEXT_LAYER).SelectedValue(settings.layer) ||
                   !DrawingChoice(DG_TEXT_STYLE).SelectedValue(settings.text_style)) {
                    SetDlgItemText(DG_STATUS,L"Select an existing drawing layer and text style.");return TRUE;
                }
            }
            NetworkActions::RequestAnnotations(request,GetSafeHwnd());
            SetDlgItemText(DG_STATUS,NetworkSession::Current().message);
            return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
    DECLARE_MESSAGE_MAP()
public:
    explicit DrawingGenerationPage(bool annotations):m_annotations(annotations){}
    void RefreshNetworks()
    {
        RefreshDrawingChoices();
        auto* combo=static_cast<CComboBox*>(GetDlgItem(DG_NETWORK));combo->ResetContent();int selected=-1;
        for(const auto& network:NetworkSession::Drafts())
        {
            int row=combo->AddString(network.name);if(row<0)continue;
            combo->SetItemData(row,network.id);if(network.id==DrawingPreview::networkId)selected=row;
        }
        if(selected<0 && combo->GetCount()>0){selected=0;DrawingPreview::networkId=static_cast<int>(combo->GetItemData(0));}
        combo->SetCurSel(selected);combo->EnableWindow(combo->GetCount()>0);
    }
};


class DrawingGenerationContent : public TabbedContent
{
    DrawingGenerationPage m_geometry{false},m_annotations{true};
protected:
    void PageActivated(int index) override{if(index==0)m_geometry.RefreshNetworks();else m_annotations.RefreshNetworks();}
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        if(CWnd::OnCreate(cs)==-1)return -1;
        if(!CreateHeader(L"Drawing Generation",L"DRAWING OUTPUT",6500,6501,6499))return -1;
        if(!m_geometry.CreatePage(this,6601) || !m_annotations.CreatePage(this,6602))return -1;
        AttachPages({&m_geometry,&m_annotations},{L"Geometry",L"Annotations"});return 0;
    }
public:
    BOOL CreateView(CWnd* parent,UINT id){return TabbedContent::CreateView(parent,id,L"Drawing Generation");}
    DECLARE_MESSAGE_MAP()
};
