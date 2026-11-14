#pragma once
#include "autocad/ui/layout/TabbedContent.h"
#include "autocad/ui/model/NetworkSession.h"
#include <array>
#include <cmath>
#include <cwchar>

enum NetworkControl : UINT
{
    NC_TITLE = 2100, NC_CONFIG, NC_NAME_LABEL, NC_NAME, NC_SYSTEM_LABEL, NC_SYSTEM,
    NC_DEFINITION, NC_ROOT_LABEL, NC_ROOT, NC_TERMINAL_LABEL, NC_TERMINAL,
    NC_DAMPER_LABEL, NC_DAMPER, NC_SIZING, NC_MIN_LABEL, NC_MIN,
    NC_MAX_LABEL, NC_MAX, NC_INCREMENT_LABEL, NC_INCREMENT, NC_ASPECT_LABEL, NC_ASPECT,
    NC_LINE1, NC_ANALYZE, NC_REGISTER, NC_NEW, NC_LINE2, NC_VISUALIZATION,
    NC_SEGMENTS, NC_FLOW, NC_BOD, NC_DIMENSIONS, NC_REFRESH, NC_STATUS, NC_NOTICE,
    NC_METHOD_LABEL,NC_METHOD,NC_HEAD_LABEL,NC_HEAD,NC_VELOCITY_LABEL,NC_VELOCITY,
    NC_PARAMETERS,NC_MATERIAL_SECTION,NC_MATERIAL_LABEL,NC_MATERIAL,NC_ROUGHNESS_LABEL,NC_ROUGHNESS,
    NC_LIST_TITLE = 2300, NC_SEARCH_LABEL, NC_SEARCH, NC_COUNT, NC_EMPTY, NC_LIST_NOTICE, NC_SUBTITLE, NC_LIST_SUBTITLE
};
void RefreshNetworkCards();
void EditNetworkDraft(size_t index);

class NetworkRegistrationPage : public ScrollPage
{
    int m_editIndex = -1;
    bool m_loading = true;
    CEdit* m_name = nullptr;
    CComboBox *m_system = nullptr, *m_method = nullptr, *m_material = nullptr;
    CComboBox *m_min = nullptr, *m_max = nullptr, *m_increment = nullptr;

    CComboBox& Combo(UINT id, bool editable = false)
    {
        return Control<CComboBox>(L"", WS_VSCROLL | CBS_AUTOHSCROLL |
            (editable ? CBS_DROPDOWN : CBS_DROPDOWNLIST),id);
    }
    void Fill(CComboBox& box, std::initializer_list<LPCTSTR> values, LPCTSTR selected)
    {
        for (LPCTSTR value : values) box.AddString(value);
        box.SelectString(-1,selected);
    }
    CString Text(UINT id) const
    {
        CString value; GetDlgItem(id)->GetWindowText(value); value.Trim(); return value;
    }
    void Select(CComboBox* box, const CString& value)
    {
        int index = box->FindStringExact(-1,value);
        if (index >= 0) box->SetCurSel(index);
        else box->SetWindowText(value);
    }
    void Status(LPCTSTR value) { SetDlgItemText(NC_STATUS,value); }
    void UpdateMethod()
    {
        const bool friction=Text(NC_METHOD)==L"Equal Friction";
        for(UINT id:{NC_HEAD_LABEL,NC_HEAD})GetDlgItem(id)->ShowWindow(friction?SW_SHOW:SW_HIDE);
        for(UINT id:{NC_VELOCITY_LABEL,NC_VELOCITY})GetDlgItem(id)->ShowWindow(friction?SW_HIDE:SW_SHOW);
        Relayout();
    }
    CEdit& NumberEdit(UINT id,LPCTSTR value)
    {
        auto& edit=Control<CEdit>(L"",ES_AUTOHSCROLL,id);edit.SetLimitText(16);edit.SetWindowText(value);return edit;
    }
    bool ValidNumber(UINT id,double minimum,bool inclusive,LPCTSTR message)
    {
        CString text=Text(id);wchar_t* end=nullptr;double value=std::wcstod(text.GetString(),&end);
        if(text.IsEmpty() || end==text.GetString() || *end!=L'\0' || !std::isfinite(value) ||
            (inclusive?value<minimum:value<=minimum))
        {Status(message);GetDlgItem(id)->SetFocus();EnsureVisible(GetDlgItem(id));return false;}
        return true;
    }
    NetworkDraft Read() const
    {
        NetworkDraft draft;
        draft.name=Text(NC_NAME); draft.system=Text(NC_SYSTEM);
        draft.sizingMethod=Text(NC_METHOD);draft.headLoss=Text(NC_HEAD);draft.velocity=Text(NC_VELOCITY);
        draft.material=Text(NC_MATERIAL);draft.roughness=Text(NC_ROUGHNESS);
        draft.minimum=Text(NC_MIN); draft.maximum=Text(NC_MAX);
        draft.increment=Text(NC_INCREMENT); draft.aspect=Text(NC_ASPECT);
        draft.segments=IsDlgButtonChecked(NC_SEGMENTS)==BST_CHECKED;
        draft.flow=IsDlgButtonChecked(NC_FLOW)==BST_CHECKED;
        draft.bod=IsDlgButtonChecked(NC_BOD)==BST_CHECKED;
        draft.dimensions=IsDlgButtonChecked(NC_DIMENSIONS)==BST_CHECKED;
        return draft;
    }
    bool Validate()
    {
        if (Text(NC_NAME).IsEmpty())
        { Status(L"Enter a network name."); m_name->SetFocus(); EnsureVisible(m_name); return false; }
        if(!ValidNumber(NC_ASPECT,1,true,L"Maximum aspect ratio must be at least 1.0.") ||
            !ValidNumber(NC_ROUGHNESS,0,true,L"Absolute roughness must be a non-negative value in millimeters."))return false;
        if(Text(NC_METHOD)==L"Equal Friction")
        {if(!ValidNumber(NC_HEAD,0,false,L"Required head loss must be a positive number."))return false;}
        else if(!ValidNumber(NC_VELOCITY,0,false,L"Required velocity must be a positive number in FPM."))return false;
        if (_wtoi(Text(NC_MIN)) > _wtoi(Text(NC_MAX)))
        { Status(L"Minimum dimension must not exceed maximum dimension."); return false; }
        for (size_t i=0;i<NetworkSession::Drafts().size();++i)
            if (static_cast<int>(i)!=m_editIndex && NetworkSession::Drafts()[i].name.CompareNoCase(Text(NC_NAME))==0)
            { Status(L"That network name is already registered. Use Modify Network."); return false; }
        return true;
    }
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        Sections({{110,254},{260,448},{454,644},{656,888},{894,1084},{1220,1478}});
        const int w=PageWidth(),margin=28,field=244,full=w-56;
        auto heading=[&](UINT id,int y){Place(id,margin,y,full,28);};
        auto row=[&](UINT label,UINT control,int y,bool combo=false)
        {
            Place(label,margin,y+6,field-margin-12,24);
            Place(control,field,y,w-field-margin,combo?240:34);
        };
        Place(NC_TITLE,margin,20,full,44);Place(NC_SUBTITLE,margin,76,full,24);
        heading(NC_CONFIG,122);
        row(NC_NAME_LABEL,NC_NAME,164);row(NC_SYSTEM_LABEL,NC_SYSTEM,208,true);
        heading(NC_DEFINITION,272);
        row(NC_ROOT_LABEL,NC_ROOT,314);row(NC_TERMINAL_LABEL,NC_TERMINAL,358);row(NC_DAMPER_LABEL,NC_DAMPER,402);
        heading(NC_SIZING,466);
        Place(NC_METHOD_LABEL,margin,508,full,24);Place(NC_METHOD,margin,540,full,240);
        row(NC_HEAD_LABEL,NC_HEAD,598);row(NC_VELOCITY_LABEL,NC_VELOCITY,598);
        heading(NC_PARAMETERS,668);
        row(NC_INCREMENT_LABEL,NC_INCREMENT,710,true);row(NC_MIN_LABEL,NC_MIN,754,true);
        row(NC_MAX_LABEL,NC_MAX,798,true);row(NC_ASPECT_LABEL,NC_ASPECT,842);
        heading(NC_MATERIAL_SECTION,906);
        Place(NC_MATERIAL_LABEL,margin,948,full,24);Place(NC_MATERIAL,margin,980,full,240);
        row(NC_ROUGHNESS_LABEL,NC_ROUGHNESS,1038);
        const int half=(full-12)/2;
        Place(NC_ANALYZE,margin,1114,half,36);Place(NC_REGISTER,margin+half+12,1114,half,36);
        Place(NC_NEW,margin,1162,full,32);
        heading(NC_VISUALIZATION,1232);
        Place(NC_SEGMENTS,margin,1274,full,28);Place(NC_FLOW,margin,1310,full,28);
        Place(NC_BOD,margin,1346,full,28);Place(NC_DIMENSIONS,margin,1382,full,28);
        Place(NC_REFRESH,margin,1430,full,36);
        Place(NC_STATUS,margin,1486,full,44);Place(NC_NOTICE,margin,1546,full,44);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if (ScrollPage::OnCreate(cs)==-1) return -1;
        ReferenceTypography();
        Label(L"Network registration",NC_TITLE,true).SetFont(&m_title);
        static_cast<UiHeading*>(GetDlgItem(NC_TITLE))->SetPlainTitle();
        Label(L"Define sizing, material and network settings.",NC_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(NC_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(L"Network configuration",NC_CONFIG,true);
        Label(L"Name:",NC_NAME_LABEL); m_name=&Control<CEdit>(L"",WS_BORDER|ES_AUTOHSCROLL,NC_NAME);
        m_name->SetWindowText(L"Supply Network 04");
        m_name->SetLimitText(100);
        Label(L"System Type:",NC_SYSTEM_LABEL); m_system=&Combo(NC_SYSTEM);
        Fill(*m_system,{L"Supply Air",L"Return Air",L"Exhaust Air"},L"Supply Air");
        Label(L"Network definition",NC_DEFINITION,true);
        Label(L"Root:",NC_ROOT_LABEL); Control<CButton>(L"Pick Line",BS_PUSHBUTTON,NC_ROOT);
        Label(L"Air Terminal Block:",NC_TERMINAL_LABEL); Control<CButton>(L"Pick Block",BS_PUSHBUTTON,NC_TERMINAL);
        Label(L"Damper Block:",NC_DAMPER_LABEL); Control<CButton>(L"Pick Block",BS_PUSHBUTTON,NC_DAMPER);
        Label(L"Sizing configuration",NC_SIZING,true);
        Label(L"Sizing method",NC_METHOD_LABEL);m_method=&Combo(NC_METHOD);
        Fill(*m_method,{L"Equal Friction",L"Velocity"},L"Equal Friction");
        Label(L"Required head loss",NC_HEAD_LABEL);NumberEdit(NC_HEAD,L"0.1");
        Label(L"Required velocity (FPM)",NC_VELOCITY_LABEL);NumberEdit(NC_VELOCITY,L"1000");
        Label(L"Sizing parameters",NC_PARAMETERS,true);
        Label(L"Size increment (mm)",NC_INCREMENT_LABEL);m_increment=&Combo(NC_INCREMENT);
        Fill(*m_increment,{L"25",L"50",L"100"},L"25");
        Label(L"Minimum dimension (mm)",NC_MIN_LABEL);m_min=&Combo(NC_MIN);
        Fill(*m_min,{L"50",L"75",L"100",L"125",L"150",L"200",L"250",L"300",L"400",L"500"},L"100");
        Label(L"Maximum dimension (mm)",NC_MAX_LABEL);m_max=&Combo(NC_MAX);
        Fill(*m_max,{L"300",L"400",L"500",L"600",L"750",L"1000",L"1250",L"1500",L"2000",L"2500",L"3000"},L"2000");
        Label(L"Maximum aspect ratio",NC_ASPECT_LABEL);NumberEdit(NC_ASPECT,L"2.0");
        Label(L"Duct material",NC_MATERIAL_SECTION,true);
        Label(L"Material",NC_MATERIAL_LABEL);m_material=&Combo(NC_MATERIAL);
        Fill(*m_material,{L"Galvanized Steel",L"Stainless Steel",L"Aluminum",L"Flexible Duct"},L"Galvanized Steel");
        Label(L"Absolute roughness (mm)",NC_ROUGHNESS_LABEL);NumberEdit(NC_ROUGHNESS,L"0.15");
        Control<CButton>(L"Analyze Network",BS_PUSHBUTTON,NC_ANALYZE);
        static_cast<UiButton*>(GetDlgItem(NC_ANALYZE))->SetPrimary(true);
        Control<CButton>(L"Register",BS_PUSHBUTTON,NC_REGISTER,true);
        static_cast<UiButton*>(GetDlgItem(NC_REGISTER))->SetPrimary(true);
        Control<CButton>(L"New Network / Reset Form",BS_PUSHBUTTON,NC_NEW);
        Label(L"Network visualization",NC_VISUALIZATION,true);
        Control<CButton>(L"Segments",BS_AUTOCHECKBOX,NC_SEGMENTS);
        Control<CButton>(L"Flow",BS_AUTOCHECKBOX,NC_FLOW);
        Control<CButton>(L"BOD",BS_AUTOCHECKBOX,NC_BOD);
        Control<CButton>(L"Width and Height",BS_AUTOCHECKBOX,NC_DIMENSIONS);
        CheckDlgButton(NC_SEGMENTS,BST_CHECKED);
        Control<CButton>(L"Refresh Visualization",BS_PUSHBUTTON,NC_REFRESH);
        Label(L"UI preview ready. All network data is sample data.",NC_STATUS);
        Label(L"Sample data for UI preview only. Nothing is saved to the drawing.",NC_NOTICE);
        const UINT sections[]={NC_CONFIG,NC_DEFINITION,NC_SIZING,NC_PARAMETERS,NC_MATERIAL_SECTION,NC_VISUALIZATION};
        for(int i=0;i<6;++i)static_cast<UiHeading*>(GetDlgItem(sections[i]))->SetNumber(i+1);
        static_cast<UiEdit*>(m_name)->SetPlaceholder(L"Enter network name");
        UiStyle::TextColor(*GetDlgItem(NC_NOTICE),UiStyle::ProjectReferenceStyle.muted);
        m_loading=false;m_contentHeight=1612;m_layoutReady=true;UpdateMethod();return 0;
    }
    BOOL OnCommand(WPARAM wp, LPARAM lp) override
    {
        const UINT id=LOWORD(wp), code=HIWORD(wp);
        if(id==NC_METHOD && code==CBN_SELCHANGE)UpdateMethod();
        if (code==BN_CLICKED)
        {
            if (id==NC_REGISTER)
            {
                if (!Validate()) return TRUE;
                NetworkDraft draft=Read();
                if (m_editIndex>=0)
                { draft.id=NetworkSession::Drafts()[m_editIndex].id; NetworkSession::Drafts()[m_editIndex]=draft; }
                else
                {
                    if(NetworkSession::Drafts().size()>=1000) { Status(L"Session limit reached (1000 drafts)."); return TRUE; }
                    draft.id=NetworkSession::NextId()++; NetworkSession::Drafts().push_back(draft);
                    m_editIndex=static_cast<int>(NetworkSession::Drafts().size())-1;
                }
                SetDlgItemText(NC_REGISTER,L"Save Changes");
                Status(L"Draft saved for this session. See Registered Networks.");
                RefreshNetworkCards(); return TRUE;
            }
            if (id==NC_NEW) { NewDraft(); return TRUE; }
            if (id==NC_ANALYZE)
            {
                if(Validate()) Status(L"Form is valid. Network analysis is not connected yet.");
                return TRUE;
            }
            if (id==NC_ROOT || id==NC_TERMINAL || id==NC_DAMPER)
            { Status(L"Drawing selection is not connected in this UI version."); return TRUE; }
            if (id==NC_REFRESH)
            { Status(L"Visualization options set. Drawing visualization is not connected yet."); return TRUE; }
        }
        if (!m_loading && ((code==EN_CHANGE && (id==NC_NAME || id==NC_ASPECT || id==NC_HEAD || id==NC_VELOCITY || id==NC_ROUGHNESS)) || code==CBN_SELCHANGE ||
            code==CBN_EDITCHANGE || (code==BN_CLICKED && id>=NC_SEGMENTS && id<=NC_DIMENSIONS)))
            Status(L"Form changed. Click Register or Save Changes to keep the draft.");
        return ScrollPage::OnCommand(wp,lp);
    }
public:
    void LoadDraft(size_t index)
    {
        if(index>=NetworkSession::Drafts().size())return;
        const NetworkDraft& draft=NetworkSession::Drafts()[index];
        m_loading=true; m_editIndex=static_cast<int>(index);
        m_name->SetWindowText(draft.name); Select(m_system,draft.system);
        Select(m_method,draft.sizingMethod);Select(m_material,draft.material);
        SetDlgItemText(NC_HEAD,draft.headLoss);SetDlgItemText(NC_VELOCITY,draft.velocity);
        SetDlgItemText(NC_ROUGHNESS,draft.roughness);
        Select(m_min,draft.minimum); Select(m_max,draft.maximum);
        Select(m_increment,draft.increment);SetDlgItemText(NC_ASPECT,draft.aspect);
        CheckDlgButton(NC_SEGMENTS,draft.segments?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton(NC_FLOW,draft.flow?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton(NC_BOD,draft.bod?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton(NC_DIMENSIONS,draft.dimensions?BST_CHECKED:BST_UNCHECKED);
        SetDlgItemText(NC_REGISTER,L"Save Changes");
        Status(L"Editing registered draft. Click Save Changes to update it.");
        UpdateMethod();m_loading=false; ScrollTo(0);
    }
    void NewDraft()
    {
        m_loading=true; m_editIndex=-1; m_name->SetWindowText(L"");
        Select(m_system,L"Supply Air");Select(m_method,L"Equal Friction");Select(m_material,L"Galvanized Steel");
        Select(m_min,L"100");Select(m_max,L"2000");Select(m_increment,L"25");
        SetDlgItemText(NC_ASPECT,L"2.0");SetDlgItemText(NC_HEAD,L"0.1");
        SetDlgItemText(NC_VELOCITY,L"1000");SetDlgItemText(NC_ROUGHNESS,L"0.15");
        CheckDlgButton(NC_SEGMENTS,BST_CHECKED); CheckDlgButton(NC_FLOW,BST_UNCHECKED);
        CheckDlgButton(NC_BOD,BST_UNCHECKED); CheckDlgButton(NC_DIMENSIONS,BST_UNCHECKED);
        SetDlgItemText(NC_REGISTER,L"Register"); Status(L"Ready. Configure a new network draft.");
        UpdateMethod();m_loading=false; ScrollTo(0); m_name->SetFocus();
    }
    DECLARE_MESSAGE_MAP()
};


class RegisteredNetworksPage : public ScrollPage
{
    struct Card
    {
        size_t index=0;
        bool expanded=true;
        UiButton title, modify;
        CStatic meta, headings[3], values[3];
        UiSeparator divider;
    };
    std::vector<std::unique_ptr<Card>> m_cards;
    std::vector<CRect> m_cardRects;
    CEdit* m_search=nullptr;
    template<class T>
    void CardControl(T& control,LPCTSTR text,DWORD style,UINT id,bool bold=false)
    {
        if(!control.Create(text,WS_CHILD|WS_VISIBLE|style,CRect(0,0,1,1),this,id))
            AfxThrowResourceException();
        if constexpr(std::is_base_of_v<CButton,T>)control.SetFont(&m_buttonFont);
        else control.SetFont(bold?&m_bold:&m_font);
        UiStyle::Surface(control);
    }
    bool Matches(const NetworkDraft& draft) const
    {
        CString query; m_search->GetWindowText(query); query.Trim(); query.MakeLower();
        CString name=draft.name; name.MakeLower(); return name.Find(query)>=0;
    }
protected:
    void OffsetDecorations(int delta) override
    {for(auto& rect:m_cardRects)rect.OffsetRect(0,delta); }
    void LayoutContent() override
    {
        const int width=PageWidth(), margin=28, full=width-56;
        Place(NC_LIST_TITLE,margin,20,full,44);Place(NC_LIST_SUBTITLE,margin,76,full,24);
        Place(NC_SEARCH_LABEL,margin,122,56,24);Place(NC_SEARCH,92,116,width-120,36);
        Place(NC_COUNT,margin,164,full,22);
        m_cardRects.clear();
        int y=202;
        for(auto& pointer:m_cards)
        {
            Card& card=*pointer;
            const int height=card.expanded?232:64;
            m_cardRects.push_back(CRect(Px(margin),Px(LayoutY(y))-m_scroll,Px(margin+full),Px(LayoutY(y+height))-m_scroll));
            Place(card.title,margin+16,y+14,full-32,32);
            if(card.expanded)
            {
                Place(card.meta,margin+18,y+74,full-36,26);
                const int column=(full-36)/3;
                for(int i=0;i<3;++i)
                {
                    Place(card.headings[i],margin+18+i*column,y+114,column,22);
                    Place(card.values[i],margin+18+i*column,y+142,column,26);
                }
                Place(card.modify,margin+18,y+184,full-36,32);
            }
            y+=height;
            // Keep a separator in the gap between complete bordered cards.
            Place(card.divider,margin,y+6,full,1); y+=12;
        }
        Place(NC_EMPTY,margin,210,full,76);
        Place(NC_LIST_NOTICE,margin,max(y,302),full,56);
    }
    afx_msg BOOL OnEraseBkgnd(CDC* dc)
    {
        ScrollPage::OnEraseBkgnd(dc);
        for(const CRect& rect:m_cardRects)
        {
            UiStyle::Rounded(*dc,rect,UiStyle::Colors(m_hWnd).card,UiStyle::Colors(m_hWnd).sectionBorder,Px(6));
            if(rect.Height()>Px(64))
                dc->FillSolidRect(rect.left+Px(18),rect.top+Px(60),rect.Width()-Px(36),1,UiStyle::Colors(m_hWnd).border);
            dc->FillSolidRect(rect.left,rect.bottom+Px(6),rect.Width(),max(1,Px(1)),UiStyle::Colors(m_hWnd).border);
        }
        return TRUE;
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(L"Registered networks",NC_LIST_TITLE,true).SetFont(&m_title);
        static_cast<UiHeading*>(GetDlgItem(NC_LIST_TITLE))->SetPlainTitle();
        Label(L"Browse and manage your registered networks.",NC_LIST_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(NC_LIST_SUBTITLE),UiStyle::Colors(m_hWnd).muted);
        Label(L"Search:",NC_SEARCH_LABEL);
        m_search=&Control<CEdit>(L"",WS_BORDER|ES_AUTOHSCROLL,NC_SEARCH);
        m_search->SetLimitText(100);
        static_cast<UiEdit*>(m_search)->SetPlaceholder(L"Search network name...");
        Label(L"0 networks",NC_COUNT);
        Label(L"No registered networks yet.\nUse the Registration tab to create your first draft.",NC_EMPTY);
        Label(L"Sample networks, counts, flows and statuses for UI preview only. Changes last for this session.",NC_LIST_NOTICE);
        m_layoutReady=true; Rebuild(); return 0;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp), code=HIWORD(wp);
        if(id==NC_SEARCH && code==EN_CHANGE && m_layoutReady)
        { m_scroll=0; Rebuild(); return TRUE; }
        if(code==BN_CLICKED && id>=6000 && id<7000)
        {
            for(auto& card:m_cards) if(card->index==id-6000)
            {
                card->expanded=!card->expanded;
                const auto& draft=NetworkSession::Drafts()[card->index];
                card->title.SetWindowText((card->expanded?CString(L"\x25BC  "):CString(L"\x25B6  "))+draft.name);
                const int show=card->expanded?SW_SHOW:SW_HIDE;
                card->meta.ShowWindow(show); card->modify.ShowWindow(show);
                for(int i=0;i<3;++i){card->headings[i].ShowWindow(show);card->values[i].ShowWindow(show);}
                UpdateHeight(); Relayout(); break;
            }
            return TRUE;
        }
        if(code==BN_CLICKED && id>=8000 && id<9000)
        { EditNetworkDraft(id-8000); return TRUE; }
        return ScrollPage::OnCommand(wp,lp);
    }
    void UpdateHeight()
    {
        int bottom=202;
        for(const auto& card:m_cards) bottom+=(card->expanded?232:64)+12;
        m_contentHeight=max(bottom,302)+76;
    }
public:
    void Rebuild()
    {
        // Release control wrappers only after their HWNDs have been destroyed.
        for(auto& card:m_cards)
        {
            card->title.DestroyWindow(); card->modify.DestroyWindow(); card->meta.DestroyWindow();
            card->divider.DestroyWindow();
            for(int i=0;i<3;++i){card->headings[i].DestroyWindow();card->values[i].DestroyWindow();}
        }
        m_cards.clear();
        for(size_t index=0;index<NetworkSession::Drafts().size();++index)
        {
            const auto& draft=NetworkSession::Drafts()[index]; if(!Matches(draft))continue;
            auto card=std::make_unique<Card>(); card->index=index;
            CardControl(card->title,CString(L"\x25BC  ")+draft.name,WS_TABSTOP|BS_PUSHBUTTON|BS_LEFT,6000+static_cast<UINT>(index),true);
            card->title.SetFlat(true);card->title.SetSurface(UiStyle::Colors(m_hWnd).card);card->title.SetFont(&m_cardTitle);
            card->modify.SetSurface(UiStyle::Colors(m_hWnd).card);
            CString meta;meta.Format(L"ID: NET-%03d  |  System: %s",draft.id,draft.system.GetString());
            CardControl(card->meta,meta,SS_LEFT|SS_NOPREFIX,0);
            UiStyle::TextColor(card->meta,UiStyle::Colors(m_hWnd).muted);
            const wchar_t* headings[]={L"Segments",L"Total flow",L"Status"};
            const wchar_t* values[]={draft.segmentCount.GetString(),draft.totalFlow.GetString(),draft.status.GetString()};
            for(int i=0;i<3;++i)
            {
                CardControl(card->headings[i],headings[i],SS_LEFT|SS_NOPREFIX,0,true);
                CardControl(card->values[i],values[i],SS_LEFT|SS_NOPREFIX,0);
                if(i<2)card->values[i].SetFont(&m_value);
                UiStyle::TextColor(card->headings[i],UiStyle::Colors(m_hWnd).muted);
                UiStyle::TextColor(card->values[i],i==2?
                    (draft.status==L"Analyzed"?UiStyle::Colors(m_hWnd).success:UiStyle::Colors(m_hWnd).amber):UiStyle::Colors(m_hWnd).bright);
            }
            CardControl(card->modify,L"Modify Network",WS_TABSTOP|BS_PUSHBUTTON,8000+static_cast<UINT>(index));
            CardControl(card->divider,L"",SS_ETCHEDHORZ,0);
            // The page paints this separator together with the card border.
            card->divider.ShowWindow(SW_HIDE);
            m_cards.push_back(std::move(card));
        }
        CString count;count.Format(L"%zu of %zu networks",m_cards.size(),NetworkSession::Drafts().size());
        SetDlgItemText(NC_COUNT,count);
        SetDlgItemText(NC_EMPTY,NetworkSession::Drafts().empty()?L"No registered networks yet.\nUse the Registration tab to create your first draft.":L"No networks match your search.");
        GetDlgItem(NC_EMPTY)->ShowWindow(m_cards.empty()?SW_SHOW:SW_HIDE);
        UpdateHeight();Relayout();
    }
    DECLARE_MESSAGE_MAP()
};



class NetworkContent : public CWnd
{
    UiHeading m_header,m_eyebrow;
    UiTabs m_tabs;
    CFont m_headingFont,m_tabFont,m_kickerFont;
    NetworkRegistrationPage m_registration;
    RegisteredNetworksPage m_registered;
    bool m_ready=false;
    int m_active=0;
    int Px(int value) const{return UiStyle::Px(m_hWnd,value);}
    ScrollPage& ActivePage()
    {
        if(m_active==1)return m_registered;
        return m_registration;
    }
    void LayoutHeader()
    {
        if(!m_ready)return;
        CRect client;GetClientRect(&client);const int offset=ActivePage().ScrollPosition();
        HDWP batch=BeginDeferWindowPos(3);
        const auto place=[&](CWnd& window,int y,int height)
        {
            const int width=max(1,client.Width()-Px(56)),pixels=Px(height);
            CRect current;window.GetWindowRect(&current);
            UINT flags=SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCOPYBITS;
            if(current.Width()==width && current.Height()==pixels)flags|=SWP_NOSIZE;
            if(batch)batch=DeferWindowPos(batch,window.m_hWnd,HWND_TOP,Px(28),Px(y)-offset,width,pixels,flags);
        };
        place(m_eyebrow,18,16);place(m_header,44,30);place(m_tabs,84,32);
        if(batch)EndDeferWindowPos(batch);
        m_eyebrow.Invalidate(FALSE);m_header.Invalidate(FALSE);m_tabs.Invalidate(FALSE);
    }
    void LayoutViews()
    {
        if(!m_ready)return;
        CRect client;GetClientRect(&client);
        // The active form owns the whole scroll viewport, including 124 logical
        // pixels before its controls for the Network heading and sub-tab strip.
        for(ScrollPage* page:{static_cast<ScrollPage*>(&m_registration),static_cast<ScrollPage*>(&m_registered)})
            page->SetWindowPos(&CWnd::wndBottom,0,0,max(1,client.Width()),max(1,client.Height()),
                SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCOPYBITS);
        LayoutHeader();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN);
    }
protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(CWnd::OnCreate(cs)==-1)return -1;
        ModifyStyleEx(0,WS_EX_COMPOSITED);
        NetworkSession::InitializeSamples();
        m_headingFont.CreateFont(-Px(22),0,0,0,FW_SEMIBOLD,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_SEMIBOLD));
        m_tabFont.CreateFont(-Px(13),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        m_kickerFont.CreateFont(-Px(10),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        if(!m_eyebrow.Create(L"NETWORK DESIGN",WS_CHILD|WS_VISIBLE|SS_NOPREFIX,CRect(0,0,1,1),this,2398))return -1;
        m_eyebrow.SetFont(&m_kickerFont);m_eyebrow.SetEyebrow();
        if(!m_header.Create(L"Network",WS_CHILD|WS_VISIBLE|SS_NOPREFIX,CRect(0,0,1,1),this,2399))return -1;
        m_header.SetFont(&m_headingFont);m_header.SetPlainTitle();
        if(!m_tabs.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|TCS_TABS,CRect(0,0,1,1),this,2400))return -1;
        m_tabs.SetFont(&m_tabFont);m_tabs.SetSurface(UiStyle::ProjectReferenceStyle.background);m_tabs.SetPadding(CSize(Px(12),Px(6)));m_tabs.InsertItem(0,L"Registration");m_tabs.InsertItem(1,L"Registered Networks");
        if(!m_registration.CreatePage(this,2401) || !m_registered.CreatePage(this,2402))return -1;
        m_registration.SetLeadingSpace(124);m_registered.SetLeadingSpace(124);
        m_ready=true;SelectView(0);return 0;
    }
    afx_msg void OnSize(UINT type,int cx,int cy){CWnd::OnSize(type,cx,cy);LayoutViews();}
    afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect r;GetClientRect(&r);dc->FillSolidRect(r,UiStyle::Colors(m_hWnd).background);return TRUE;}
    afx_msg LRESULT OnPrint(WPARAM wp,LPARAM)
    {
        CDC* dc=CDC::FromHandle(reinterpret_cast<HDC>(wp));CRect client;GetClientRect(&client);
        dc->FillSolidRect(client,UiStyle::Colors(m_hWnd).background);
        const auto print=[&](CWnd& child)
        {
            CRect rect;child.GetWindowRect(&rect);ScreenToClient(&rect);
            const int saved=dc->SaveDC();dc->IntersectClipRect(client);dc->OffsetViewportOrg(rect.left,rect.top);
            dc->IntersectClipRect(0,0,rect.Width(),rect.Height());
            child.SendMessage(WM_PRINT,wp,PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
            dc->RestoreDC(saved);
        };
        // Match the actual sibling z-order when rendering the overlapping scroll header.
        if(m_ready){print(ActivePage());print(m_eyebrow);print(m_header);print(m_tabs);}return 0;
    }
    afx_msg LRESULT OnPageScrolled(WPARAM,LPARAM){LayoutHeader();return 0;}
    afx_msg BOOL OnMouseWheel(UINT flags,short delta,CPoint point)
    {ActivePage().SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(flags,static_cast<WORD>(delta)),MAKELPARAM(point.x,point.y));return TRUE;}
    afx_msg void OnVScroll(UINT code,UINT position,CScrollBar*)
    {ActivePage().SendMessage(WM_VSCROLL,MAKEWPARAM(code,position));}
    afx_msg void OnViewChanged(NMHDR*,LRESULT* result){SelectView(m_tabs.GetCurSel());*result=0;}
public:
    BOOL CreateView(CWnd* parent,UINT id)
    {
        LPCTSTR name=AfxRegisterWndClass(CS_DBLCLKS,::LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);
        return CreateEx(WS_EX_CONTROLPARENT,name,L"Network",WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
            CRect(0,0,1,1),parent,id);
    }
    void SelectView(int index)
    {
        if(!m_ready)return;
        m_active=std::clamp(index,0,1);m_tabs.SetCurSel(m_active);
        m_registration.ShowWindow(m_active==0?SW_SHOW:SW_HIDE);m_registered.ShowWindow(m_active==1?SW_SHOW:SW_HIDE);
        LayoutViews();
    }
    void Refresh(){SelectView(m_active);}
    void RefreshCards(){if(m_ready)m_registered.Rebuild();}
    void EditDraft(size_t index){m_registration.LoadDraft(index);SelectView(0);}
    BOOL PreTranslateMessage(MSG* message) override
    {if(m_ready && ActivePage().PreTranslateMessage(message))return TRUE;return CWnd::PreTranslateMessage(message);}
    DECLARE_MESSAGE_MAP()
};
