#pragma once
#include "autocad/ui/layout/ScrollPage.h"

// Shared visual/navigation host for the approved reference styling. Pages stay
// alive when switching tabs, and their existing commands remain in the pages.
class TabbedContent : public CWnd
{
    UiHeading m_header,m_eyebrow;
    UiTabs m_tabs;
    CFont m_headingFont,m_tabFont,m_kickerFont;
    std::vector<ScrollPage*> m_pages;
    UINT m_tabId=0;
    bool m_ready=false;
    int m_active=0;
    int Px(int value) const{return UiStyle::Px(m_hWnd,value);}
    ScrollPage& ActivePage(){return *m_pages[m_active];}
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
        for(auto* page:m_pages)page->SetWindowPos(&CWnd::wndBottom,0,0,max(1,client.Width()),max(1,client.Height()),
            SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCOPYBITS);
        LayoutHeader();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN);
    }
protected:
    BOOL CreateView(CWnd* parent,UINT id,LPCTSTR title)
    {
        LPCTSTR name=AfxRegisterWndClass(CS_DBLCLKS,::LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);
        return CreateEx(WS_EX_CONTROLPARENT,name,title,WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,CRect(0,0,1,1),parent,id);
    }
    bool CreateHeader(LPCTSTR title,LPCTSTR eyebrow,UINT headerId,UINT tabId,UINT eyebrowId)
    {
        UiStyle::SetVisualStyle(*this,UiStyle::ProjectReferenceStyle);ModifyStyleEx(0,WS_EX_COMPOSITED);m_tabId=tabId;
        m_headingFont.CreateFont(-Px(22),0,0,0,FW_SEMIBOLD,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_SEMIBOLD));
        m_tabFont.CreateFont(-Px(13),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        m_kickerFont.CreateFont(-Px(10),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        if(!m_eyebrow.Create(eyebrow,WS_CHILD|WS_VISIBLE|SS_NOPREFIX,CRect(0,0,1,1),this,eyebrowId))return false;
        m_eyebrow.SetFont(&m_kickerFont);m_eyebrow.SetEyebrow();
        if(!m_header.Create(title,WS_CHILD|WS_VISIBLE|SS_NOPREFIX,CRect(0,0,1,1),this,headerId))return false;
        m_header.SetFont(&m_headingFont);m_header.SetPlainTitle();
        if(!m_tabs.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|TCS_TABS,CRect(0,0,1,1),this,tabId))return false;
        m_tabs.SetFont(&m_tabFont);m_tabs.SetSurface(UiStyle::ProjectReferenceStyle.background);return true;
    }
    void AttachPages(std::initializer_list<ScrollPage*> pages,std::initializer_list<LPCTSTR> names)
    {
        m_pages.assign(pages);int index=0;for(auto name:names)m_tabs.InsertItem(index++,name);
        for(auto* page:m_pages)page->SetLeadingSpace(124);
        m_ready=true;SelectView(0);
    }
    virtual void PageActivated(int){}
    afx_msg void OnSize(UINT type,int cx,int cy){CWnd::OnSize(type,cx,cy);LayoutViews();}
    afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect r;GetClientRect(&r);dc->FillSolidRect(r,UiStyle::Colors(m_hWnd).background);return TRUE;}
    afx_msg LRESULT OnPageScrolled(WPARAM,LPARAM){LayoutHeader();return 0;}
    afx_msg BOOL OnMouseWheel(UINT flags,short delta,CPoint point)
    {if(m_ready)ActivePage().SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(flags,static_cast<WORD>(delta)),MAKELPARAM(point.x,point.y));return TRUE;}
    afx_msg void OnVScroll(UINT code,UINT position,CScrollBar*){if(m_ready)ActivePage().SendMessage(WM_VSCROLL,MAKEWPARAM(code,position));}
    BOOL OnNotify(WPARAM wp,LPARAM lp,LRESULT* result) override
    {
        const auto* note=reinterpret_cast<NMHDR*>(lp);
        if(note && note->idFrom==m_tabId && note->code==TCN_SELCHANGE){SelectView(m_tabs.GetCurSel());*result=0;return TRUE;}
        return CWnd::OnNotify(wp,lp,result);
    }
    afx_msg LRESULT OnPrint(WPARAM wp,LPARAM)
    {
        CDC* dc=CDC::FromHandle(reinterpret_cast<HDC>(wp));CRect client;GetClientRect(&client);
        dc->FillSolidRect(client,UiStyle::Colors(m_hWnd).background);
        const auto print=[&](CWnd& child)
        {
            CRect rect;child.GetWindowRect(&rect);ScreenToClient(&rect);int saved=dc->SaveDC();
            dc->IntersectClipRect(client);dc->OffsetViewportOrg(rect.left,rect.top);dc->IntersectClipRect(0,0,rect.Width(),rect.Height());
            child.SendMessage(WM_PRINT,wp,PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND|PRF_CHILDREN);dc->RestoreDC(saved);
        };
        if(m_ready){print(ActivePage());print(m_eyebrow);print(m_header);print(m_tabs);}return 0;
    }
public:
    void SelectView(int index)
    {
        if(!m_ready)return;m_active=std::clamp(index,0,static_cast<int>(m_pages.size())-1);m_tabs.SetCurSel(m_active);
        for(size_t i=0;i<m_pages.size();++i)m_pages[i]->ShowWindow(static_cast<int>(i)==m_active?SW_SHOW:SW_HIDE);
        PageActivated(m_active);LayoutViews();
    }
    void Refresh(){SelectView(m_active);}
    BOOL PreTranslateMessage(MSG* message) override
    {if(m_ready && ActivePage().PreTranslateMessage(message))return TRUE;return CWnd::PreTranslateMessage(message);}
    DECLARE_MESSAGE_MAP()
};
