#pragma once
#include "autocad/ui/layout/TabbedContent.h"
#include "autocad/ui/model/NetworkSession.h"
#include <array>
#include <cmath>
#include <cwchar>

#include <array>

// UI preview only. These selections do not generate reports or write files.
namespace ReportPreview
{
    inline constexpr const wchar_t* SystemSheets[]={L"Duct Weight Sheet",L"Duct Fitting Weight Sheet",
        L"Pressure Loss - Format 1",L"Pressure Loss - Format 2",L"BOQ"};
    inline constexpr const wchar_t* SummarySheets[]={L"Combined BOQ",L"Duct Weight Summary",
        L"Fitting Weight Summary",L"Pressure Loss Summary"};
    inline std::array<bool,5> systemSheets={true,true,true,false,true};
    inline std::array<bool,4> summarySheets={true,true,true,true};
    inline int systemId=1,format=0,scope=2;
    inline constexpr const wchar_t* Formats[]={L"Excel (.xlsx)",L"PDF (.pdf)",L"CSV (.csv)"};
    inline constexpr const wchar_t* Scopes[]={L"System reports",L"Project summary",L"System reports and summary"};
    inline int SystemCount(){return static_cast<int>(std::count(systemSheets.begin(),systemSheets.end(),true));}
    inline int SummaryCount(){return static_cast<int>(std::count(summarySheets.begin(),summarySheets.end(),true));}
    inline CString SystemName()
    {
        for(const auto& draft:NetworkSession::Drafts())if(draft.id==systemId)return draft.name;
        return L"No registered system";
    }
    inline void FillSystems(CComboBox& combo)
    {
        combo.ResetContent();int selected=-1;
        for(const auto& draft:NetworkSession::Drafts())
        {
            int row=combo.AddString(draft.name);
            if(row>=0){combo.SetItemData(row,static_cast<DWORD_PTR>(draft.id));if(draft.id==systemId)selected=row;}
        }
        if(selected<0 && combo.GetCount()>0){selected=0;systemId=static_cast<int>(combo.GetItemData(0));}
        combo.SetCurSel(selected);
    }
    inline void SelectSystem(CComboBox& combo)
    {
        const int row=combo.GetCurSel();if(row>=0)systemId=static_cast<int>(combo.GetItemData(row));
    }
}
enum ReportsControl : UINT
{
    RC_SYSTEM_TITLE=4100,RC_SYSTEM_SUBTITLE,RC_SYSTEM_LABEL,RC_SYSTEM,
    RC_SYSTEM_FIRST=4120,RC_SYSTEM_LAST=4124,RC_SYSTEM_PREVIEW=4140,RC_SYSTEM_STATUS,RC_SYSTEM_NOTICE,
    RC_SUMMARY_TITLE=4300,RC_SUMMARY_SUBTITLE,RC_SUMMARY_FIRST=4320,RC_SUMMARY_LAST=4323,
    RC_SUMMARY_PREVIEW=4340,RC_SUMMARY_STATUS,RC_SUMMARY_NOTICE,
    RC_EXPORT_TITLE=4400,RC_EXPORT_SUBTITLE,RC_FORMAT_LABEL,RC_FORMAT,RC_SCOPE_LABEL,RC_SCOPE,
    RC_EXPORT_SYSTEM_LABEL,RC_EXPORT_SYSTEM,RC_EXPORT_PREVIEW,RC_EXPORT_STATUS,RC_EXPORT_NOTICE
};
class SystemReportsPage : public ScrollPage
{
    void UpdateStatus()
    {
        CString text;text.Format(L"%d report types selected for %s.",ReportPreview::SystemCount(),
            ReportPreview::SystemName().GetString());SetDlgItemText(RC_SYSTEM_STATUS,text);
    }
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        Sections({{8,410}});

        const int w=PageWidth(),margin=28,full=w-56;
        Place(RC_SYSTEM_TITLE,margin,20,full,28);Place(RC_SYSTEM_SUBTITLE,margin,64,full,40);
        Place(RC_SYSTEM_LABEL,margin,120,72,24);Place(RC_SYSTEM,108,114,w-136,240);
        for(int i=0;i<5;++i)Place(RC_SYSTEM_FIRST+i,margin,184+44*i,full,30);
        Place(RC_SYSTEM_PREVIEW,margin,428,full,36);
        Place(RC_SYSTEM_STATUS,margin,488,full,64);Place(RC_SYSTEM_NOTICE,margin,568,full,56);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(L"System reports",RC_SYSTEM_TITLE,true);
        static_cast<UiHeading*>(GetDlgItem(RC_SYSTEM_TITLE))->SetNumber(1);
        Label(L"Choose the sheets to include for this system.",RC_SYSTEM_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(RC_SYSTEM_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(L"System:",RC_SYSTEM_LABEL);
        auto& system=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,RC_SYSTEM);
        ReportPreview::FillSystems(system);
        for(int i=0;i<5;++i)
        {
            Control<CButton>(ReportPreview::SystemSheets[i],BS_AUTOCHECKBOX,RC_SYSTEM_FIRST+i);
            CheckDlgButton(RC_SYSTEM_FIRST+i,ReportPreview::systemSheets[i]?BST_CHECKED:BST_UNCHECKED);
        }
        Control<CButton>(L"Preview reports",BS_PUSHBUTTON,RC_SYSTEM_PREVIEW);
        static_cast<UiButton*>(GetDlgItem(RC_SYSTEM_PREVIEW))->SetPrimary(true);
        Label(L"",RC_SYSTEM_STATUS);
        Label(L"UI preview only. Report calculations and document generation are not connected yet.",RC_SYSTEM_NOTICE);
        UiStyle::TextColor(*GetDlgItem(RC_SYSTEM_NOTICE),UiStyle::ProjectReferenceStyle.muted);
        UpdateStatus();m_contentHeight=644;m_layoutReady=true;Relayout();return 0;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if(id==RC_SYSTEM && code==CBN_SELCHANGE)
        {ReportPreview::SelectSystem(*static_cast<CComboBox*>(GetDlgItem(RC_SYSTEM)));UpdateStatus();return TRUE;}
        if(code==BN_CLICKED && id>=RC_SYSTEM_FIRST && id<=RC_SYSTEM_LAST)
        {ReportPreview::systemSheets[id-RC_SYSTEM_FIRST]=IsDlgButtonChecked(id)==BST_CHECKED;UpdateStatus();return TRUE;}
        if(id==RC_SYSTEM_PREVIEW && code==BN_CLICKED)
        {
            if(ReportPreview::SystemCount()==0){SetDlgItemText(RC_SYSTEM_STATUS,L"Select at least one report type.");return TRUE;}
            if(static_cast<CComboBox*>(GetDlgItem(RC_SYSTEM))->GetCurSel()<0)
            {SetDlgItemText(RC_SYSTEM_STATUS,L"Register a network before preparing system reports.");return TRUE;}
            CString text;text.Format(L"Preview: %d report types for %s. No report files have been generated.",
                ReportPreview::SystemCount(),ReportPreview::SystemName().GetString());
            SetDlgItemText(RC_SYSTEM_STATUS,text);return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
public:
    void RefreshSystems()
    {
        if(m_layoutReady){ReportPreview::FillSystems(*static_cast<CComboBox*>(GetDlgItem(RC_SYSTEM)));UpdateStatus();}
    }
    DECLARE_MESSAGE_MAP()
};


class ProjectSummaryPage : public ScrollPage
{
    void UpdateStatus()
    {
        CString text;text.Format(L"%d summary sheets selected across all registered networks.",ReportPreview::SummaryCount());
        SetDlgItemText(RC_SUMMARY_STATUS,text);
    }
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        Sections({{8,320}});

        const int margin=28,full=PageWidth()-56;
        Place(RC_SUMMARY_TITLE,margin,20,full,28);Place(RC_SUMMARY_SUBTITLE,margin,64,full,44);
        for(int i=0;i<4;++i)Place(RC_SUMMARY_FIRST+i,margin,132+44*i,full,30);
        Place(RC_SUMMARY_PREVIEW,margin,340,full,36);
        Place(RC_SUMMARY_STATUS,margin,400,full,64);Place(RC_SUMMARY_NOTICE,margin,480,full,56);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(L"Project summary",RC_SUMMARY_TITLE,true);
        static_cast<UiHeading*>(GetDlgItem(RC_SUMMARY_TITLE))->SetNumber(1);
        Label(L"Choose the summaries to combine across all networks.",RC_SUMMARY_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(RC_SUMMARY_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        for(int i=0;i<4;++i)
        {
            Control<CButton>(ReportPreview::SummarySheets[i],BS_AUTOCHECKBOX,RC_SUMMARY_FIRST+i);
            CheckDlgButton(RC_SUMMARY_FIRST+i,ReportPreview::summarySheets[i]?BST_CHECKED:BST_UNCHECKED);
        }
        Control<CButton>(L"Preview summary",BS_PUSHBUTTON,RC_SUMMARY_PREVIEW);
        static_cast<UiButton*>(GetDlgItem(RC_SUMMARY_PREVIEW))->SetPrimary(true);
        Label(L"",RC_SUMMARY_STATUS);
        Label(L"UI preview only. Combined quantities, weights and pressure losses are not calculated yet.",RC_SUMMARY_NOTICE);
        UiStyle::TextColor(*GetDlgItem(RC_SUMMARY_NOTICE),UiStyle::ProjectReferenceStyle.muted);
        UpdateStatus();m_contentHeight=556;m_layoutReady=true;Relayout();return 0;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if(code==BN_CLICKED && id>=RC_SUMMARY_FIRST && id<=RC_SUMMARY_LAST)
        {ReportPreview::summarySheets[id-RC_SUMMARY_FIRST]=IsDlgButtonChecked(id)==BST_CHECKED;UpdateStatus();return TRUE;}
        if(id==RC_SUMMARY_PREVIEW && code==BN_CLICKED)
        {
            if(ReportPreview::SummaryCount()==0){SetDlgItemText(RC_SUMMARY_STATUS,L"Select at least one summary sheet.");return TRUE;}
            CString text;text.Format(L"Preview: %d project summary sheets selected. No summary files have been generated.",
                ReportPreview::SummaryCount());SetDlgItemText(RC_SUMMARY_STATUS,text);return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
    DECLARE_MESSAGE_MAP()
};


class ReportExportPage : public ScrollPage
{
    void UpdateStatus(bool preview=false)
    {
        CString text;
        const int systems=ReportPreview::scope==1?0:ReportPreview::SystemCount();
        const int summaries=ReportPreview::scope==0?0:ReportPreview::SummaryCount();
        text.Format(L"%s: %s.\n%d system report types and %d project summary sheets selected.\nNo files have been written.",
            preview?L"Export preview":L"Selection",ReportPreview::Formats[ReportPreview::format],systems,summaries);
        SetDlgItemText(RC_EXPORT_STATUS,text);
    }
protected:
    int ExtraSectionGap(int gap) const override{return gap>0 && gap<=64?12-gap:0;}
    void LayoutContent() override
    {
        Sections({{8,390}});

        const int margin=28,full=PageWidth()-56;
        Place(RC_EXPORT_TITLE,margin,20,full,28);Place(RC_EXPORT_SUBTITLE,margin,64,full,44);
        Place(RC_FORMAT_LABEL,margin,126,full,24);Place(RC_FORMAT,margin,158,full,240);
        Place(RC_SCOPE_LABEL,margin,214,full,24);Place(RC_SCOPE,margin,246,full,240);
        Place(RC_EXPORT_SYSTEM_LABEL,margin,302,full,24);Place(RC_EXPORT_SYSTEM,margin,334,full,240);
        Place(RC_EXPORT_PREVIEW,margin,408,full,36);Place(RC_EXPORT_STATUS,margin,468,full,110);
        Place(RC_EXPORT_NOTICE,margin,594,full,58);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        ReferenceTypography();
        Label(L"Export",RC_EXPORT_TITLE,true);
        static_cast<UiHeading*>(GetDlgItem(RC_EXPORT_TITLE))->SetNumber(1);
        Label(L"Choose an output format for the selected report sheets.",RC_EXPORT_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(RC_EXPORT_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(L"File format:",RC_FORMAT_LABEL);
        auto& format=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,RC_FORMAT);
        for(auto value:ReportPreview::Formats)format.AddString(value);format.SetCurSel(ReportPreview::format);
        Label(L"Include:",RC_SCOPE_LABEL);
        auto& scope=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,RC_SCOPE);
        for(auto value:ReportPreview::Scopes)scope.AddString(value);scope.SetCurSel(ReportPreview::scope);
        Label(L"System:",RC_EXPORT_SYSTEM_LABEL);
        auto& system=Control<CComboBox>(L"",CBS_DROPDOWNLIST|WS_VSCROLL,RC_EXPORT_SYSTEM);
        ReportPreview::FillSystems(system);
        Control<CButton>(L"Preview export",BS_PUSHBUTTON,RC_EXPORT_PREVIEW);
        static_cast<UiButton*>(GetDlgItem(RC_EXPORT_PREVIEW))->SetPrimary(true);
        Label(L"",RC_EXPORT_STATUS);
        Label(L"UI preview only. File export is not connected. Summary sheets cover all registered networks.",RC_EXPORT_NOTICE);
        UiStyle::TextColor(*GetDlgItem(RC_EXPORT_NOTICE),UiStyle::ProjectReferenceStyle.muted);
        UpdateStatus();m_contentHeight=672;m_layoutReady=true;Relayout();return 0;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if(code==CBN_SELCHANGE)
        {
            if(id==RC_FORMAT)ReportPreview::format=static_cast<CComboBox*>(GetDlgItem(id))->GetCurSel();
            else if(id==RC_SCOPE)ReportPreview::scope=static_cast<CComboBox*>(GetDlgItem(id))->GetCurSel();
            else if(id==RC_EXPORT_SYSTEM)ReportPreview::SelectSystem(*static_cast<CComboBox*>(GetDlgItem(id)));
            else return ScrollPage::OnCommand(wp,lp);
            UpdateStatus();return TRUE;
        }
        if(id==RC_EXPORT_PREVIEW && code==BN_CLICKED)
        {
            const bool system=ReportPreview::scope!=1,summary=ReportPreview::scope!=0;
            if((system && ReportPreview::SystemCount()==0) || (summary && ReportPreview::SummaryCount()==0))
            {SetDlgItemText(RC_EXPORT_STATUS,L"Select the sheets to include on the System Reports and Summary tabs.");return TRUE;}
            if(system && static_cast<CComboBox*>(GetDlgItem(RC_EXPORT_SYSTEM))->GetCurSel()<0)
            {SetDlgItemText(RC_EXPORT_STATUS,L"Register a network before preparing system reports.");return TRUE;}
            UpdateStatus(true);return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
public:
    void RefreshSystems()
    {if(m_layoutReady){ReportPreview::FillSystems(*static_cast<CComboBox*>(GetDlgItem(RC_EXPORT_SYSTEM)));UpdateStatus();}}
    DECLARE_MESSAGE_MAP()
};


// Plain MFC container so tab switching can be verified outside AutoCAD too.
class ReportsContent : public TabbedContent
{
    SystemReportsPage m_system;
    ProjectSummaryPage m_summary;
    ReportExportPage m_export;
protected:
    void PageActivated(int index) override{if(index==0)m_system.RefreshSystems();else if(index==2)m_export.RefreshSystems();}
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        if(CWnd::OnCreate(cs)==-1)return -1;
        if(!CreateHeader(L"BOQ & Reports",L"PROJECT DELIVERABLES",4700,4701,4699))return -1;
        if(!m_system.CreatePage(this,4801) || !m_summary.CreatePage(this,4802) || !m_export.CreatePage(this,4803))return -1;
        AttachPages({&m_system,&m_summary,&m_export},{L"System Reports",L"Summary",L"Export"});return 0;
    }
public:
    BOOL CreateView(CWnd* parent,UINT id){return TabbedContent::CreateView(parent,id,L"BOQ & Reports");}
    DECLARE_MESSAGE_MAP()
};
