#pragma once
#include "autocad/ui/layout/TabbedContent.h"
#include "autocad/ui/model/ProjectInformation.h"
#include <exception>


enum ProjectInfoControl : UINT
{
    PI_TITLE=5500,PI_SUBTITLE,PI_NOTICE,PI_SAVE,PI_EYEBROW,
    PI_FIELD_FIRST=5520,PI_FIELD_LAST=5530,PI_LABEL_FIRST=5540,
    PI_PROJECT_SECTION=5560,PI_CLIENT_SECTION,PI_ORGANIZATION_SECTION,PI_DRAWING_SECTION
};
class ProjectInformationPage : public ScrollPage
{
    bool m_loading=true;
    CFont m_caption,m_kicker;
    void ApplyEditingState()
    {
        for(size_t i=0;i<ProjectInfo::Count;++i)
        {
            auto* edit=static_cast<CEdit*>(GetDlgItem(PI_FIELD_FIRST+static_cast<UINT>(i)));
            edit->SetReadOnly(ProjectInfo::editing?FALSE:TRUE);
            edit->Invalidate(FALSE);
        }
        SetDlgItemText(PI_SAVE,ProjectInfo::editing?L"Save":L"Modify");
        if(!ProjectInfo::editing)
            SetDlgItemText(PI_NOTICE,L"Project information saved for this session. Click Modify to edit.");
        else if(ProjectInfo::draft!=ProjectInfo::saved)
            SetDlgItemText(PI_NOTICE,L"Unsaved changes. Click Save to register project information.");
        else
            SetDlgItemText(PI_NOTICE,L"Edit project information and click Save. All fields are optional.");
    }
protected:
    afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* window,UINT kind)
    {
        HBRUSH brush=ScrollPage::OnCtlColor(dc,window,kind);
        if(window && window->GetDlgCtrlID()>=PI_FIELD_FIRST && window->GetDlgCtrlID()<=PI_FIELD_LAST)
            dc->SetTextColor(ProjectInfo::editing?UiStyle::Colors(m_hWnd).bright:UiStyle::Colors(m_hWnd).muted);
        return brush;
    }
    int ExtraSectionGap(int gap) const override{return 12-gap;}
    void LayoutContent() override
    {
        Sections({{124,460},{480,616},{636,840},{860,1244}});
        const int margin=28,full=PageWidth()-56;
        Place(PI_EYEBROW,margin,18,full,16);
        Place(PI_TITLE,margin,44,full,30);Place(PI_SUBTITLE,margin,84,full,28);
        Place(PI_PROJECT_SECTION,margin,138,full,28);
        Place(PI_CLIENT_SECTION,margin,494,full,28);
        Place(PI_ORGANIZATION_SECTION,margin,650,full,28);
        Place(PI_DRAWING_SECTION,margin,874,full,28);
        const int y[]={184,252,320,388,540,696,764,920,988,1056,1124};
        for(size_t i=0;i<ProjectInfo::Count;++i)
        {
            Place(PI_LABEL_FIRST+static_cast<UINT>(i),margin,y[i],full,18);
            Place(PI_FIELD_FIRST+static_cast<UINT>(i),margin,y[i]+24,full,
                i==ProjectInfo::AdditionalInformation?80:36);
        }
        const int buttonWidth=min(240,full);
        Place(PI_SAVE,(PageWidth()-buttonWidth)/2,1272,buttonWidth,40);
        Place(PI_NOTICE,margin,1330,full,54);
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);
        if(ScrollPage::OnCreate(cs)==-1)return -1;
        const auto font=[&](CFont& target,int size,int weight)
        {
            target.DeleteObject();target.CreateFont(-Px(size),0,0,0,weight,FALSE,FALSE,0,DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(weight));
        };
        font(m_font,13,FW_NORMAL);font(m_sectionFont,13,FW_SEMIBOLD);font(m_title,22,FW_SEMIBOLD);
        font(m_buttonFont,13,FW_SEMIBOLD);font(m_caption,11,FW_NORMAL);font(m_kicker,10,FW_MEDIUM);
        auto& kicker=Label(L"DRAWING PROPERTIES",PI_EYEBROW,true);kicker.SetFont(&m_kicker);
        static_cast<UiHeading*>(&kicker)->SetEyebrow();
        Label(L"Project information",PI_TITLE,true).SetFont(&m_title);
        static_cast<UiHeading*>(GetDlgItem(PI_TITLE))->SetPlainTitle();
        Label(L"Project, client, organization and drawing details.",PI_SUBTITLE);
        UiStyle::TextColor(*GetDlgItem(PI_SUBTITLE),UiStyle::ProjectReferenceStyle.muted);
        Label(L"Project details",PI_PROJECT_SECTION,true);
        Label(L"Client information",PI_CLIENT_SECTION,true);
        Label(L"Organization information",PI_ORGANIZATION_SECTION,true);
        Label(L"Drawing information",PI_DRAWING_SECTION,true);
        const UINT headings[]={PI_PROJECT_SECTION,PI_CLIENT_SECTION,PI_ORGANIZATION_SECTION,PI_DRAWING_SECTION};
        for(int i=0;i<4;++i)static_cast<UiHeading*>(GetDlgItem(headings[i]))->SetNumber(i+1);
        for(size_t i=0;i<ProjectInfo::Count;++i)
        {
            Label(ProjectInfo::Labels[i],PI_LABEL_FIRST+static_cast<UINT>(i));
            const bool notes=i==ProjectInfo::AdditionalInformation;
            const DWORD style=notes?(ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN):ES_AUTOHSCROLL;
            CEdit& edit=Control<CEdit>(L"",style,PI_FIELD_FIRST+static_cast<UINT>(i));
            edit.SetLimitText(notes?4000:(i==ProjectInfo::ProjectAddress?512:200));
            edit.SetWindowText(ProjectInfo::draft[i]);
            if(!notes)static_cast<UiEdit*>(&edit)->SetPlaceholder(ProjectInfo::Placeholders[i]);
        }
        Control<CButton>(L"Save",BS_PUSHBUTTON,PI_SAVE);
        static_cast<UiButton*>(GetDlgItem(PI_SAVE))->SetPrimary(true);
        Label(L"",PI_NOTICE);
        GetDlgItem(PI_NOTICE)->SetFont(&m_caption);
        UiStyle::TextColor(*GetDlgItem(PI_NOTICE),UiStyle::ProjectReferenceStyle.muted);
        ApplyEditingState();
        m_loading=false;m_contentHeight=1412;m_layoutReady=true;Relayout();return 0;
    }
    BOOL OnCommand(WPARAM wp,LPARAM lp) override
    {
        const UINT id=LOWORD(wp),code=HIWORD(wp);
        if(!m_loading && ProjectInfo::editing && code==EN_CHANGE && id>=PI_FIELD_FIRST && id<=PI_FIELD_LAST)
        {
            GetDlgItemText(id,ProjectInfo::draft[id-PI_FIELD_FIRST]);
            SetDlgItemText(PI_NOTICE,L"Unsaved changes. Click Save to register project information.");
            return TRUE;
        }
        if(code==BN_CLICKED && id==PI_SAVE)
        {
            if(!ProjectInfo::editing)
            {
                ProjectInfo::editing=true;
                ApplyEditingState();
                return TRUE;
            }
            try
            {
                for(size_t i=0;i<ProjectInfo::Count;++i)
                    GetDlgItemText(PI_FIELD_FIRST+static_cast<UINT>(i),ProjectInfo::draft[i]);
                ProjectInfo::RegisterProjectInformation();
                ApplyEditingState();
            }
            catch(CException* error)
            {
                error->Delete();
                SetDlgItemText(PI_NOTICE,L"Project information could not be saved. Please try again.");
            }
            catch(const std::exception&)
            {
                SetDlgItemText(PI_NOTICE,L"Project information could not be saved. Please try again.");
            }
            return TRUE;
        }
        return ScrollPage::OnCommand(wp,lp);
    }
    DECLARE_MESSAGE_MAP()
};
