#pragma once
#include "autocad/ui/layout/TabbedContent.h"
#include "autocad/ui/model/NetworkSession.h"
#include <array>
#include <cmath>
#include <cwchar>

#include <cmath>
#include <cwchar>

// Preview-only segment editor. No entity selection or drawing writes yet.
enum PropertyControl : UINT
{
    PC_TITLE=3100,PC_SUBTITLE,PC_SEGMENT_LABEL,PC_SEGMENT,PC_PICK,
    PC_NETWORK_LABEL,PC_NETWORK,PC_FLOW_LABEL,PC_FLOW,PC_WIDTH_LABEL,PC_WIDTH,
    PC_HEIGHT_LABEL,PC_HEIGHT,PC_APPLY,PC_STATUS,PC_SELECTION_SECTION,PC_DATA_SECTION
};
class SegmentPropertiesPage : public ScrollPage
{
    void SelectSize(UINT id,LPCTSTR value)
    {
        auto* combo=static_cast<CComboBox*>(GetDlgItem(id));
        for(int size=100;size<=2000;size+=50)
        {CString text;text.Format(L"%d",size);combo->AddString(text);}
        combo->SetCurSel(combo->FindStringExact(-1,value));
    }
    void Status(LPCTSTR text){SetDlgItemText(PC_STATUS,text);}
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        Sections({{124,352},{364,680}});
        const int width=PageWidth(),margin=28,full=width-56;
        Place(PC_TITLE,margin,20,full,44);Place(PC_SUBTITLE,margin,76,full,42);
        Place(PC_SELECTION_SECTION,margin,138,full,28);
        Place(PC_SEGMENT_LABEL,margin,182,full,24);
        Place(PC_SEGMENT,margin,214,full-88,36);Place(PC_PICK,width-margin-76,214,76,36);
        Place(PC_NETWORK_LABEL,margin,270,full,24);Place(PC_NETWORK,margin,302,full,36);
        Place(PC_DATA_SECTION,margin,378,full,28);
        Place(PC_FLOW_LABEL,margin,422,full,24);Place(PC_FLOW,margin,454,full,36);
        Place(PC_WIDTH_LABEL,margin,510,full,24);Place(PC_WIDTH,margin,542,full,240);
        Place(PC_HEIGHT_LABEL,margin,598,full,24);Place(PC_HEIGHT,margin,630,full,240);
        const int buttonWidth=min(240,full);
        Place(PC_APPLY,(width-buttonWidth)/2,708,buttonWidth,40);Place(PC_STATUS,margin,770,full,60);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(L"Selection",PC_SELECTION_SECTION,true);Label(L"Properties",PC_DATA_SECTION,true);
        static_cast<UiHeading*>(GetDlgItem(PC_SELECTION_SECTION))->SetNumber(1);
        static_cast<UiHeading*>(GetDlgItem(PC_DATA_SECTION))->SetNumber(2);
        Label(L"Segment properties",PC_TITLE,true).SetFont(&m_title);
        static_cast<UiHeading*>(GetDlgItem(PC_TITLE))->SetPlainTitle();
        Label(L"Adjust flow and dimensions for the selected duct segment.",PC_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(PC_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(L"Selected segment:",PC_SEGMENT_LABEL);
        Control<CEdit>(L"",ES_READONLY|ES_AUTOHSCROLL,PC_SEGMENT).SetWindowText(L"DUCT-014");
        Control<CButton>(L"Pick",BS_PUSHBUTTON,PC_PICK);
        Label(L"Network name:",PC_NETWORK_LABEL);
        Control<CEdit>(L"",ES_READONLY|ES_AUTOHSCROLL,PC_NETWORK).SetWindowText(L"Supply Network 01");
        Label(L"Flow (CFM):",PC_FLOW_LABEL);
        CEdit& flow=Control<CEdit>(L"",ES_AUTOHSCROLL,PC_FLOW);
        flow.SetLimitText(16);flow.SetWindowText(L"350");
        Label(L"Width (mm):",PC_WIDTH_LABEL);
        Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,PC_WIDTH);SelectSize(PC_WIDTH,L"400");
        Label(L"Height (mm):",PC_HEIGHT_LABEL);
        Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,PC_HEIGHT);SelectSize(PC_HEIGHT,L"250");
        Control<CButton>(L"Apply Changes",BS_PUSHBUTTON,PC_APPLY);
        static_cast<UiButton*>(GetDlgItem(PC_APPLY))->SetPrimary(true);
        Label(L"Sample properties for UI preview. Drawing selection and updates are not connected yet.",PC_STATUS);
        UiStyle::TextColor(*GetDlgItem(PC_STATUS),UiStyle::ProjectReferenceStyle.muted);
        m_contentHeight=856;m_layoutReady=true;Relayout();return 0;
    }
    afx_msg BOOL OnEraseBkgnd(CDC* dc)
    {
        ScrollPage::OnEraseBkgnd(dc);
        return TRUE;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if(code==BN_CLICKED && id==PC_PICK)
        {
            Status(L"Pick will select a duct segment when drawing selection is connected.");return TRUE;
        }
        if(code==BN_CLICKED && id==PC_APPLY)
        {
            CString flow;GetDlgItemText(PC_FLOW,flow);flow.Trim();
            wchar_t* end=nullptr;const double value=std::wcstod(flow.GetString(),&end);
            if(flow.IsEmpty() || end==flow.GetString() || *end!=L'\0' || !std::isfinite(value) || value<0)
            {
                Status(L"Enter a valid non-negative flow in CFM.");
                GetDlgItem(PC_FLOW)->SetFocus();EnsureVisible(GetDlgItem(PC_FLOW));return TRUE;
            }
            CString width,height;GetDlgItemText(PC_WIDTH,width);GetDlgItemText(PC_HEIGHT,height);
            CString message;message.Format(L"Preview updated: %s CFM | %s x %s mm. Drawing unchanged.",
                flow.GetString(),width.GetString(),height.GetString());Status(message);return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
    DECLARE_MESSAGE_MAP()
};



// Sample equipment editors; actions update the preview only.
enum EquipmentPropertyControl : UINT
{
    EP_TITLE=3400,EP_SELECTED_LABEL,EP_SELECTED,EP_PICK,EP_NETWORK_LABEL,EP_NETWORK,
    EP_FIRST_LABEL,EP_FIRST,EP_SECOND_LABEL,EP_SECOND,EP_THIRD_LABEL,EP_THIRD,
    EP_FOURTH_LABEL,EP_FOURTH,EP_APPLY,EP_STATUS,EP_SUBTITLE,EP_SELECTION_SECTION,EP_DATA_SECTION
};
class EquipmentPropertiesPage : public ScrollPage
{
    bool m_damper;
    void SizeOptions(UINT id,LPCTSTR selected)
    {
        auto* combo=static_cast<CComboBox*>(GetDlgItem(id));
        for(auto text:{L"150 x 150",L"200 x 200",L"250 x 250",L"300 x 300",L"400 x 400",L"600 x 600"})combo->AddString(text);
        combo->SetCurSel(combo->FindStringExact(-1,selected));
    }
    bool Number(UINT id,bool whole,bool positive)
    {
        CString text;GetDlgItemText(id,text);text.Trim();
        wchar_t* end=nullptr;double value=std::wcstod(text.GetString(),&end);
        if(text.IsEmpty() || end==text.GetString() || *end!=L'\0' || !std::isfinite(value) ||
            (positive?value<=0:value<0) || (whole && std::floor(value)!=value))
        {
            SetDlgItemText(EP_STATUS,whole?L"Enter positive whole-number dimensions in millimeters.":L"Enter a valid non-negative flow in CFM.");
            GetDlgItem(id)->SetFocus();EnsureVisible(GetDlgItem(id));return false;
        }
        return true;
    }
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        Sections({{124,352},{364,m_damper?768:680}});
        const int width=PageWidth(),margin=28,full=width-56;
        Place(EP_TITLE,margin,20,full,44);Place(EP_SUBTITLE,margin,76,full,42);
        Place(EP_SELECTION_SECTION,margin,138,full,28);
        Place(EP_SELECTED_LABEL,margin,182,full,24);
        Place(EP_SELECTED,margin,214,full-88,36);Place(EP_PICK,width-margin-76,214,76,36);
        Place(EP_NETWORK_LABEL,margin,270,full,24);Place(EP_NETWORK,margin,302,full,36);
        Place(EP_DATA_SECTION,margin,378,full,28);
        const UINT labels[]={EP_FIRST_LABEL,EP_SECOND_LABEL,EP_THIRD_LABEL,EP_FOURTH_LABEL};
        const UINT fields[]={EP_FIRST,EP_SECOND,EP_THIRD,EP_FOURTH};
        for(int i=0;i<(m_damper?4:3);++i)
        {
            Place(labels[i],margin,422+i*88,full,24);
            Place(fields[i],margin,454+i*88,full,((m_damper && i<2) || (!m_damper && i>0))?240:36);
        }
        const int buttonWidth=min(240,full),buttonY=m_damper?796:708;
        Place(EP_APPLY,(width-buttonWidth)/2,buttonY,buttonWidth,40);Place(EP_STATUS,margin,buttonY+62,full,60);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(L"Selection",EP_SELECTION_SECTION,true);Label(L"Properties",EP_DATA_SECTION,true);
        static_cast<UiHeading*>(GetDlgItem(EP_SELECTION_SECTION))->SetNumber(1);
        static_cast<UiHeading*>(GetDlgItem(EP_DATA_SECTION))->SetNumber(2);
        Label(m_damper?L"Damper properties":L"Air terminal properties",EP_TITLE,true).SetFont(&m_title);
        static_cast<UiHeading*>(GetDlgItem(EP_TITLE))->SetPlainTitle();
        Label(m_damper?L"Review damper details and adjust dimensions for the selected damper.":
            L"Adjust flow and sizes for the selected air terminal.",EP_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(EP_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(m_damper?L"Selected damper:":L"Selected air terminal:",EP_SELECTED_LABEL);
        Control<CEdit>(L"",ES_READONLY|ES_AUTOHSCROLL,EP_SELECTED).SetWindowText(m_damper?L"VCD-003":L"AT-003");
        Control<CButton>(L"Pick",BS_PUSHBUTTON,EP_PICK);
        Label(L"Network name:",EP_NETWORK_LABEL);
        Control<CEdit>(L"",ES_READONLY|ES_AUTOHSCROLL,EP_NETWORK).SetWindowText(L"Supply Network 01");
        if(m_damper)
        {
            Label(L"Damper type:",EP_FIRST_LABEL);
            auto& types=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,EP_FIRST);
            for(auto value:{L"Volume Control Damper",L"Fire Damper",L"Smoke Damper",L"Fire/Smoke Damper",L"Backdraft Damper"})types.AddString(value);
            types.SetCurSel(0);
            Label(L"Blade arrangement:",EP_SECOND_LABEL);
            auto& blades=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,EP_SECOND);
            blades.AddString(L"Opposed Blade");blades.AddString(L"Parallel Blade");blades.SetCurSel(0);
            Label(L"Width (mm):",EP_THIRD_LABEL);
            Control<CEdit>(L"",ES_AUTOHSCROLL,EP_THIRD).SetWindowText(L"400");
            Label(L"Height (mm):",EP_FOURTH_LABEL);
            Control<CEdit>(L"",ES_AUTOHSCROLL,EP_FOURTH).SetWindowText(L"250");
            static_cast<CEdit*>(GetDlgItem(EP_THIRD))->SetLimitText(8);
            static_cast<CEdit*>(GetDlgItem(EP_FOURTH))->SetLimitText(8);
        }
        else
        {
            Label(L"Flow (CFM):",EP_FIRST_LABEL);
            Control<CEdit>(L"",ES_AUTOHSCROLL,EP_FIRST).SetWindowText(L"250");
            static_cast<CEdit*>(GetDlgItem(EP_FIRST))->SetLimitText(16);
            Label(L"Neck size (mm):",EP_SECOND_LABEL);
            Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,EP_SECOND);SizeOptions(EP_SECOND,L"200 x 200");
            Label(L"Face size (mm):",EP_THIRD_LABEL);
            Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,EP_THIRD);SizeOptions(EP_THIRD,L"300 x 300");
        }
        Control<CButton>(L"Apply Changes",BS_PUSHBUTTON,EP_APPLY);
        static_cast<UiButton*>(GetDlgItem(EP_APPLY))->SetPrimary(true);
        Label(L"Sample properties for UI preview. Drawing selection and updates are not connected yet.",EP_STATUS);
        UiStyle::TextColor(*GetDlgItem(EP_STATUS),UiStyle::ProjectReferenceStyle.muted);
        m_contentHeight=m_damper?944:856;m_layoutReady=true;Relayout();return 0;
    }
    afx_msg BOOL OnEraseBkgnd(CDC* dc)
    {
        ScrollPage::OnEraseBkgnd(dc);
        return TRUE;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if(code==BN_CLICKED && id==EP_PICK)
        {
            SetDlgItemText(EP_STATUS,m_damper?L"Pick will select a damper when drawing selection is connected.":
                L"Pick will select an air terminal when drawing selection is connected.");return TRUE;
        }
        if(code==BN_CLICKED && id==EP_APPLY)
        {
            if(m_damper?(!Number(EP_THIRD,true,true) || !Number(EP_FOURTH,true,true)):!Number(EP_FIRST,false,false))return TRUE;
            CString first,second,third;GetDlgItemText(m_damper?EP_THIRD:EP_FIRST,first);
            GetDlgItemText(m_damper?EP_FOURTH:EP_SECOND,second);GetDlgItemText(EP_THIRD,third);
            CString message;
            if(m_damper)message.Format(L"Preview updated: %s x %s mm. Drawing unchanged.",first.GetString(),second.GetString());
            else message.Format(L"Preview updated: %s CFM | Neck %s mm | Face %s mm. Drawing unchanged.",first.GetString(),second.GetString(),third.GetString());
            SetDlgItemText(EP_STATUS,message);return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
    DECLARE_MESSAGE_MAP()
public:
    explicit EquipmentPropertiesPage(bool damper):m_damper(damper){}
};


class PropertiesContent : public TabbedContent
{
    SegmentPropertiesPage m_system;
    EquipmentPropertiesPage m_summary{false},m_export{true};
protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        if(CWnd::OnCreate(cs)==-1)return -1;
        if(!CreateHeader(L"Properties",L"ELEMENT PROPERTIES",3700,3701,3699))return -1;
        if(!m_system.CreatePage(this,3801) || !m_summary.CreatePage(this,3802) || !m_export.CreatePage(this,3803))return -1;
        AttachPages({&m_system,&m_summary,&m_export},{L"Segments",L"Air Terminals",L"Dampers"});return 0;
    }
public:
    BOOL CreateView(CWnd* parent,UINT id){return TabbedContent::CreateView(parent,id,L"Properties");}
    DECLARE_MESSAGE_MAP()
};
